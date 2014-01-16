#pragma once
#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"
#include "gui/Image.hpp"
#include "3d/CameraControls.hpp"
#include "gpu/Buffer.hpp"

#include <vector>

#include "RayTracer.hpp"
#include "AreaLight.hpp"
#include "Renderer.hpp"
#include "Sequence.hpp"
#include "Bvh.hpp"


namespace FW
{
//------------------------------------------------------------------------
class RTTriangle;

class App : public Window::Listener, public CommonControls::StateObject
{
private:
    enum Action
    {
        Action_None,

        Action_LoadMesh,
        Action_ReloadMesh,
        Action_SaveMesh,

        Action_ResetCamera,
        Action_EncodeCameraSignature,
        Action_DecodeCameraSignature,

        Action_NormalizeScale,
        Action_FlipXY,
        Action_FlipYZ,
        Action_FlipZ,

        Action_NormalizeNormals,
        Action_FlipNormals,
        Action_RecomputeNormals,

        Action_FlipTriangles,

        Action_CleanMesh,
        Action_CollapseVertices,
        Action_DupVertsPerSubmesh,
        Action_FixMaterialColors,
        Action_DownscaleTextures,
        Action_ChopBehindNear,
		
		// Assignment actions
		Action_TracePrimaryRays,
		Action_PathTraceMode,
		Action_PlaceLightSourceAtCamera,
		Action_ComputeRadiosity,
		Action_LoadRadiosity,
		Action_SaveRadiosity,

		// 
    };

    enum CullMode
    {
        CullMode_None = 0,
        CullMode_CW,
        CullMode_CCW,
    };

    struct RayVertex
    {
        Vec3f       pos;
        U32         color;
    };

public:
                    App             (void);
    virtual         ~App            (void);

    virtual bool    handleEvent     (const Window::Event& ev);
    virtual void    readState       (StateDump& d);
    virtual void    writeState      (StateDump& d) const;

private:
    void            waitKey         (void);
    void            renderFrame     (GLContext* gl);
    void            renderScene     (GLContext* gl, const Mat4f& worldToCamera, const Mat4f& projection);
    void            loadMesh        (const String& fileName);
    void            saveMesh        (const String& fileName);
    void            loadRayDump     (const String& fileName);

    static void     downscaleTextures(MeshBase* mesh);
    static void     chopBehindPlane (MeshBase* mesh, const Vec4f& pleq);

	static bool		fileExists		(const String& filenName);

	// 
	void			constructTracer	(void);
	void			trace			(GLContext*, Image&);

private:
                    App             (const App&); // forbidden
    App&            operator=       (const App&); // forbidden

private:
    Window          m_window;
    CommonControls  m_commonCtrl;
    CameraControls  m_cameraCtrl;

    Action          m_action;
    String          m_meshFileName;
    MeshWithColors*	m_mesh;
    CullMode        m_cullMode;
	Sequence::SequenceType m_sequenceType;
	Bvh::BvhMode	m_bvhMode;

	bool								m_RTMode;
	bool								m_useRussianRoulette;
	Image								m_img;

	Renderer*							m_renderer;

	RayTracer*							m_rt;
	AreaLight*							m_areaLight;
	std::vector<Vec3f>					m_rtVertices;
	std::vector<RTTriangle>				m_rtTriangles;
	std::vector<RTToMesh>				m_rtMap;
	int									m_numBounces;
	float								m_lightSize;
	Timer								m_updateClock;
};

//------------------------------------------------------------------------
}
