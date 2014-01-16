#include "Radiosity.hpp"
#include "AreaLight.hpp"
#include "RayTracer.hpp"
#include "Renderer.hpp"
#include "RTTriangle.hpp"
#include "Hit.hpp"
#include <iostream>

namespace FW
{

// --------------------------------------------------------------------------

Radiosity::~Radiosity()
{
	if ( isRunning() )
	{
		m_context.m_bForceExit = true;
		while( m_launcher.getNumTasks() > m_launcher.getNumFinished() )
			Sleep( 1 );
		m_launcher.popAll();
	}
}

// --------------------------------------------------------------------------

void Radiosity::vertexTaskFunc( MulticoreLauncher::Task& task )
{
	RadiosityContext& ctx = *(RadiosityContext*)task.data;

	if( ctx.m_bForceExit ) return;

	// Which vertex are we to compute?
	int v = task.idx;

	// Fetch vertex and its normal
	Vec3f n = ctx.m_scene->vertex(v).n;
	Vec3f o = ctx.m_scene->vertex(v).p + 0.01f*n;

	// YOUR CODE HERE
	// This starter code merely puts the color-coded normal into the result.
	//
	// In the first bounce, your task is to compute the direct irradiance
	// falling on this vertex from the area light source.
	// In the subsequent passes, you should compute the irradiance by a
	// hemispherical gathering integral. The commented code below gives you
	// an idea of the loop structure. Note that you also have to account
	// for how diffuse textures modulate the irradiance.

	Random randgen;

	// Direct lighting pass? => integrate direct illumination by shooting shadow rays to light source
	if ( ctx.m_currentBounce == 0 )
	{
		Vec3f E(0);
		int randomIdx = ctx.m_rand.getU32(0, 100000 - ctx.m_numDirectRays);

		for ( int r = 0; r < ctx.m_numDirectRays; ++r )
		{
			// Draw a sample on light source
			float pdf;
			Vec3f Pl;
			ctx.m_light->sample( pdf, Pl, randomIdx + r, ctx.m_numDirectRays, randgen );

			// Construct vector from current vertex (o) to light sample
			Vec3f vl = Pl - o;
			Vec3f vl_normalized = vl.normalized();

			// Calculate the lamp cosine term and check whether we're at the back of the lamp
			float cosl = FW::clamp(FW::dot(ctx.m_light->getNormal(), -vl_normalized), 0.0f, 1.0f);
			if (cosl == 0.0f) break;

			// Trace shadow ray to see if it's blocked
			if (!ctx.m_rt->rayCastShadow(o, vl))
			{
				// if not, add the appropriate emission, 1/r^2 and clamped cosine terms, accounting for the PDF as well.
				// accumulate into E
				float cosv = FW::clamp(FW::dot(vl_normalized, n), 0.0f, 1.0f);
				float vl_length = vl.length();
				E += (ctx.m_light->getEmission() * (1.0f / (vl_length * vl_length)) * cosl * cosv) / pdf;
			}
		}
		// Note we are NOT multiplying by PI here;
		// it's implicit in the hemisphere-to-light source area change of variables.
		// The result we are computing is _irradiance_, not radiosity.
		ctx.m_vecCurr[ v ] = E * (1.0f/ctx.m_numDirectRays);
		ctx.m_vecResult[ v ] = ctx.m_vecCurr[ v ];
	}
	else
	{
		// OK, time for indirect!
		// Implemente hemispherical gathering integral for bounces > 1.

		// Get local coordinate system the rays are shot from.
		Mat3f B = Renderer::formBasis( n );
		Vec3f E(0.0f);
		int randomIdx = ctx.m_rand.getU32(0, 100000-ctx.m_numHemisphereRays);

		for ( int r = 0; r < ctx.m_numHemisphereRays; ++r )
		{
			// Draw a cosine weighted direction and find out where it hits (if anywhere)
			// You need to transform it from the local frame to the vertex' hemisphere using B.

			// Make the direction long but not too long to avoid numerical instability in the ray tracer.
			// For our scenes, 100 is a good length. I (know, this special casing sucks.)

			// Shoot ray, see where we hit
			const float scale = 100.0f;
			Vec3f d = scale * B * Sampler::CosineSampleHemisphere(randgen, randomIdx+r, (int)FW::sqrt((float)ctx.m_numHemisphereRays));
			Vec3f d_normalized = d.normalized();
			const Hit hit = ctx.m_rt->rayCast( o, d );
			const RTTriangle* tri = hit.triangle;

			if ( tri != 0 )
			{
				// Interpolate lighting from previous pass
				const RTToMesh* map = (const RTToMesh*)tri->m_userPointer;
				const Vec3i& indices = ctx.m_scene->indices( map->submesh )[ map->tri_idx ];

				// Fetch barycentrics
				Vec3f barycentrics = tri->getBarycentrics(hit.intersection);

				// Check for backfaces => don't accumulate if we hit a surface from below!
				// TODO: create switch for smooth vs. flat shading
				float cosl = FW::dot(tri->getNormal(), -d_normalized);
				if (cosl < 0.0f) continue;

				// Ei = interpolated irradiance determined by ctx.m_vecPrevBounce from vertices using the barycentrics
				Vec3f Ei = barycentrics[0]*ctx.m_vecPrevBounce[indices[0]] + 
						   barycentrics[1]*ctx.m_vecPrevBounce[indices[1]] + 
						   barycentrics[2]*ctx.m_vecPrevBounce[indices[2]];

				// Divide incident irradiance by PI so that we can turn it into outgoing
				// radiosity by multiplying by the reflectance factor below.
				Ei *= (1.0f / FW_PI);

				// check for texture
				const MeshBase::Material& mat = ctx.m_scene->material(map->submesh);
				if ( mat.textures[MeshBase::TextureType_Diffuse].exists() )
				{
					// Yes, texture; fetch diffuse albedo from there.
					// First interpolate UV coordinates from the vertices using barycentrics
					// Then turn those into pixel coordinates within the texture
					// Finally fetch the color using Image::getVec4f() (point sampling is fine).
					// Use the result as the diffuse reflectance instead of mat.diffuse.

					const MeshWithColors& mesh = *ctx.m_scene;
					const Texture& tex = mat.textures[MeshBase::TextureType_Diffuse];
					const Image& teximg = *tex.getImage();

					Vec2f texCoord = barycentrics[0]*ctx.m_scene->vertex(indices[0]).t + 
						             barycentrics[1]*ctx.m_scene->vertex(indices[1]).t + 
									 barycentrics[2]*ctx.m_scene->vertex(indices[2]).t;

					Vec2i texCoordi = Vec2i( (texCoord.x - floor(texCoord.x))*teximg.getSize().x, 
											 (texCoord.y - floor(texCoord.y))*teximg.getSize().y );

					Ei *= teximg.getVec4f(texCoordi).getXYZ();
				}
				else
				{
					Ei *= mat.diffuse.getXYZ();
				}

				E += Ei;	// accumulate
			}
		}

		// Store result for this bounce
		// Note that since we are storing irradiance, we multiply by PI(
		// (Remember the slides about cosine weighted importance sampling!)
		ctx.m_vecCurr[ v ] = E * (FW_PI / ctx.m_numHemisphereRays);
		// Also add to the global accumulator.
		ctx.m_vecResult[ v ] = ctx.m_vecResult[ v ] + ctx.m_vecCurr[ v ];

		// uncomment this to visualize only the current bounce
		//ctx.m_vecResult[ v ] = ctx.m_vecCurr[ v ];
	}
}
// --------------------------------------------------------------------------

void Radiosity::startRadiosityProcess( MeshWithColors* scene, AreaLight* light, RayTracer* rt, int numBounces, int numDirectRays, int numHemisphereRays )
{
	// put stuff the asyncronous processor needs 
	m_context.m_scene				= scene;
	m_context.m_rt					= rt;
	m_context.m_light				= light;
	m_context.m_currentBounce		= 0;
	m_context.m_numBounces			= numBounces;
	m_context.m_numDirectRays		= numDirectRays;
	m_context.m_numHemisphereRays	= numHemisphereRays;

	// resize all the buffers according to how many vertices we have in the scene
	m_context.m_vecCurr.resize( scene->numVertices() );
	m_context.m_vecPrevBounce.resize( scene->numVertices() );
	m_context.m_vecResult.assign( scene->numVertices(), Vec3f(0,0,0) );

	// fire away!
	m_launcher.setNumThreads(m_launcher.getNumCores());	// the solution exe is multithreaded
	//m_launcher.setNumThreads(1);						// but you have to make sure your code is thread safe before enabling this!
	m_launcher.popAll();
	m_timer.start();
	m_launcher.push( vertexTaskFunc, &m_context, 0, scene->numVertices() );
}
// --------------------------------------------------------------------------

void Radiosity::updateMeshColors()
{
	// Print progress.
	printf( "%.2f%% done     \r", 100.0f*m_launcher.getNumFinished()/m_context.m_scene->numVertices() );

	// Copy irradiance over to the display mesh.
	// Because we want outgoing radiosity in the end, we divide by PI here
	// and let the shader multiply the final diffuse reflectance in. See App::setupShaders() for details.
	for( S32 i = 0; i < m_context.m_scene->numVertices(); ++i )
		m_context.m_scene->mutableVertex(i).c = m_context.m_vecResult[ i ] * (1.0f/FW_PI);
}
// --------------------------------------------------------------------------

void Radiosity::checkFinish()
{
	// have all the vertices from current bounce finished computing?
	if ( m_launcher.getNumTasks() == m_launcher.getNumFinished() )
	{
		// yes, remove from task list
		m_launcher.popAll();

		// more bounces desired?
		if ( m_context.m_currentBounce < m_context.m_numBounces )
		{
			// move current bounce to prev
			m_context.m_vecPrevBounce = m_context.m_vecCurr;
			++m_context.m_currentBounce;
			// start new tasks for all vertices
			m_launcher.push( vertexTaskFunc, &m_context, 0, m_context.m_scene->numVertices() );
			printf( "\nStarting bounce %d\n", m_context.m_currentBounce );
		}
		else printf( "\n DONE!\n" );
	}

	if (!isRunning() && m_launcher.getNumTasks() == 0) {
		m_timer.end();
		printf("Radiosity computed in: %f sec\n\n", m_timer.getTotal());
		m_timer.clearTotal();
	}
}
// --------------------------------------------------------------------------

} // namespace FW