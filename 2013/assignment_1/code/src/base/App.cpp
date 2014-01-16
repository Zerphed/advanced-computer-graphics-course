#define _CRT_SECURE_NO_WARNINGS

#include "App.hpp"
#include "base/Main.hpp"
#include "gpu/GLContext.hpp"
#include "3d/Mesh.hpp"
#include "io/File.hpp"
#include "io/StateDump.hpp"
#include "base/Random.hpp"

#include "RayTracer.hpp"
#include "Renderer.hpp"
#include "RTTriangle.hpp"

#include <stdio.h>
#include <conio.h>

using namespace FW;

//------------------------------------------------------------------------

App::App(void)
:   m_commonCtrl    (CommonControls::Feature_Default & ~CommonControls::Feature_RepaintOnF5),
    m_cameraCtrl    (&m_commonCtrl, CameraControls::Feature_Default | CameraControls::Feature_StereoControls),
    m_action        (Action_None),
    m_mesh          (NULL),
    m_cullMode      (CullMode_None),
	m_rt			(NULL),
	m_rtImage		(NULL),
	m_shadingMode	(Renderer::ShadingMode_Headlight),
	m_samplingMode	(Renderer::SamplingMode::SamplingMode_Normal),
	m_colorMode		(Renderer::ColorMode::ColorMode_Off),
	m_showRTImage	(false),
	m_numAORays		(16),
	m_aoRayLength	(1.0f)
{
    m_commonCtrl.showFPS(true);
    m_commonCtrl.addStateObject(this);
    m_cameraCtrl.setKeepAligned(true);

    m_commonCtrl.addButton((S32*)&m_action, Action_LoadMesh,                FW_KEY_M,       "Load mesh... (M)");
    m_commonCtrl.addButton((S32*)&m_action, Action_ReloadMesh,              FW_KEY_F5,      "Reload mesh (F5)");
    m_commonCtrl.addButton((S32*)&m_action, Action_SaveMesh,                FW_KEY_O,       "Save mesh... (O)");
    m_commonCtrl.addSeparator();

    m_commonCtrl.addButton((S32*)&m_action, Action_ResetCamera,             FW_KEY_NONE,    "Reset camera");
    m_commonCtrl.addButton((S32*)&m_action, Action_EncodeCameraSignature,   FW_KEY_NONE,    "Encode camera signature");
    m_commonCtrl.addButton((S32*)&m_action, Action_DecodeCameraSignature,   FW_KEY_NONE,    "Decode camera signature...");
    m_window.addListener(&m_cameraCtrl);
    m_commonCtrl.addSeparator();

    m_commonCtrl.addButton((S32*)&m_action, Action_NormalizeScale,          FW_KEY_NONE,    "Normalize scale");
    m_commonCtrl.addButton((S32*)&m_action, Action_FlipXY,                  FW_KEY_NONE,    "Flip X/Y");
    m_commonCtrl.addButton((S32*)&m_action, Action_FlipYZ,                  FW_KEY_NONE,    "Flip Y/Z");
    m_commonCtrl.addButton((S32*)&m_action, Action_FlipZ,                   FW_KEY_NONE,    "Flip Z");
    m_commonCtrl.addSeparator();

    m_commonCtrl.addButton((S32*)&m_action, Action_NormalizeNormals,        FW_KEY_NONE,    "Normalize normals");
    m_commonCtrl.addButton((S32*)&m_action, Action_FlipNormals,             FW_KEY_NONE,    "Flip normals");
    m_commonCtrl.addButton((S32*)&m_action, Action_RecomputeNormals,        FW_KEY_NONE,    "Recompute normals");
    m_commonCtrl.addSeparator();

    m_commonCtrl.addToggle((S32*)&m_cullMode, CullMode_None,                FW_KEY_NONE,    "Disable backface culling");
    m_commonCtrl.addToggle((S32*)&m_cullMode, CullMode_CW,                  FW_KEY_NONE,    "Cull clockwise faces");
    m_commonCtrl.addToggle((S32*)&m_cullMode, CullMode_CCW,                 FW_KEY_NONE,    "Cull counter-clockwise faces");
    m_commonCtrl.addButton((S32*)&m_action, Action_FlipTriangles,           FW_KEY_NONE,    "Flip triangles");
    m_commonCtrl.addSeparator();

    m_commonCtrl.addButton((S32*)&m_action, Action_CleanMesh,               FW_KEY_NONE,    "Remove unused materials, denegerate triangles, and unreferenced vertices");
    m_commonCtrl.addButton((S32*)&m_action, Action_CollapseVertices,        FW_KEY_NONE,    "Collapse duplicate vertices");
    m_commonCtrl.addButton((S32*)&m_action, Action_DupVertsPerSubmesh,      FW_KEY_NONE,    "Duplicate vertices shared between multiple materials");
    m_commonCtrl.addButton((S32*)&m_action, Action_FixMaterialColors,       FW_KEY_NONE,    "Override material colors with average over texels");
    m_commonCtrl.addButton((S32*)&m_action, Action_DownscaleTextures,       FW_KEY_NONE,    "Downscale textures by 2x");
    m_commonCtrl.addButton((S32*)&m_action, Action_ChopBehindNear,          FW_KEY_NONE,    "Chop triangles behind near plane");
    m_commonCtrl.addSeparator();

	// 
	m_commonCtrl.addButton((S32*)&m_action, Action_TracePrimaryRays,        FW_KEY_ENTER,   "Trace Rays (ENTER)");
	m_commonCtrl.addToggle(&m_showRTImage,									FW_KEY_SPACE,	"Show Ray Tracer result (SPACE)" );
    m_commonCtrl.addSeparator();

	m_commonCtrl.addToggle((S32*)&m_shadingMode, Renderer::ShadingMode_Headlight,			FW_KEY_F1,    "Headlight shading (F1)");
	m_commonCtrl.addToggle((S32*)&m_shadingMode, Renderer::ShadingMode_AmbientOcclusion,	FW_KEY_F2,    "Ambient occlusion shading (F2)");
    m_commonCtrl.addSeparator();

	m_commonCtrl.addToggle((S32*)&m_samplingMode, Renderer::SamplingMode_Normal,			FW_KEY_N,    "AO Normal distribution sampling (N)");
	m_commonCtrl.addToggle((S32*)&m_samplingMode, Renderer::SamplingMode_Halton,			FW_KEY_H,    "AO Halton sequence sampling (H)");
    m_commonCtrl.addSeparator();

	m_commonCtrl.addToggle((S32*)&m_colorMode, Renderer::ColorMode_Off,			FW_KEY_F,    "AO Colors off (F)");
	m_commonCtrl.addToggle((S32*)&m_colorMode, Renderer::ColorMode_On,			FW_KEY_O,    "AO Colors on (C)");
	m_commonCtrl.addSeparator();

    m_commonCtrl.beginSliderStack();
    m_commonCtrl.addSlider(&m_numAORays, 1, 256, true, FW_KEY_NONE, FW_KEY_NONE, "AO rays= %d");
    m_commonCtrl.addSlider(&m_aoRayLength, 0.1f, 10.0f, false, FW_KEY_NONE, FW_KEY_NONE, "AO ray length= %.2f");
    m_commonCtrl.endSliderStack();


    m_window.setTitle("Assignment 1");
    m_window.addListener(this);
    m_window.addListener(&m_commonCtrl);

	// allocate image for ray tracer results
	m_rtImage = new Image( m_window.getSize(), ImageFormat::RGBA_Vec4f );
	m_renderer = new Renderer;

	// all state files fill be prefixed by this; enables loading/saving between debug and release builds
	m_commonCtrl.setStateFilePrefix( "state_assignment1_" );

    m_commonCtrl.loadState(m_commonCtrl.getStateFileName(1));
}

//------------------------------------------------------------------------

App::~App(void)
{
	delete m_rt;
	delete m_renderer;
    delete m_mesh;
	delete m_rtImage;
}

//------------------------------------------------------------------------

bool App::handleEvent(const Window::Event& ev)
{
    if (ev.type == Window::EventType_Close)
    {
        m_window.showModalMessage("Exiting...");
        delete this;
        return true;
    }

    Action action = m_action;
    m_action = Action_None;
    String name;
    Mat4f mat;

    switch (action)
    {
    case Action_None:
        break;

    case Action_LoadMesh:
        name = m_window.showFileLoadDialog("Load mesh", getMeshImportFilter());
        if (name.getLength())
            loadMesh(name);
        break;

    case Action_ReloadMesh:
        if (m_meshFileName.getLength())
            loadMesh(m_meshFileName);
        break;

    case Action_SaveMesh:
        name = m_window.showFileSaveDialog("Save mesh", getMeshExportFilter());
        if (name.getLength())
            saveMesh(name);
        break;

    case Action_ResetCamera:
        if (m_mesh)
        {
            m_cameraCtrl.initForMesh(m_mesh);
            m_commonCtrl.message("Camera reset");
        }
        break;

    case Action_EncodeCameraSignature:
        m_window.setVisible(false);
        printf("\nCamera signature:\n");
        printf("%s\n", m_cameraCtrl.encodeSignature().getPtr());
        waitKey();
        break;

    case Action_DecodeCameraSignature:
        {
            m_window.setVisible(false);
            printf("\nEnter camera signature:\n");

            char buf[1024];
            if (scanf_s("%s", buf, FW_ARRAY_SIZE(buf)))
                m_cameraCtrl.decodeSignature(buf);
            else
                setError("Signature too long!");

            if (!hasError())
                printf("Done.\n\n");
            else
            {
                printf("Error: %s\n", getError().getPtr());
                clearError();
                waitKey();
            }
        }
        break;

    case Action_NormalizeScale:
        if (m_mesh)
        {
            Vec3f lo, hi;
            m_mesh->getBBox(lo, hi);
            m_mesh->xform(Mat4f::scale(Vec3f(2.0f / (hi - lo).max())) * Mat4f::translate((lo + hi) * -0.5f));
        }
        break;

    case Action_FlipXY:
        nvswap(mat.col(0), mat.col(1));
        if (m_mesh)
        {
            m_mesh->xform(mat);
            m_mesh->flipTriangles();
        }
        break;

    case Action_FlipYZ:
        nvswap(mat.col(1), mat.col(2));
        if (m_mesh)
        {
            m_mesh->xform(mat);
            m_mesh->flipTriangles();
        }
        break;

    case Action_FlipZ:
        mat.col(2) = -mat.col(2);
        if (m_mesh)
        {
            m_mesh->xform(mat);
            m_mesh->flipTriangles();
        }
        break;

    case Action_NormalizeNormals:
        if (m_mesh)
            m_mesh->xformNormals(mat.getXYZ(), true);
        break;

    case Action_FlipNormals:
        mat = -mat;
        if (m_mesh)
            m_mesh->xformNormals(mat.getXYZ(), false);
        break;

    case Action_RecomputeNormals:
        if (m_mesh)
            m_mesh->recomputeNormals();
        break;

    case Action_FlipTriangles:
        if (m_mesh)
            m_mesh->flipTriangles();
        break;

    case Action_CleanMesh:
        if (m_mesh)
            m_mesh->clean();
        break;

    case Action_CollapseVertices:
        if (m_mesh)
            m_mesh->collapseVertices();
        break;

    case Action_DupVertsPerSubmesh:
        if (m_mesh)
            m_mesh->dupVertsPerSubmesh();
        break;

    case Action_FixMaterialColors:
        if (m_mesh)
            m_mesh->fixMaterialColors();
        break;

    case Action_DownscaleTextures:
        if (m_mesh)
            downscaleTextures(m_mesh);
        break;

    case Action_ChopBehindNear:
        if (m_mesh)
        {
            Mat4f worldToClip = m_cameraCtrl.getCameraToClip() * m_cameraCtrl.getWorldToCamera();
            Vec4f pleq = worldToClip.getRow(2) + worldToClip.getRow(3);
            chopBehindPlane(m_mesh, pleq);
        }
        break;

	// 
	case Action_TracePrimaryRays:
		{
			m_renderer->setAONumRays( m_numAORays );
			m_renderer->setAORayLength( m_aoRayLength );
			m_renderer->rayTracePicture( m_mesh, m_rt, m_rtImage, m_cameraCtrl, (Renderer::ShadingMode)m_shadingMode, (Renderer::SamplingMode)m_samplingMode, (Renderer::ColorMode)m_colorMode );
			m_showRTImage = true;
		}
		break;

    default:
        FW_ASSERT(false);
        break;
    }

    m_window.setVisible(true);

    if (ev.type == Window::EventType_Paint)
        renderFrame(m_window.getGL());
    m_window.repaint();
    return false;
}

//------------------------------------------------------------------------

void App::readState(StateDump& d)
{
    String meshFileName;

    d.pushOwner("App");
    d.get(meshFileName,     "m_meshFileName");
    d.get((S32&)m_cullMode, "m_cullMode");
    d.get((S32&)m_numAORays, "m_numAORays");
	d.get((F32&)m_aoRayLength, "m_aoRayLength");
    d.popOwner();

    if (m_meshFileName != meshFileName && meshFileName.getLength())
        loadMesh(meshFileName);
}

//------------------------------------------------------------------------

void App::writeState(StateDump& d) const
{
    d.pushOwner("App");
    d.set(m_meshFileName,       "m_meshFileName");
    d.set((S32)m_cullMode,      "m_cullMode");
    d.set((S32&)m_numAORays, "m_numAORays");
	d.set((F32&)m_aoRayLength, "m_aoRayLength");
    d.popOwner();
}

//------------------------------------------------------------------------

void App::waitKey(void)
{
    printf("Press any key to continue . . . ");
    _getch();
    printf("\n\n");
}

//------------------------------------------------------------------------

void App::renderFrame(GLContext* gl)
{
	// make sure the RT image is correct size. If not, reallocate
	if ( m_rtImage->getSize() != m_window.getSize() )
	{
		delete m_rtImage;
		m_rtImage = new Image( m_window.getSize(), ImageFormat::RGBA_Vec4f );
	}

	// if desired, show the ray traced result image
	if ( m_showRTImage )
	{
		glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// blit the image
		gl->drawImage( *m_rtImage, Vec2f(0,0) );

		m_commonCtrl.message(sprintf("%.2f Mrays/sec",m_renderer->getRaysPerSecond()/1000000.0f),"rayStats");
	}
	else
	{
		// nope, let's draw using OpenGL
		// Setup transformations.

		Mat4f worldToCamera = m_cameraCtrl.getWorldToCamera();
		Mat4f projection = gl->xformFitToView(Vec2f(-1.0f, -1.0f), Vec2f(2.0f, 2.0f)) * m_cameraCtrl.getCameraToClip();

		// Initialize GL state.

		glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		if (m_cullMode == CullMode_None)
			glDisable(GL_CULL_FACE);
		else
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace((m_cullMode == CullMode_CW) ? GL_CCW : GL_CW);
		}

		// No mesh => skip.

		if (!m_mesh)
		{
			gl->drawModalMessage("No mesh loaded!");
			return;
		}

		// Render.

		if (!gl->getConfig().isStereo)
			renderScene(gl, worldToCamera, projection);
		else
		{
			glDrawBuffer(GL_BACK_LEFT);
			renderScene(gl, m_cameraCtrl.getCameraToLeftEye() * worldToCamera, projection);
			glDrawBuffer(GL_BACK_RIGHT);
			glClear(GL_DEPTH_BUFFER_BIT);
			renderScene(gl, m_cameraCtrl.getCameraToRightEye() * worldToCamera, projection);
			glDrawBuffer(GL_BACK);
		}

		// Display status line.

		m_commonCtrl.message(sprintf("Triangles = %d, vertices = %d, materials = %d",
			m_mesh->numTriangles(),
			m_mesh->numVertices(),
			m_mesh->numSubmeshes()),
			"meshStats");
	}	// end GL draw code
}

//------------------------------------------------------------------------

void App::renderScene(GLContext* gl, const Mat4f& worldToCamera, const Mat4f& projection)
{
    // Draw mesh.
    if (m_mesh)
        m_mesh->draw(gl, worldToCamera, projection);
}

//------------------------------------------------------------------------

void App::loadMesh(const String& fileName)
{
    m_window.showModalMessage(sprintf("Loading mesh from '%s'...", fileName.getPtr()));
    String oldError = clearError();
    MeshBase* mesh = importMesh(fileName);
    String newError = getError();

    if (restoreError(oldError))
    {
        delete mesh;
        m_commonCtrl.message(sprintf("Error while loading '%s': %s", fileName.getPtr(), newError.getPtr()));
        return;
    }

    delete m_mesh;
    m_meshFileName = fileName;
    m_mesh = mesh;
    m_commonCtrl.message(sprintf("Loaded mesh from '%s'", fileName.getPtr()));

	// build the BVH!
	constructTracer();
}

//------------------------------------------------------------------------

void App::saveMesh(const String& fileName)
{
    if (!m_mesh)
    {
        m_commonCtrl.message("No mesh to save!");
        return;
    }

    m_window.showModalMessage(sprintf("Saving mesh to '%s'...", fileName.getPtr()));
    String oldError = clearError();
    exportMesh(fileName, m_mesh);
    String newError = getError();

    if (restoreError(oldError))
    {
        m_commonCtrl.message(sprintf("Error while saving '%s': %s", fileName.getPtr(), newError.getPtr()));
        return;
    }

    m_meshFileName = fileName;
    m_commonCtrl.message(sprintf("Saved mesh to '%s'", fileName.getPtr()));
}

//------------------------------------------------------------------------

// This function iterates over all the "submeshes" (parts of the object with different materials),
// heaps all the vertices and triangles together, and calls the BVH constructor.
// It is the responsibility of the tree to free the data when deleted.
// This functionality is _not_ part of the RayTracer class in order to keep it separate
// from the specifics of the Mesh class.
void App::constructTracer()
{
	// if we had a hierarchy, delete it.
	if ( m_rt != 0 )
		delete m_rt;

	// fetch vertex and triangle data ----->
	m_rtVertices.clear();
	m_rtVertices.reserve( m_mesh->numVertices() );
	m_rtTriangles.clear();
	m_rtTriangles.reserve( m_mesh->numTriangles() );
	m_rtMap.clear();
	m_rtMap.reserve( m_mesh->numTriangles() );
	for ( int i = 0; i < m_mesh->numVertices(); ++i )
	{
		Vec3f p = m_mesh->getVertexAttrib( i, FW::MeshBase::AttribType_Position ).getXYZ();
		m_rtVertices.push_back( p );
	}
	for ( int i = 0; i < m_mesh->numSubmeshes(); ++i )
	{
		const Array<Vec3i>& idx = m_mesh->indices( i );
		for ( int j = 0; j < idx.getSize(); ++j )
		{
			Renderer::RTToMesh m;
			m.submesh = i;
			m.tri_idx = j;
			m_rtMap.push_back( m );

			RTTriangle t;
			t.m_userPointer = 0;
			t.m_vertices[0] = &m_rtVertices[0] + idx[j][0];
			t.m_vertices[1] = &m_rtVertices[0] + idx[j][1];
			t.m_vertices[2] = &m_rtVertices[0] + idx[j][2];
			m_rtTriangles.push_back( t );
		}
	}
	FW_ASSERT( m_rtMap.size() == m_rtTriangles.size() );
	// fix the user pointers to point to the array that tells
	// which (submesh,tri) combination each linearly-indexed triangle corresponds to.
	for ( size_t i = 0; i < m_rtTriangles.size(); ++i )
		m_rtTriangles[ i ].m_userPointer = &m_rtMap[ i ];
	// <-------

	// compute checksum
	String md5 = RayTracer::computeMD5( m_rtVertices );
	FW::printf( "Mesh MD5: %s\n", md5.getPtr() );

	// ..
	m_rt = new RayTracer();

	const bool tryLoadHieararchy = false;



	if ( tryLoadHieararchy )
	{
		// check if saved hierarchy exists
		String hierarchyCacheFile = String("Hierarchy-") + md5 + String(".bin");
		if ( fileExists( hierarchyCacheFile.getPtr() ) )
		{
			// yes, load!
			m_rt->loadHierarchy( hierarchyCacheFile.getPtr(), m_rtTriangles );
			::printf( "Loaded hierarchy from %s\n", hierarchyCacheFile.getPtr() );
		}
		else
		{
			// no, construct...
			m_rt->constructHierarchy( m_rtTriangles );
			// .. and save!
			m_rt->saveHierarchy( hierarchyCacheFile.getPtr(), m_rtTriangles );
			::printf( "Saved hierarchy to %s\n", hierarchyCacheFile.getPtr() );
		}
	}
	else
	{
		// nope, bite the bullet and construct it
		m_rt->constructHierarchy( m_rtTriangles );
	}
}

//------------------------------------------------------------------------

void App::downscaleTextures(MeshBase* mesh)
{
    FW_ASSERT(mesh);
    Hash<const Image*, Texture> hash;
    for (int submeshIdx = 0; submeshIdx < mesh->numSubmeshes(); submeshIdx++)
    for (int textureIdx = 0; textureIdx < MeshBase::TextureType_Max; textureIdx++)
    {
        Texture& tex = mesh->material(submeshIdx).textures[textureIdx];
        if (tex.exists())
        {
            const Image* orig = tex.getImage();
            if (!hash.contains(orig))
            {
                Image* scaled = orig->downscale2x();
                hash.add(orig, (scaled) ? Texture(scaled, tex.getID()) : tex);
            }
            tex = hash.get(orig);
        }
    }
}

//------------------------------------------------------------------------

void App::chopBehindPlane(MeshBase* mesh, const Vec4f& pleq)
{
    FW_ASSERT(mesh);
    int posAttrib = mesh->findAttrib(MeshBase::AttribType_Position);
    if (posAttrib == -1)
        return;

    for (int submeshIdx = 0; submeshIdx < mesh->numSubmeshes(); submeshIdx++)
    {
        Array<Vec3i>& inds = mesh->mutableIndices(submeshIdx);
        int triOut = 0;
        for (int triIn = 0; triIn < inds.getSize(); triIn++)
        {
            if (dot(mesh->getVertexAttrib(inds[triIn].x, posAttrib), pleq) >= 0.0f ||
                dot(mesh->getVertexAttrib(inds[triIn].y, posAttrib), pleq) >= 0.0f ||
                dot(mesh->getVertexAttrib(inds[triIn].z, posAttrib), pleq) >= 0.0f)
            {
                inds[triOut++] = inds[triIn];
            }
        }
        inds.resize(triOut);
    }

    mesh->clean();
}

//------------------------------------------------------------------------

void FW::init(void)
{
    new App;
}

//------------------------------------------------------------------------

bool App::fileExists( const String& fn )
{
	FILE* pF = fopen( fn.getPtr(), "rb" );
	if( pF != 0 )
	{
		fclose( pF );
		return true;
	}
	else
	{
		return false;
	}
}

//------------------------------------------------------------------------
