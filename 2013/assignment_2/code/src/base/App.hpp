#pragma once
#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"
#include "gui/Image.hpp"
#include "3d/CameraControls.hpp"
#include "gpu/Buffer.hpp"

#include <vector>

#include "RayTracer.hpp"
#include "AreaLight.hpp"
#include "Radiosity.hpp"
#include "Bvh.hpp"

namespace FW
{

//------------------------------------------------------------------------

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
		
		// Assignment 2 actions
		Action_TracePrimaryRays,
		Action_PlaceLightSourceAtCamera,
		Action_ComputeRadiosity,
		Action_LoadRadiosity,
		Action_SaveRadiosity,
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
	void			setupShaders	(GLContext* gl);
	void			loadRadiosity	(const String& fileName);
	void			saveRadiosity	(const String& fileName);

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
	// Assignment 1
	/*
    MeshBase*       m_mesh;
	*/
    CullMode        m_cullMode;

	RayTracer*							m_rt;
	// Assignment 1
	/*
	Renderer*							m_renderer;
	*/
	Radiosity*							m_radiosity;
	AreaLight*							m_areaLight;
	std::vector<Vec3f>					m_rtVertices;
	std::vector<RTTriangle>				m_rtTriangles;
	std::vector<RTToMesh>				m_rtMap;

	int									m_numHemisphereRays;
	int									m_numDirectRays;
	int									m_numBounces;
	float								m_lightSize;
	float								m_toneMapWhite;
	float								m_toneMapBoost;
	Timer								m_updateClock;

	int									m_samplingMode;
	int									m_bvhMode;

	// Assignment 1
	/*
	Image*								m_rtImage;
	bool								m_showRTImage;
	int									m_shadingMode;
	int									m_colorMode;
	int									m_numAORays;
	float								m_aoRayLength;
	*/
};

//------------------------------------------------------------------------
}
