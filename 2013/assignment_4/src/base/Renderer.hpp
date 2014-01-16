#pragma once

#include "3d/CameraControls.hpp"
#include "3d/Mesh.hpp"
#include "base/Random.hpp"
#include "base/MulticoreLauncher.hpp"
#include "base/Timer.hpp"

#include <vector>

#include "RayTracer.hpp"
#include "AreaLight.hpp"
#include "TLSVariable.h"

#define EPSILON 0.001f

namespace FW
{

//------------------------------------------------------------------------

typedef Mesh<VertexPNTC>	MeshWithColors;

//------------------------------------------------------------------------

class RayTracer;
class RTTriangle;
class Image;
class Sequence;

// This class contains functionality to render pictures using a ray tracer.
class Renderer
{
public:
			Renderer();
			~Renderer();

			// are we still processing?
			bool				isRunning							( void ) const		{ return m_launcher.getNumTasks() > 0; }

			// negative #bounces = -N means start russian roulette from Nth bounce
			// positive N means always trace up to N bounces
			void				startPathTracingProcess				( const MeshWithColors* scene, AreaLight*, RayTracer* rt, Image* dest, int bounces, const CameraControls& camera );
			static void			pathTraceScanline					( MulticoreLauncher::Task& t );
			void				updatePicture						( Image* display );	// normalize by 1/w
			void				checkFinish							( void );
			
			void				stop								( void )			
			{ 
				printf("\nPath tracing stopped.\nTotal number of passes: %d\nTotal time spent......: %.4f secs\n\n", 
						m_context.m_pass, m_totalTime);
				m_context.m_bForceExit = true; 
			}

			static Vec3f		albedo								( const MeshWithColors* mesh, const Vec3i& indices, const RTToMesh* map, const Vec3f& barys );


protected:
			// this function fetches a given attribute from the vertices that correspond
			// to the given RTTriangle, and interpolates them using the barycentrics a and b.
			Vec4f				interpolateAttribute				( const RTTriangle* tri, float a, float b, const MeshBase* mesh, int attribidx );

			__forceinline static Vec3f getDirectContribution(const AreaLight* light, int idx, const Vec3f& origin, 
												 const Vec3f& normal, const RayTracer* rt, Sequence* s)
			{
				// Draw a sample on light source
				float pdf;
				Vec3f Pl;
				light->sample( pdf, Pl, idx, s );

				// Construct vector from current vertex (o) to light sample
				Vec3f vl = Pl - origin;
				Vec3f vl_normalized = vl.normalized();

				// Calculate the lamp cosine term and check whether we're at the back of the lamp
				float cosl = FW::clamp(FW::dot(light->getNormal(), -vl_normalized), 0.0f, 1.0f);
				if (cosl == 0.0f) 
					return Vec3f(0.0f);

				// Trace shadow ray to see if it's blocked
				if (!rt->rayCastShadow(origin + EPSILON*normal, vl))
				{
					// If not, add the appropriate emission, 1/r^2 and clamped cosine terms, accounting for the PDF as well.
					// accumulate into E
					float cosv = FW::clamp(FW::dot(vl_normalized, normal), 0.0f, 1.0f);
					float vl_length = vl.length();
					return (light->getEmission() * (1.0f/(vl_length*vl_length)) * cosl * cosv) / pdf;
				}

				return Vec3f(0.0f);
			}

public:
			// YOUR CODE HERE
			// Given a vector n, form an orthogonal matrix with n as the last column, i.e.,
			// a coordinate system aligned such that n is its local z axis.
	static	__forceinline Mat3f	formBasis( const Vec3f& n ) 
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

protected:

	// random number generator
	Random						m_rand;

	__int64						m_s64TotalRays;
	float						m_raysPerSecond;

	Timer						m_passTimer;
	float						m_totalTime;

	struct PathTracerContext
	{
		PathTracerContext()			: m_bForceExit(false), m_bResidual(false), m_scene(0), m_pass(0), m_rt(0), m_image(0), m_coarseImage(0), m_camera(0), m_bounces(0) { }
		bool						m_bForceExit;
		bool						m_bResidual;
		const MeshWithColors*		m_scene;
		RayTracer*					m_rt;
		AreaLight*					m_light;
		int							m_pass;
		int							m_bounces;
		Image*						m_image;
		Image*						m_coarseImage;
		Image*						m_destImage;
		const CameraControls*		m_camera;
		std::vector<Sequence*>		m_sequences;
		
		bool						m_rr;
		float						m_invPI;
		float						m_xCoordMapping;
		float						m_yCoordMapping;

		~PathTracerContext() 
		{
			for (int i = 0; i < m_sequences.size(); ++i)
				delete m_sequences[i];
		}
	};

	MulticoreLauncher			m_launcher;
	PathTracerContext			m_context;

};

};	// namespace FW