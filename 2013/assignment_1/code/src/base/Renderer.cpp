#include "Renderer.hpp"
#include "RayTracer.hpp"
#include "Hit.hpp"
#include "RTTriangle.hpp"

#include <iostream>
#include <algorithm>

#define HALTON_MAX_IDX 100000

namespace FW
{

Renderer::Renderer()
{
	m_aoRayLength = 0.5f;
	m_aoNumRays = 16;
	m_raysPerSecond = 0.0f;
}

Renderer::~Renderer()
{
}

void tracePictureThread(MulticoreLauncher::Task& task) {

	Renderer::ThreadData* d = (Renderer::ThreadData*)task.data;
	task.result = new int(0);

	int width = d->image->getSize().x;
	int height = d->image->getSize().y;

	//#pragma omp parallel for
	for (int j = task.idx; j < task.idx+1; ++j)
	{
		for ( int i = 0; i < width; ++i )
		{
			// generate ray through pixel
			float x = (i + 0.5f) / width *  2.0f - 1.0f;
			float y = (j + 0.5f) / height * -2.0f + 1.0f;
			// point on front plane in homogeneous coordinates
			Vec4f P0( x, y, 0.0f, 1.0f );
			// point on back plane in homogeneous coordinates
			Vec4f P1( x, y, 1.0f, 1.0f );

			// apply inverse projection, divide by w to get object-space points
			Vec4f Roh = (d->renderer.invP * P0);
			Vec3f Ro = (Roh * (1.0f / Roh.w)).getXYZ();
			Vec4f Rdh = (d->renderer.invP * P1);
			Vec3f Rd = (Rdh * (1.0f / Rdh.w)).getXYZ();

			// Subtract front plane point from back plane point,
			// yields ray direction.
			// NOTE that it's not normalized; the direction Rd is defined
			// so that the segment to be traced is [Ro, Ro+Rd], i.e.,
			// intersections that come _after_ the point Ro+Rd are to be discarded.
			Rd = Rd - Ro;

			// trace!
			Hit pHit = d->rt->rayCast( Ro, Rd );
			// bookkeeping for perf measurement
			(*((int*)task.result))++;

			// if we hit something, fetch a color and insert into image
			Vec4f color(0,0,0,1);
			if ( pHit.triangle != 0 )
			{
				switch( d->renderer.mode )
				{
					case Renderer::ShadingMode_Headlight:
						color = d->renderer.computeShadingHeadlight( d->rt, d->mesh, pHit, d->cameraCtrl );
						break;
					case Renderer::ShadingMode_AmbientOcclusion:
						(*((int*)task.result)) += d->renderer.m_aoNumRays;
						color = d->renderer.computeShadingAmbientOcclusion( d->rt, d->mesh, pHit, d->cameraCtrl );
						break;
				}
			}
			// put pixel.
			d->image->setVec4f( Vec2i(i,j), color );
		}
	}
}


void Renderer::rayTracePicture( const MeshBase* mesh, RayTracer* rt, Image* image, const CameraControls& cameraCtrl, ShadingMode mode, SamplingMode samplingMode, ColorMode colorMode )
{
	// Get camera orientation and projection
	this->worldToCamera = cameraCtrl.getWorldToCamera();
	this->projection = Mat4f::fitToView(Vec2f(-1,-1), Vec2f(2,2), image->getSize())*cameraCtrl.getCameraToClip();

	// Inverse projection from clip space to world space
	this->invP = (projection * worldToCamera).inverted();

	// Store the shading and sampling modes
	this->mode = mode;
	this->samplingMode = samplingMode;
	this->colorMode = colorMode;

	// For timing the casts
	Timer stopwatch;
	stopwatch.start();
	m_s64TotalRays = 0;

	int height = image->getSize().y;
	int width = image->getSize().x;

	// Create the Multicore launcher
	MulticoreLauncher launcher;
	std::cout << "Number of cores: " << launcher.getNumCores() << std::endl;

	// Create dynamically allocated data for the threads and
	// launch the threads
	ThreadData* data = new ThreadData(mesh, rt, image, cameraCtrl, *this);
	launcher.push(tracePictureThread, data, 0, height);

	// Wait until all the threads are done, accumulate the total rays the threads have traced
	while (launcher.getNumTasks())
		m_s64TotalRays += (*(int*)launcher.pop().result);

	// End timer, see how fast we go
	stopwatch.end();

	// Free all the dynamically allocated ThreadData
	delete data;

	m_raysPerSecond = m_s64TotalRays/stopwatch.getTotal();
	std::cout << "Total time: " << stopwatch.getTotal() << " sec" << std::endl;
	std::cout << "Total rays: " << m_s64TotalRays << std::endl << std::endl;
}


Vec4f Renderer::interpolateAttribute( const RTTriangle* tri, float a, float b, const MeshBase* mesh, int attribidx )
{
	const RTToMesh* map = (const RTToMesh*)tri->m_userPointer;

	Vec4f v[3];
	v[0] = mesh->getVertexAttrib( mesh->indices(map->submesh)[ map->tri_idx ][0], attribidx );
	v[1] = mesh->getVertexAttrib( mesh->indices(map->submesh)[ map->tri_idx ][1], attribidx );
	v[2] = mesh->getVertexAttrib( mesh->indices(map->submesh)[ map->tri_idx ][2], attribidx );
	return (1.0f-a-b)*v[0] + a*v[1] + b*v[2];
}


Vec4f Renderer::computeShadingHeadlight( const RayTracer* rt, const MeshBase* mesh, const Hit& pHit, const CameraControls& cameraCtrl ) const
{
	// compute normal for intersected triangle instead (faceted shading)
	Vec3f n = cross(*pHit.triangle->m_vertices[1] - *pHit.triangle->m_vertices[0], *pHit.triangle->m_vertices[2] - *pHit.triangle->m_vertices[0]).normalized();

	// dot with view ray direction <=> "headlight shading"
	float d = fabs( dot( n, (pHit.intersection-cameraCtrl.getPosition()).normalized() ) );

	// assign gray value (d,d,d)
	Vec3f shade = d;
	
	// get diffuse color
	const RTToMesh* map = (const RTToMesh*)pHit.triangle->m_userPointer;
	Vec3f diffuse = mesh->material( map->submesh ).diffuse.getXYZ();
	// uncomment this to shade in diffuse white
	//Vec3f diffuse = 1;

	// and return
	return Vec4f( shade*diffuse, 1.0f );
}


Vec4f Renderer::computeShadingAmbientOcclusion( const RayTracer* rt, const MeshBase* mesh, const Hit& pHit, const CameraControls& cameraCtrl ) const
{
	// Calculate a vector from the surface point to the camera
	Vec3f surfaceToCam = (cameraCtrl.getPosition()-pHit.intersection).normalized();

	// Compute intersection normal
	// Flip the normal if the camera is on the backside
	Vec3f n = cross(*pHit.triangle->m_vertices[1] - *pHit.triangle->m_vertices[0], *pHit.triangle->m_vertices[2] - *pHit.triangle->m_vertices[0]).normalized();
		
	if ( dot(n, surfaceToCam) < 0.0f ) {
		n *= -1.0f;
	}

	// Get the rotation matrix from the normal
	Mat3f R = formBasis(n);

	// Initialize variables for origin, random direction and number of hits
	// Nudge the ray origin by EPSILON towards the camera
	Vec3f origin = pHit.intersection + (surfaceToCam * 0.001f);
	Vec3f u;
	int noHits = 0;

	// Initialize random
	Random rand;
	unsigned int random = rand.getU32(0, HALTON_MAX_IDX);

	for (int i = 0; i < this->m_aoNumRays; ++i) {
		// Get a random AO ray direction. Choose sampling.
		switch (samplingMode) {

			case SamplingMode::SamplingMode_Normal:
			{
				// Pick two random numbers x,y that are within a unit circle
				do {
					u.x = rand.getF32(-1.0f, 1.0f);
					u.y = rand.getF32(-1.0f, 1.0f);
				} while ( (u.x*u.x + u.y*u.y) > 1.0f );
		
				// Lift the point vertically onto the 3D unit sphere by setting z = sqrt(1 - x^2 - y^2)
				u.z = sqrt(1.0f - (u.x*u.x) - (u.y*u.y));
				break;
			}
			case SamplingMode::SamplingMode_Halton:
			{
				u = Sample::CosineSampleHemisphere(HaltonSampler::generateHaltonNumber(random+i, 2), 
												   HaltonSampler::generateHaltonNumber(random+i, 3));
				break;
			}

		}

		// Rotate the random direction to the normal direction and normalize
		u = (R*u);

		// Trace a ray from the surface point to the random direction
		// The rays are not normalized, the length of the ray describes the maximum distance to trace
		if ( !rt->rayCastAny(origin, this->m_aoRayLength * u) ) {
			noHits++;
		}
	}
	
	
	float color = (float)noHits / m_aoNumRays;

	switch (colorMode) {
		case ColorMode::ColorMode_On:
		{
			const RTToMesh* map = (const RTToMesh*)pHit.triangle->m_userPointer;
			Vec3f diffuse = mesh->material( map->submesh ).diffuse.getXYZ();
			return Vec4f(color*diffuse, 1.0f);
		}
		case ColorMode::ColorMode_Off:
		{
			return Vec4f(color, color, color, 1.0f);
		}
	}
}

Mat3f Renderer::formBasis(const Vec3f& n) 
{
	Mat3f r;
	Vec3f q, b, t;

	// Get the minimum component of n
	int minIdx = 0;
	for (int i = 0; i < 3; ++i) {
		if ( abs(n[i]) < abs(n[minIdx]) ) minIdx = i;
	}

	// Replace the minimum component by 1.0f and take a cross-product to get a vector perpendicular to n
	q = n;
	q[minIdx] = 1.0f;

	t = (cross(q, n)).normalized();

	// Get the last perpendicular vector by taking n x t
	b = (cross(n, t)).normalized();

	// Construct the rotation matrix
	r.setCol(0, t);
	r.setCol(1, b);
	r.setCol(2, n);

	return r;
}

} // namespace FW