#pragma once

#include "3d/CameraControls.hpp"
#include "3d/Mesh.hpp"
#include "base/Random.hpp"
#include "base/MulticoreLauncher.hpp"

#include <vector>
#include <cstdio>

#include "TLSVariable.h"

namespace FW
{

class RayTracer;
class Hit;
class RTTriangle;
class Image;
class ShadingMode;

class Sequence
{

public:

	__forceinline static void initialize(void) {
		FILE* fh2 = fopen("halton_base2.pcalc", "rb");
		FILE* fh3 = fopen("halton_base3.pcalc", "rb");

		if (!fh2 || !fh3) {
			fh2 = fopen("halton_base2.pcalc", "wb");
			fh3 = fopen("halton_base3.pcalc", "wb");

			for (int i = 0; i < maxHaltonIndex; ++i) {
				haltonBase2.push_back(generateHaltonNumber(i, 2));
				haltonBase3.push_back(generateHaltonNumber(i, 3));
			}

			size_t fh2_write = fwrite(&haltonBase2[0], sizeof(float), haltonBase2.size(), fh2);
			size_t fh3_write = fwrite(&haltonBase3[0], sizeof(float), haltonBase3.size(), fh3);
			if (!fh2_write != haltonBase2.size() || !fh3_write != haltonBase3.size())
				printf("ERROR: Could not write all the precalculated Halton values\n");
		}
		else {
			fseek(fh2, 0, SEEK_END);
			unsigned long fh2_len = (unsigned long)ftell(fh2);
			unsigned long fh2_vals = fh2_len/sizeof(float);
			fclose(fh2);

			fseek(fh3, 0, SEEK_END);
			unsigned long fh3_len = (unsigned long)ftell(fh3);
			unsigned long fh3_vals = fh3_len/sizeof(float);
			fclose(fh3);

			haltonBase2 = std::vector<float>(fh2_vals);
			haltonBase3 = std::vector<float>(fh3_vals);

			fh2 = fopen("halton_base2.pcalc", "rb");
			fh3 = fopen("halton_base3.pcalc", "rb");

			size_t fh2_read = fread(&haltonBase2[0], sizeof(float), fh2_vals, fh2);
			size_t fh3_read = fread(&haltonBase3[0], sizeof(float), fh3_vals, fh3);
			if (fh2_read != Sequence::maxHaltonIndex || fh3_read != Sequence::maxHaltonIndex)
				printf("ERROR: Could not read all the precalculated Halton values\n");
		}

		fclose(fh2);
		fclose(fh3);
	}

	__forceinline static float generateHaltonNumber(int idx, int base)
	{
		if (base == 2 && idx < haltonBase2.size())
			return haltonBase2[idx];
		if (base == 3 && idx < haltonBase3.size())
			return haltonBase3[idx];

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

	__forceinline static float generateRandomNumber(void)
	{
		Random rand;
		return rand.getF32();
	}

	__forceinline static float generateGridNumber(int idx, int max)
	{	
		float step = (1.0f/max);
		float offset = step*0.5f;
		return offset+(idx*step);
	}

	__forceinline static float generateStratifiedNumber(Random& m_rand, int idx, int max) {
		float step = (1.0f/max);
		float offset = step*0.5f;
		float subPixelOffset = m_rand.getF32(-offset, offset);

		return (offset+(idx*step))+subPixelOffset;
	}

private:

	static const int			maxHaltonIndex;	// Number of Halton values to precalculate in initialization
	static std::vector<float>	haltonBase2;	// Precalculated vector of maxHaltonIndex num Halton values of base 2
	static std::vector<float>	haltonBase3;	// Precalculated vector of maxHaltonIndex num Halton values of base 3
};


class Sampler
{

public:

	enum SequenceMode
	{
		SequenceMode_Random = 0,
		SequenceMode_Uniform,
		SequenceMode_Halton,
		SequenceMode_Stratified
	};

	__forceinline static void setSequenceMode(Sampler::SequenceMode sm) {
		sequenceMode = sm;
	}

	__forceinline static Vec2f UniformSample(Random& m_rand, int idx, const Vec2f& min =Vec2f(-1.0f), const Vec2f max =Vec2f(1.0f), int maxUSamples =Sampler::maxUniformSamples, SequenceMode mode =Sampler::sequenceMode)
	{
		Vec2f u = sampleSequence(m_rand, idx, maxUSamples, mode);
		return min+u*(max-min);
	}


	__forceinline static Vec3f UniformSampleHemisphere(Random& m_rand, int idx, int maxUSamples =Sampler::maxUniformSamples, SequenceMode mode =Sampler::sequenceMode)
	{
		Vec2f u = sampleSequence(m_rand, idx, maxUSamples, mode);

		float r = sqrt(1.0f - u.x * u.x);
		float phi = 2 * FW_PI * u.y;
 
		return Vec3f(cos(phi) * r, sin(phi) * r, u.x);
	}


	__forceinline static Vec3f CosineSampleHemisphere(Random& m_rand, int idx, int maxUSamples =Sampler::maxUniformSamples, SequenceMode mode =Sampler::sequenceMode)
	{
		Vec2f u = sampleSequence(m_rand, idx, maxUSamples, mode);

		float r = sqrt(u.x);
		float theta = 2.0f * FW_PI * u.y;
 
		float x = r * cos(theta);
		float y = r * sin(theta);
 
		return Vec3f(x, y, sqrt(max(0.0f, 1.0f - u.x)));
	}

private:

	__forceinline static Vec2f sampleSequence(Random& m_rand, int idx, int maxUSamples =Sampler::maxUniformSamples, SequenceMode seqMode =Sampler::sequenceMode) {
		Vec2f u;
		switch (seqMode) {
			case SequenceMode_Random:
				u.x = m_rand.getF32();
				u.y = m_rand.getF32();
				break;
				
			case SequenceMode_Uniform: // TODO: fix
				idx = idx%(maxUSamples*maxUSamples);
				u.x = Sequence::generateGridNumber(idx%maxUSamples, maxUSamples);
				u.y = Sequence::generateGridNumber(idx/maxUSamples, maxUSamples);
				break;
				
			case SequenceMode_Halton:
				u.x = Sequence::generateHaltonNumber(idx, 2);
				u.y = Sequence::generateHaltonNumber(idx, 3);
				break;
			case SequenceMode_Stratified: // TODO: fix
				idx = idx%(maxUSamples*maxUSamples);
				u.x = Sequence::generateStratifiedNumber(m_rand, idx%maxUSamples, maxUSamples);
				u.y = Sequence::generateStratifiedNumber(m_rand, idx/maxUSamples, maxUSamples);
				break;
		}
		return u;
	}

	static	int				maxUniformSamples;
	static	SequenceMode	sequenceMode;
};


// This class contains functionality to render pictures using a ray tracer.
class Renderer
{

// Make the threaded function friend in order to access the protected members
friend		void		tracePictureThread			( MulticoreLauncher::Task& task );
friend		class		Radiosity;

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