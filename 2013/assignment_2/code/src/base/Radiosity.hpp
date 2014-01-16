#pragma once

#include <base/Math.hpp>
#include <base/MulticoreLauncher.hpp>
#include <base/Random.hpp>
#include <base/Timer.hpp>
#include <3d/Mesh.hpp>

#include <vector>

namespace FW
{

class AreaLight;
class RayTracer;

//------------------------------------------------------------------------

typedef Mesh<VertexPNTC>	MeshWithColors;

//------------------------------------------------------------------------

class Radiosity
{
public:
	Radiosity() { }
	~Radiosity();

	// we'll compute radiosity asynchronously, meaning we'll be able to
	// fly around in the scene watching the process complete.
	void	startRadiosityProcess( MeshWithColors* scene, AreaLight* light, RayTracer* rt, int numBounces, int numDirectRays, int numHemisphereRays );

	// are we still processing?
	bool	isRunning() const		{ return m_launcher.getNumTasks() > 0; }
	// sees if we need to switch bounces, etc.
	void	checkFinish();	

	// copy the current solution to the mesh colors for display
	void	updateMeshColors();

protected:
	MulticoreLauncher	m_launcher;

	// This structure holds all the variables needed for computing the illumination
	// in multithreaded fashion. See Radiosity::vertexTaskFunc()
	struct RadiosityContext
	{
		RadiosityContext() : m_scene(0), m_numBounces(1), m_numDirectRays(64), m_numHemisphereRays(256), m_currentBounce(0), m_bForceExit(false) { }

		MeshWithColors*		m_scene;
		AreaLight*			m_light;
		RayTracer*			m_rt;

		Random				m_rand;

		int					m_numBounces;
		int					m_numDirectRays;
		int					m_numHemisphereRays;
		int					m_currentBounce;

		bool				m_bForceExit;

		// these are vectors with one value per vertex
		std::vector<Vec3f>	m_vecCurr;			// this one holds the results for the bounce currently being computed. Zero in the beginning.
		std::vector<Vec3f>	m_vecPrevBounce;	// Once a bounce finishes, the results are copied here to be used as the input illumination to the next bounce.
		std::vector<Vec3f>	m_vecResult;		// This vector should hold a sum of all bounces (used for display).
	};

	static void vertexTaskFunc( MulticoreLauncher::Task& );

	RadiosityContext		m_context;
	Timer					m_timer;
};

} // namepace FW