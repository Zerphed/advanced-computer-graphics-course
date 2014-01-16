#pragma once
#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"
#include "gui/Image.hpp"
#include "3d/CameraControls.hpp"
#include "gpu/Buffer.hpp"

#include <vector>

#include "RayTracer.hpp"
#include "InstantRadiosity.hpp"

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
		Action_PlaceLightSourceAtCamera,
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
    void            loadMesh        (const String& fileName);
    void            saveMesh        (const String& fileName);
    void            loadRayDump     (const String& fileName);

    static void     downscaleTextures(MeshBase* mesh);
    static void     chopBehindPlane (MeshBase* mesh, const Vec4f& pleq);

	static bool		fileExists		(const String& filenName);

	// 
	void			constructTracer	(void);
	void			setupShaders	(GLContext* gl);

	void			setupRenderToTexture(const Vec2i& resolution);
	void			deleteRenderToTexture();
	void			blitRttToScreen(GLContext* gl);
	void			blitShadowMapToScreen(GLContext* gl, const LightSource& ls);

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

	RayTracer*							m_rt;
	LightSource*						m_lightSource;
	std::vector<Vec3f>					m_rtVertices;
	std::vector<RTTriangle>				m_rtTriangles;
	std::vector<RTToMesh>				m_rtMap;
	int									m_numHemisphereRays;
	float								m_lightSize;
	float								m_toneMapWhite;
	float								m_toneMapBoost;
	Timer								m_updateClock;


	ShadowMapContext					m_smcontext;
	InstantRadiosity					m_instantRadiosity;
	float								m_shadowMapVisMultiplier;
	float								m_lightFOV;
	float								m_indirectFOV;
	float								m_lightIntensity;
	bool								m_visualizeLight;
	bool								m_visualizeIndirect;
	bool								m_renderFromLight;
	int									m_num_indirect;
	int									m_smResolutionLevel;
	int									m_smResolutionLevelPrev;

	GLuint								m_rttFBO;
	GLuint								m_rttDepth;
	GLuint								m_rttTex;
};

//------------------------------------------------------------------------
}
