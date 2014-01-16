#include "ShadowMap.hpp"

namespace FW 
{


Mat4f LightSource::getPosToLightClip() const 
{
	// YOUR CODE HERE
	// You'll need to construct the matrix that sends world space points to
	// points in the light source's clip space. You'll need to somehow use
	// m_xform, which describes the light source pose, and Mat4f::perspective().
	return Mat4f::perspective(this->getFOV(), this->getNear(), this->getFar()) * m_xform.inverted();
}


void LightSource::renderShadowedScene(GLContext *gl, MeshWithColors *scene, const Mat4f& worldToCamera, const Mat4f& projection, bool fromLight)
{
	// Get or build the shader that renders the scene using the shadow map
	GLContext::Program *prog = gl->getProgram("MeshBase::draw_generic");
	if (!prog)
	{
		printf("Compiling MeshBase::draw_generic\n");
	    prog = new GLContext::Program(
			"#version 120\n"
			FW_GL_SHADER_SOURCE(
				// VERTEX SHADER BEGINS HERE. This part is executed for every vertex in the scene. The output
				// is the bunch of variables with the 'varying' qualifier; these values will be interpolated across triangles, and passed
				// to the fragment shader (which is below this one). The vertex shader also needs to fill in the
				// variable gl_Position, which determines where on the screen the vertex will be drawn. See below
				// for how that's done.

				// Boilerplate stuff, no need to touch these first lines, but it might be useful to understand
				// what they do because you'll be implementing similar things:
				uniform mat4 posToClip;			// projection * view of _camera_
				uniform mat4 posToCamera;		// view matrix
				uniform mat3 normalToCamera;
				attribute vec3 positionAttrib;	// world space coordinates of this vertex
				attribute vec3 normalAttrib;
				attribute vec4 vcolorAttrib; // Workaround. "colorAttrib" appears to confuse certain ATI drivers.
				attribute vec2 texCoordAttrib;
				centroid varying vec3 positionVarying;
				centroid varying vec3 normalVarying;
				centroid varying vec4 colorVarying;
				varying vec2 texCoordVarying;
				uniform bool renderFromLight;

				// Here starts the part you'll need to be concerned with.
				// The uniform variables are the "inputs" to the shader. They are filled in prior to drawing
				// using setUniform() calls (see the end of the renderShadowedScene() function).

				// The transformation to light's clip space.
				// Notice the similarity to the posToClip above.
				uniform mat4 posToLightClip;
				
				// This 'varying' stuff below will be filled in by your code in the main() function of the vertex shader. 

				// Your code will need to compute the depth from the light's point of view; it will be placed
				// in the following variable, and compared to the shadow map value in the fragment shader:
				varying float shadowDepth;
				// Similarly, you'll need to compute the shadow map UV coordinates (in the range [0,1]x[0,1]) that correspond to this point:
				varying vec2 shadowUV;
				
				// Feel free to define whatever else you might need. Use uniforms to bring in stuff from the caller ("C++ side"),
				// and varyings to pass computation results to the fragment shader.

				void main()
				{
					// vec3 positionAttrib contains the world space coordinates of this vertex. Make a homogeneous version:
					vec4 pos = vec4(positionAttrib, 1.0);
					// Next we'll fulfill the main duty of the vertex shader by assigning the gl_Position variable. The value
					// must be the camera clip space coordinates of the vertex. In the notation of the instructions, this is
					// the computation c = P * V * p, because posToClip contains the readily multiplied projection and
					// view matrices. So in other words the following line corresponds to something like 
					// gl_Position = cameraProjectionMatrix * cameraViewMatrix * pos;
					gl_Position = posToClip * pos;

					// Then do some other stuff that might be useful (texture coordinate interpolation, etc.)
					positionVarying = (posToCamera * pos).xyz;
					normalVarying = normalToCamera * normalAttrib;
					colorVarying = vcolorAttrib;
					texCoordVarying = texCoordAttrib;

					// YOUR CODE HERE: Compute shadowDepth and shadowUV.
					// Compute the shadowDepth using posLightClip in the same manner you used it in the depth map shader,
					// i.e. transform to NDC coordinates and use the suitable components.
					// Note that the typical transformation procedure gives you values in the rectangle
					// [-1,1]x[-1,1], but UV coordinates are scaled to the rectangle [0,1]x[0,1], so you'll need to
					// transform them a bit in the end.
					vec4 posLightClip = posToLightClip * pos;
					
					float w_inv = 1.0f/posLightClip.w;
					shadowDepth = posLightClip.z * w_inv;
					shadowUV = ((posLightClip.xy * w_inv) * 0.5f) + 0.5f;

					// Debug hack that renders the scene from the light's view, activated by a toggle button in the UI:
					if (renderFromLight)
						gl_Position = posLightClip;//posToLightClip * pos;
					
				}
				// VERTEX SHADER ENDS HERE
			),
			"#version 120\n"
			FW_GL_SHADER_SOURCE(
				// FRAGMENT SHADER BEGINS HERE. This part is executed for every pixel after the vertex shader is done.
				// The ultimate task of the fragment shader is to assign a value to vec4 gl_FragColor. That value
				// will be written to the screen (or wherever we're drawing, possibly into a texture).

				// Boilerplate, no need to touch:
				uniform bool hasNormals;
				uniform bool hasDiffuseTexture;
				uniform bool hasAlphaTexture;
				uniform vec4 diffuseUniform;
				uniform vec3 specularUniform;
				uniform float glossiness;
				uniform sampler2D diffuseSampler;
				uniform sampler2D alphaSampler;
				centroid varying vec3 positionVarying;
				centroid varying vec3 normalVarying;
				centroid varying vec4 colorVarying;
				varying vec2 texCoordVarying;

				// Some light-related uniforms that were fed in to the shader from the caller.
				uniform float lightFOVRad;
				uniform vec3 lightPosEye;
				uniform vec3 lightDirEye;
				uniform vec3 lightE;

				// Contains the texture unit that has the shadow map:
				uniform sampler2D shadowSampler;

				// Interpolated varying variables. Note that the names correspond to those in the vertex shader.
				// So whatever you computed and assigned to these in the vertex shader, it's now here:
				varying float shadowDepth;	
				varying vec2 shadowUV;
				

				void main()
				{
					// Read the basic surface properties:
					vec4 diffuseColor = diffuseUniform;
					vec3 specularColor = specularUniform;

					if (hasDiffuseTexture)
						diffuseColor.rgb = texture2D(diffuseSampler, texCoordVarying).rgb;

					diffuseColor *= colorVarying;

					if (hasAlphaTexture)
						diffuseColor.a = texture2D(alphaSampler, texCoordVarying).g;

					if (diffuseColor.a <= 0.5)
						discard;
					// Basic stuff done.


					// YOUR CODE HERE: Compute diffuse shading and the spot light cone.
					// The uniforms lightPosEye and lightDirEye contain the light position and direction in eye coordinates.
					// The varyings positionVarying and normalVarying contain the point position and normal in eye coordinates.
					// Your task is to compute the incoming vs surface normal cosine (diffuse shading), incoming vs light direction 
					// cosine (because we're assuming cosine distribution within the cone), and inverse square distance from
					// the light. Multiply these all together with lightE (light intensity), and that's the shading.
					//
					// Figure out a way to determine if we're outside the spot light cone. If so, set the color
					// to black. Preferably you should make it fade to black smoothly towards the edges, because
					// otherwise the image will be full of nasty rough edges. The cone opening angle is given by the
					// lightFOVRad uniform.
					// Be careful with the lightFOVRad -- it contains the full angle of opening,
					// which is twice the angle of the cone against its axis. Often times you want the latter.

					// Calculate the incoming direction
					vec3 incomingDir = normalize(positionVarying - lightPosEye);

					// Calculate the incoming vs. surface normal cosine
					float incomingVsSurfaceNormal = max(0.0f, dot(-incomingDir, normalize(normalVarying)));
					
					// Calculate the incoming vs. light direction cosine
					float incomingVsLightDir = max(0.0f, dot(incomingDir, normalize(lightDirEye)));
					
					// Calculate the inverse square distance from the light
					float dist = length(positionVarying - lightPosEye);
					// Should clamp this to be at max 10.0f (see the instructions)
					float invDist = min( 10.0f, 1.0f / (dist * dist) );

					float lightFOVRadDIV2COS = cos(lightFOVRad/2.0f);
					vec3 shading = incomingVsSurfaceNormal * incomingVsLightDir * invDist * lightE;
					float cone = max( 0.0f, min( 1.0f, 4.0f * (incomingVsLightDir-lightFOVRadDIV2COS)/(1.0f-lightFOVRadDIV2COS) ) );

					// YOUR CODE HERE:
					// This is the place where you need to fetch the shadow map value in the position you computed,
					// and compare it with the depth you computed. The shadow map texture is in the sampler variable
					// shadowSampler. Use GLSL function texture2D to fetch the value, and compare it to shadowDepth
					// to determine whether this pixel is shadowed or not. If it is, set the color to zero.
					float shadow = texture2D(shadowSampler, shadowUV).x < shadowDepth ? 0.0f : 1.0f;
					diffuseColor.rgb *= shading * shadow * cone * (1.0f/3.1415926f);

					// Set the alpha value to 1.0, we're not using it for anything and other values might or might not
					// cause weird things.
					diffuseColor.a = 1.0;
					// Finally, assign the color to the pixel.
					gl_FragColor = diffuseColor;
				}
				// FRAGMENT SHADER ENDS HERE
			));

		gl->setProgram( "MeshBase::draw_generic", prog );
	}
	prog->use();
	
	// Attach the shadow map to the texture slot 2 and tell the shader about it by setting a uniform
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture);
	gl->setUniform(prog->getUniformLoc("shadowSampler"), 2);

	// Set the input parameters to the shader

	// Here we set the light source transformation according to your getPosToLightClip() implementation:
	gl->setUniform(prog->getUniformLoc("posToLightClip"), getPosToLightClip());
	// Other light source parameters:
	gl->setUniform(prog->getUniformLoc("lightPosEye"), (worldToCamera*Vec4f(getPosition(),1.0f)).getXYZ()); // Transformed to eye coords
	gl->setUniform(prog->getUniformLoc("lightDirEye"), (worldToCamera*Vec4f(getNormal(),0.0f)).getXYZ());
	gl->setUniform(prog->getUniformLoc("lightE"), getEmission());
	gl->setUniform(prog->getUniformLoc("lightFOVRad"), getFOVRad());
	gl->setUniform(prog->getUniformLoc("renderFromLight"), fromLight);

	scene->draw(gl, worldToCamera, projection);	

	glUseProgram(0);
}



void LightSource::renderShadowMap(FW::GLContext *gl, MeshWithColors *scene, ShadowMapContext *sm, bool debug)
{
	// Get or build the shader that outputs depth values
    static const char* progId = "ShadowMap::renderDepthTexture";
    GLContext::Program* prog = gl->getProgram(progId);
	if (!prog)
	{
		printf("Creating shadow map program\n");

		prog = new GLContext::Program(
			FW_GL_SHADER_SOURCE(
			
				// Have a look at the comments in the main shader in the renderShadowedScene() function in case
				// you are unfamiliar with GLSL shaders.

				// This is the shader that will render the scene from the light source's point of view, and
				// output the depth value at every pixel.

				// VERTEX SHADER

				// The posToLightClip uniform will be used to transform the world coordinate points of the scene.
				// They should end up in the clip coordinates of the light source "camera". If you look below,
				// you'll see that its value is set based on the function getWorldToLightClip(), which you'll
				// need to implement.
				uniform mat4	posToLightClip;
				attribute vec3	positionAttrib;
				varying float depthVarying;

				void main()
				{
					// Here we perform the transformation from world to light clip space.
					// If the posToLightClip is set correctly (see the corresponding setUniform()
					// call below), the scene will be viewed from the light.
					vec4 pos = vec4(positionAttrib, 1.0);
					gl_Position = posToLightClip * pos;
				
					// YOUR CODE HERE: compute the depth of this vertex and assign it to depthVarying.
					// See the instructions and the discussion on the NDC coordinates.

					// z_ndc = z_clip / w_clip
					depthVarying = gl_Position.z/gl_Position.w;

				}
			),
			FW_GL_SHADER_SOURCE(
				// FRAGMENT SHADER
				varying float depthVarying;
				void main()
				{
					// Just output the depth value you computed in the vertex shader. It'll go into the shadow
					// map texture.
					gl_FragColor = vec4(depthVarying);
				}
			));

		gl->setProgram(progId, prog);
	}
    prog->use();


	// If this light source does not yet have an allocated OpenGL texture, then get one
	if (m_shadowMapTexture == 0)
		m_shadowMapTexture = sm->allocateDepthTexture();

	// Attach the shadow rendering state (off-screen buffers, render target texture)
	sm->attach(gl, m_shadowMapTexture);

	// Here's a trick: we can cull the front faces instead of the backfaces; this
	// moves the shadow map bias nastiness problems to the dark side of the objects
	// where they might not be visible.
	// You may want to comment these lines during debugging, as the front-culled
	// shadow map may look a bit weird.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// Clear the texture to maximum distance (1.0 in NDC coordinates)
	glClearColor(1.0f, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Set the transformation matrix uniform
	gl->setUniform(prog->getUniformLoc("posToLightClip"), getPosToLightClip());

	renderSceneRaw(gl, scene, prog);

	GLContext::checkErrors();

	// Detach off-screen buffers
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


Mat3f LightSource::formBasis(const Vec3f& n) 
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

void LightSource::sampleEmittedRays(int num, std::vector<Vec3f>& origs, std::vector<Vec3f>& dirs, std::vector<Vec3f>& E_times_pdf) const
{
	Random rand(1234);	// Use this random number generator, so that we'll get the same rays on every frame. Otherwise it'll flicker.

	// YOUR CODE HERE
	// Fill the three vectors with #num ray origins, directions, and intensities multiplied by probability density.
	// See the instructions.

	Mat3f rotationMatrix = LightSource::formBasis(getNormal());
	float r = sin(getFOVRad()*0.5f);
	
	// Pre-allocate space in the output vectors (just a slight optimization)
	// For now the emissions and origins are always the same for all the light sources
	// The origin and emission are always the same
	origs.assign(num, getPosition());
	E_times_pdf.assign(num, (getEmission()/num) * (r*r));
	dirs.resize(num);
	
	for (int i = 0; i < num; ++i) {

		// Pick a random point from a unit circle using rejection sampling
		Vec3f u(0.0f);

		do {
			u.x = rand.getF32(-1.0f, 1.0f);
			u.y = rand.getF32(-1.0f, 1.0f);
		} while ( (u.x*u.x + u.y*u.y) > 1.0f );

		// Scale the coordinates
		u *= r;

		// Lift the point vertically onto the 3D unit sphere by setting z = sqrt(1 - x^2 - y^2)
		u.z = sqrt(1.0f - (u.x*u.x) - (u.y*u.y));

		// Calculate the direction
		// Remember to scale by a factor of r (see instructions)
		dirs[i] = ( rotationMatrix * u * getFar() );
	}
}





/*************************************************************************************
 * Nasty OpenGL stuff begins here, you should not need to touch the remainder of this
 * file for the basic requirements.
 *
 */

bool ShadowMapContext::setup(Vec2i resolution)
{
	m_resolution = resolution;

	printf("Setting up ShadowMapContext... ");
	free();	// Delete the existing stuff, if any
	
	glGenRenderbuffers(1, &m_depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_resolution.x, m_resolution.y);
	GLContext::checkErrors();

	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
	
	GLContext::checkErrors();

	// We now have a framebuffer object with a renderbuffer attachment for depth.
	// We'll render the actual depth maps into separate textures, which are attached
	// to the color buffers upon rendering.

	// Don't attach yet.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("done\n");

	return true;
}

GLuint ShadowMapContext::allocateDepthTexture()
{
	printf("Allocating depth texture... ");

	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_resolution.x, m_resolution.y,	0, GL_LUMINANCE, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	printf("done, %i\n", tex);
	return tex;
}


void ShadowMapContext::attach(GLContext *gl, GLuint texture)
{
	if (m_framebuffer == 0)
	{
		printf("Error: ShadowMapContext not initialized\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glViewport(0, 0, m_resolution.x, m_resolution.y);
}



void LightSource::renderSceneRaw(FW::GLContext *gl, MeshWithColors *scene, GLContext::Program *prog)
{
	// Just draw the triangles without doing anything fancy; anything else should be done on the caller's side

	int posAttrib = scene->findAttrib(MeshBase::AttribType_Position);
	if (posAttrib == -1)
		return;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->getVBO().getGLBuffer());
	scene->setGLAttrib(gl, posAttrib, prog->getAttribLoc("positionAttrib"));

	for (int i = 0; i < scene->numSubmeshes(); i++)
	{
		glDrawElements(GL_TRIANGLES, scene->vboIndexSize(i), GL_UNSIGNED_INT, (void*)(UPTR)scene->vboIndexOffset(i));
	}

	gl->resetAttribs();
}


void LightSource::draw(const Mat4f& worldToCamera, const Mat4f& projection, bool show_axis, bool show_frame, bool show_square)
{
	glUseProgram(0);
	if (show_axis)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((float*)&projection);
		glMatrixMode(GL_MODELVIEW);
	//	Mat4f S = Mat4f::scale(Vec3f(m_size,1));
		Mat4f M = worldToCamera * m_xform;
		glLoadMatrixf((float*)&M);

		const float orig[3] = {0,0,0};
 
		const float 
		  XP[3] = {1,0,0},
		  YP[3] = {0,1,0},
		  ZP[3] = {0,0,1};
	
		glLineWidth (3.0);
 
		glBegin(GL_LINES);
			  glColor3f (1,0,0);  glVertex3fv (orig);  glVertex3fv (XP);
			  glColor3f (0,1,0);  glVertex3fv (orig);  glVertex3fv (YP);
			  glColor3f (0,0,1);  glVertex3fv (orig);  glVertex3fv (ZP);
		glEnd();
	}

	if (show_frame)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((float*)&projection);
		glMatrixMode(GL_MODELVIEW);
	//	Mat4f S = Mat4f::scale(Vec3f(m_size,1));
		Mat4f M = worldToCamera * m_xform * Mat4f::scale(Vec3f(tan(m_fov*3.14159/360), tan(m_fov*3.14159/360),1));
		glLoadMatrixf((float*)&M);
		glLineWidth (1.0);

		glBegin(GL_LINES);
			  glColor3f (1,1,1);
			  glVertex3f(-1, -1, -1);
			  glVertex3f(1, -1, -1);
			  glVertex3f(1, -1, -1);
			  glVertex3f(1, 1, -1);
			  glVertex3f(1, 1, -1);
			  glVertex3f(-1, 1, -1);
			  glVertex3f(-1, 1, -1);
			  glVertex3f(-1, -1, -1);

			  glVertex3f(-1, -1, -1);
			  glVertex3f(0,0,0);
			  glVertex3f(1, -1, -1);
			  glVertex3f(0,0,0);
			  glVertex3f(1, 1, -1);
			  glVertex3f(0,0,0);
			  glVertex3f(-1, 1, -1);
			  glVertex3f(0,0,0);
		glEnd();

	}

	if (show_square)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((float*)&projection);
		glMatrixMode(GL_MODELVIEW);
		Mat4f S = Mat4f::scale(Vec3f(m_size,1));
		Mat4f M = worldToCamera * m_xform * S;
		glLoadMatrixf((float*)&M);
		glLineWidth (1.0);

		glBegin(GL_TRIANGLES);
		glColor3fv( &m_E.x );
		glVertex3f(1,1,0); glVertex3f(1,-1,0); glVertex3f( -1,-1,0 );
		glVertex3f(1,1,0); glVertex3f( -1,-1,0 ); glVertex3f(-1,1,0); 
		glEnd();
	}

}

}