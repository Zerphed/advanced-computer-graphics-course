
#include <iostream>
#include <fstream>

#include "io/ImageLodePngIO.hpp"
#include "io/File.hpp"

#include "Renderer.hpp"
#include "RayTracer.hpp"
#include "RTTriangle.hpp"
#include "Sampler.hpp"
#include "Hit.hpp"

namespace FW
{

static std::vector<std::vector<int>> randomseeds;

Renderer::Renderer()
{
	m_raysPerSecond = 0.0f;
}

Renderer::~Renderer()
{
	if ( isRunning() )
	{
		m_context.m_bForceExit = true;
		while( m_launcher.getNumTasks() > m_launcher.getNumFinished() )
			Sleep( 1 );
		m_launcher.popAll();
	}
	delete m_context.m_image;
	delete m_context.m_coarseImage;
}

// 
Vec4f Renderer::interpolateAttribute( const RTTriangle* tri, float a, float b, const MeshBase* mesh, int attribidx )
{
	const RTToMesh* map = (const RTToMesh*)tri->m_userPointer;

	Vec4f v[3];
	v[0] = mesh->getVertexAttrib( mesh->indices(map->submesh)[ map->tri_idx ][0], attribidx );
	v[1] = mesh->getVertexAttrib( mesh->indices(map->submesh)[ map->tri_idx ][1], attribidx );
	v[2] = mesh->getVertexAttrib( mesh->indices(map->submesh)[ map->tri_idx ][2], attribidx );
	return (1.0f-a-b)*v[0] + a*v[1] + b*v[2];
}


// YOUR CODE HERE
// copy the code over from the radiosity assignment.
// you should return the diffuse albedo from the point hit by the ray.
// if textured, use the texture; if not, use Material.diffuse.
Vec3f Renderer::albedo( const MeshWithColors* mesh, const Vec3i& indices, const RTToMesh* map, const Vec3f& barys )
{
	Vec3f Kd;

	// Check for texture
	const MeshBase::Material& mat = mesh->material(map->submesh);

	if ( mat.textures[MeshBase::TextureType_Diffuse].exists() )
	{
		// Yes, texture; fetch diffuse albedo from there.
		// First interpolate UV coordinates from the vertices using barycentrics
		// Then turn those into pixel coordinates within the texture
		// Finally fetch the color using Image::getVec4f() (point sampling is fine).
		// Use the result as the diffuse reflectance instead of mat.diffuse.

		const Texture& tex = mat.textures[MeshBase::TextureType_Diffuse];
		const Image& teximg = *tex.getImage();

		Vec2f texCoord = barys[0] * mesh->vertex(indices[0]).t + 
						 barys[1] * mesh->vertex(indices[1]).t + 
						 barys[2] * mesh->vertex(indices[2]).t;

		Vec2i texCoordi = Vec2i( (int)((texCoord.x - floor(texCoord.x))*teximg.getSize().x), 
								 (int)((texCoord.y - floor(texCoord.y))*teximg.getSize().y) );

		Kd = teximg.getVec4f(texCoordi).getXYZ();
	}
	else
	{
		Kd = mat.diffuse.getXYZ();
	}

	return Kd;
}


// This function is responsible for asynchronously rendering one path per pixel for a given scanline.
// The path tracer logic you write goes in here. And it's pretty much all you _must_ do this time!
void Renderer::pathTraceScanline( MulticoreLauncher::Task& t )
{
	static Mat3f oldCameraOrientation;
	static Vec3f oldCameraPosition;
	static Mat4f invP;

	PathTracerContext& ctx = *(PathTracerContext*)t.data;

	const MeshWithColors* scene			= ctx.m_scene;
	RayTracer* rt						= ctx.m_rt;
	Image* image						= ctx.m_image;
	const CameraControls& cameraCtrl	= *ctx.m_camera;
	AreaLight* light					= ctx.m_light;
	bool rr								= ctx.m_rr;

	// Make sure we're on CPU
	image->getMutablePtr();

	// Get camera orientation and projection (and compute inverse projection if changed)
	if (cameraCtrl.getOrientation() != oldCameraOrientation || cameraCtrl.getPosition() != oldCameraPosition)  {
		Mat4f worldToCamera				= cameraCtrl.getWorldToCamera();
		Mat4f projection				= Mat4f::fitToView(Vec2f(-1.0f,-1.0f), Vec2f(2.0f,2.0f), image->getSize())*cameraCtrl.getCameraToClip();
	
		// Inverse projection from clip space to world space
		invP							= (projection*worldToCamera).inverted();
	}

	// Scanline index from task
	int j = t.idx;

	float inv_PI						= ctx.m_invPI;
	float x_coord_mapping				= ctx.m_xCoordMapping;
	float y_coord_mapping				= ctx.m_yCoordMapping;

	int width = image->getSize().x;
	int height = image->getSize().y;

	// We're doing bounces + 1 direct iterations
	int iterations						= abs(ctx.m_bounces) + 1;

	for ( int i = 0; i < width; ++i )
	{
		if( ctx.m_bForceExit )
			return;

		// Generate a ray through pixel (with anti-aliasing)
		Vec2f jitter = Sampler::uniformSample(ctx.m_sequences[j], randomseeds[j][i] + ctx.m_pass, Vec2f(0.0f), Vec2f(1.0f));
		float x = (i + jitter.x) * x_coord_mapping - 1.0f; // / width *  2.0f - 1.0f;
		float y = (j + jitter.y) * y_coord_mapping + 1.0f; // / height * -2.0f + 1.0f;

		// Point on front and black planes in homogeneous coordinates
		Vec4f P0( x, y, 0.0f, 1.0f );
		Vec4f P1( x, y, 1.0f, 1.0f );

		// Apply inverse projection, divide by w to get object-space points
		Vec4f Roh = (invP * P0);
		Vec3f Ro = (Roh/Roh.w).getXYZ();
		Vec4f Rdh = (invP * P1);
		Vec3f Rd = (Rdh/Rdh.w).getXYZ();

		// Subtract front plane point from back plane point,
		// yields ray direction.
		// NOTE that it's not normalized; the direction Rd is defined
		// so that the segment to be traced is [Ro, Ro+Rd], i.e.,
		// intersections that come _after_ the point Ro+Rd are to be discarded.
		Rd = Rd - Ro;

		Vec3f tcol = Vec3f(0.0f); // Total color
		Vec3f fcol = Vec3f(1.0f); 

		Vec3f scol;
		Vec3f dcol;
		Vec3f normal;
		//debugfile << "[\n"; // DEBUG

		// Go through the direct and indirect bounces and trace the paths
		for (int b = 0; b < iterations; ++b) 
		{
			// Trace ray through the pixel
			Hit pHit = rt->rayCast( Ro, Rd );

			// If we hit something, fetch a color and insert into image
			if ( pHit.triangle != 0 )
			{
				// Perform the path tracing operations for this pixel.
				const RTToMesh* map = (const RTToMesh*)pHit.triangle->m_userPointer;
				const Vec3i& indices = ctx.m_scene->indices( map->submesh )[ map->tri_idx ];
				const Vec3f barys = pHit.triangle->getBarycentrics(pHit.intersection);

				// Get the surface color
				scol = Renderer::albedo(ctx.m_scene, indices, map, barys);

				// Get the normal
				normal = pHit.triangle->getNormal(pHit.intersection, barys);
				if (FW::dot(Rd, normal) > 0.0f)
					normal = -normal;

				// Add the direct contribution
				dcol = Renderer::getDirectContribution(ctx.m_light, randomseeds[(j+1)%height][i] + ctx.m_pass, pHit.intersection, normal, ctx.m_rt, ctx.m_sequences[j]);
				
				// Update the origin and direction for another round
				Ro = pHit.intersection + EPSILON*normal;
				//debugfile << "(" << Ro.x << "," << Ro.y << "," << Ro.z << ")\n";

				Rd = formBasis(normal) * Sampler::cosineSampleHemisphere(ctx.m_sequences[j], randomseeds[(j+2)%height][i] + ctx.m_pass) * 100.0f;

				fcol *= scol;
				tcol += fcol * dcol;
			}
			else 
			{
				break;
			}
		}

		// If russian roulette is on enter the russian roulette stage
		if (rr) 
		{
			unsigned int contribution = 2;
			while ( ctx.m_sequences[j]->getRandomU32() & 0x01 ) 
			{
				// Trace ray through the pixel
				Hit pHit = rt->rayCast( Ro, Rd );

				// If we hit something, fetch a color and insert into image
				if ( pHit.triangle != 0 )
				{
					// Perform the path tracing operations for this pixel.
					const RTToMesh* map = (const RTToMesh*)pHit.triangle->m_userPointer;
					const Vec3i& indices = ctx.m_scene->indices( map->submesh )[ map->tri_idx ];
					const Vec3f barys = pHit.triangle->getBarycentrics(pHit.intersection);

					scol = Renderer::albedo(ctx.m_scene, indices, map, barys);

					// Get the normal
					normal = pHit.triangle->getNormal(pHit.intersection, barys);
					if (FW::dot(Rd, normal) > 0.0f)
						normal = -normal;

					// Add the contribution
					dcol = Renderer::getDirectContribution(ctx.m_light, randomseeds[(j+1)%(int)height][i] + ctx.m_pass, pHit.intersection, normal, ctx.m_rt, ctx.m_sequences[j]);

					// Update the origin and direction for another round
					Ro = pHit.intersection + EPSILON*normal;
					//debugfile << "(" << Ro.x << "," << Ro.y << "," << Ro.z << ")\n";

					Rd = formBasis(normal) * Sampler::cosineSampleHemisphere(ctx.m_sequences[j], randomseeds[(j+2)%(int)height][i] + ctx.m_pass) * 100.0f;

					fcol *= scol;
					tcol += (float)contribution * fcol * dcol;
				
					contribution <<= 1;
				}
				else 
				{
					break;
				}
			}
		}

		//debugfile << "]\n";
		// Remember to divide the BRDF by PI
		tcol *= inv_PI;

		// Put pixel
		Vec4f prev = image->getVec4f( Vec2i(i,j) );
		prev += Vec4f( tcol, 1.0f );
		image->setVec4f( Vec2i(i,j), prev );
	}

}

void Renderer::startPathTracingProcess( const MeshWithColors* scene, AreaLight* light, RayTracer* rt, Image* dest, int bounces, const CameraControls& camera )
{
	FW_ASSERT( !m_context.m_bForceExit );

	// HAXXXXX
	randomseeds.clear();
	for (int i = 0; i < dest->getSize().y; ++i) {
		randomseeds.push_back(std::vector<int>());
		for (int j = 0; j < dest->getSize().x; ++j)
			randomseeds[i].push_back(m_rand.getU32()%100000);
	}

	// Delete the old context member variables
	delete m_context.m_image;
	delete m_context.m_coarseImage;

	for (int i = 0; i < m_context.m_sequences.size(); ++i)
		delete m_context.m_sequences[i];
	m_context.m_sequences.clear();

	m_context.m_bForceExit = false;
	m_context.m_bResidual = false;
	m_context.m_camera = &camera;
	m_context.m_rt = rt;
	m_context.m_scene = scene;
	m_context.m_light = light;
	m_context.m_pass = 0;
	m_context.m_bounces = bounces;
	m_context.m_image = new Image( dest->getSize(), ImageFormat::RGBA_Vec4f );
	m_context.m_coarseImage = new Image( dest->getSize(), ImageFormat::RGBA_Vec4f );
	
	m_context.m_rr = bounces < 0 ? true : false;
	m_context.m_invPI = 1.0f/FW_PI;
	m_context.m_xCoordMapping = 1.0f / m_context.m_image->getSize().x *  2.0f;
	m_context.m_yCoordMapping = 1.0f / m_context.m_image->getSize().y *  -2.0f;

	m_context.m_destImage = dest;
	m_context.m_image->clear();
	m_context.m_coarseImage->clear();

	// Create new sequences
	for (int i = 0; i < dest->getSize().y; ++i)
		m_context.m_sequences.push_back(Sampler::getSequenceInstance());

	dest->clear();

	// Print statistics
	::printf("Path tracing started.\nSequence mode...: %s\nIndirect bounces: %d\nRussian roulette: %s\n\n", 
		     Sampler::getSequenceInstanceStr(), FW::abs(m_context.m_bounces), m_context.m_rr ? "Enabled" : "Disabled");

	// fire away!
	m_launcher.setNumThreads( m_launcher.getNumCores() );	// the solution exe is multithreaded
	m_launcher.popAll();
	m_launcher.push( pathTraceScanline, &m_context, 0, dest->getSize().y );
	
	m_totalTime = 0.0f;
	m_passTimer.start();
}

void Renderer::updatePicture( Image* dest )
{
	FW_ASSERT( m_context.m_image != 0 );
	FW_ASSERT( m_context.m_image->getSize() == dest->getSize() );
	for ( int i = 0; i < dest->getSize().y; ++i )
		for ( int j = 0; j < dest->getSize().x; ++j )
		{
			Vec4f D = m_context.m_image->getVec4f(Vec2i(j,i));
			if ( D.w != 0.0f )
				D = D*(1.0f/D.w);
			dest->setVec4f( Vec2i(j,i), D + m_context.m_coarseImage->getVec4f(Vec2i(j,i)) );
		}
}

void Renderer::checkFinish()
{
	// have all the vertices from current bounce finished computing?
	if ( m_launcher.getNumTasks() == m_launcher.getNumFinished() )
	{
		// yes, remove from task list
		m_launcher.popAll();

		++m_context.m_pass;

		// you may want to uncomment this to write out a sequence of PNG images
		// after the completion of each full round through the image.
		//String fn = sprintf( "pt-%03dppp.png", m_context.m_pass );
		//File outfile( fn, File::Create );
		//exportLodePngImage( outfile, m_context.m_destImage );

		if ( !m_context.m_bForceExit )
		{
			// keep going
			m_launcher.setNumThreads( m_launcher.getNumCores() );	// the solution exe is multithreaded
			m_launcher.popAll();
			m_launcher.push( pathTraceScanline, &m_context, 0, m_context.m_image->getSize().y );
			::printf( "Pass %d done, time used for pass: %.4f secs\n", m_context.m_pass, m_passTimer.getElapsed() );
			m_totalTime += m_passTimer.getElapsed();
			m_passTimer.start();
		}
		else
			::printf( "Stopped.\n" );
	}

}
// --------------------------------------------------------------------------



} // namespace FW