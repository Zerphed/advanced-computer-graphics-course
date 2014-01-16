#pragma once

#include "3d/CameraControls.hpp"
#include "3d/Mesh.hpp"
#include "base/Random.hpp"
#include "base/MulticoreLauncher.hpp"

#include <vector>

#include "TLSVariable.h"

namespace FW
{

class RayTracer;
class Hit;
class RTTriangle;
class Image;
class ShadingMode;

class HaltonSampler
{
public:
	/* This function generates the n-th number in Halton's low
	 * discrepancy sequence.
	 */
	static float generateHaltonNumber(int idx, int base) 
	{
		
		float result = 0.0f;
		float f = 1.0f / base;
		int i = idx;
		while (i > 0.0f) 
		{
			result = result + f * (i%base);
			i = (int)floor((float)i / base);
			f = f / base;
		}
		return result;
		
	}
	
};

class Sample 
{
public:
	
	static Vec3f UniformSampleHemisphere(float u1, float u2)
	{
		static const float PI = 3.14159265359;
		float r = sqrt(1.0f - u1 * u1);
		float phi = 2 * PI * u2;
 
		return Vec3f(cos(phi) * r, sin(phi) * r, u1);
	}


	static Vec3f CosineSampleHemisphere(float u1, float u2)
	{
		static const float PI = 3.14159265359;
		float r = sqrt(u1);
		float theta = 2.0f * PI * u2;
 
		float x = r * cos(theta);
		float y = r * sin(theta);
 
		return Vec3f(x, y, sqrt(max(0.0f, 1.0f - u1)));
	}
};

// This class contains functionality to render pictures using a ray tracer.
class Renderer
{

// Make the threaded function friend in order to access the protected members
friend		void		tracePictureThread			(MulticoreLauncher::Task& task);

public:
			Renderer();
			~Renderer();

			// this struct helps to map the linear indices of the ray tracer triangles
			// back to the (submesh, tri) indexing of BaseMesh. Helps when getting colors,
			// normals, etc.
			struct RTToMesh
			{
				int submesh;
				int tri_idx;
			};

			enum ShadingMode
			{
				ShadingMode_Headlight = 0,
				ShadingMode_AmbientOcclusion
			};
	
			enum SamplingMode
			{
				SamplingMode_Normal = 0,
				SamplingMode_Halton,
			};

			enum ColorMode
			{
				ColorMode_Off = 0,
				ColorMode_On,
			};


			struct ThreadData {
				ThreadData (const MeshBase* mesh, const RayTracer* rt, Image* image, const CameraControls& cameraCtrl, const Renderer& renderer) :
					mesh(mesh), rt(rt), image(image), cameraCtrl(cameraCtrl), renderer(renderer)
				{
				}

				const MeshBase* mesh;
				const RayTracer* rt;
				Image* image;
				const CameraControls& cameraCtrl;
				const Renderer& renderer;
			};

			// draw a picture of the mesh from the viewpoint specified by the matrix.
			// shading mode determined by the enum.
			void				rayTracePicture						( const MeshBase*, RayTracer*, Image*, const CameraControls& camera, ShadingMode mode, SamplingMode samplingMode,
																		ColorMode colorMode);


			// controls how long the ambient occlusion rays are.
			// what looks good depends on the scene.
			void				setAORayLength						( float len )		{ m_aoRayLength = len; }

			// how many ambient occlusion rays to shoot per primary ray.
			void				setAONumRays						( int i )			{ m_aoNumRays = i; }


			float				getRaysPerSecond					( void )			{ return m_raysPerSecond; }


protected:
			// this function fetches a given attribute from the vertices that correspond
			// to the given RTTriangle, and interpolates them using the barycentrics a and b.
			Vec4f				interpolateAttribute				( const RTTriangle* tri, float a, float b, const MeshBase* mesh, int attribidx );

			// simple headlight shading, ray direction dot surface normal
			Vec4f				computeShadingHeadlight				( const RayTracer* rt, const MeshBase* mesh, const Hit& pHit, const CameraControls& cameraCtrl ) const;

			// YOUR CODE HERE
			// implement ambient occlusion as per the instructions
			Vec4f				computeShadingAmbientOcclusion		( const RayTracer* rt, const MeshBase* mesh, const Hit& pHit, const CameraControls& cameraCtrl ) const;

			// YOUR CODE HERE
			// Given a vector n, form an orthogonal matrix with n as the last column, i.e.,
			// a coordinate system aligned such that n is its local z axis.
			static	Mat3f		formBasis							( const Vec3f& n );

	
			// Projection matrices for this renderer, these need to be updated in rayTracePicture
			// but are better to be kept here than be given as a parameter
			Mat4f						worldToCamera;
			Mat4f						projection;
			Mat4f						invP;
			ShadingMode					mode;
			SamplingMode				samplingMode;
			ColorMode					colorMode;

			float						m_aoRayLength;
			int							m_aoNumRays;

			__int64						m_s64TotalRays;
			float						m_raysPerSecond;

};

};	// namespace FW