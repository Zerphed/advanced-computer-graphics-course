#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "bunny.hh"
#include "utils.hh"
#include "plyparser.hh"
#include "rgbparser.hh"

#define WINDOW_TITLE_PREFIX "Standford Bunny in OpenGL"

typedef struct LightSource_s {
	GLfloat position[4];
	GLfloat direction[3];
	GLfloat color[4];
	GLfloat spotCutoff;
	GLfloat spotCoeff;
	GLuint type;
} LightSource;

typedef struct Material_s {
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat specularity;
} Material;

int currentWidth = 600,
	currentHeight = 600,
	windowHandle = 0;

GLint throbbing = 0;

unsigned frameCount = 0;

int lineNumber = 0, numOfVertices = 0, numOfFaces = 0;

GLuint
	projectionMatrixUniformLocation,
	viewMatrixUniformLocation,
	modelMatrixUniformLocation,
	
	lightUniformLocation,
	materialUniformLocation,

	textureUniformLocation,
	normalMapUniformLocation,

	currentProgramId,

	bufferIds[3] = {0};

GLint timeUniformLocation;

GLuint
	*vertexShaderIds,
	*fragmentShaderIds,
	*shaderIds;

Matrix
	projectionMatrix,
	viewMatrix,
	modelMatrix;

static const char* vertexShaders[] = {"SimpleShader.vertex.glsl", "ToonShader.vertex.glsl", "ProceduralTextureShader.vertex.glsl", "NormalMapShader.vertex.glsl"};
static const char* fragmentShaders[] = {"SimpleShader.fragment.glsl", "ToonShader.fragment.glsl", "ProceduralTextureShader.fragment.glsl", "NormalMapShader.fragment.glsl"};

static int numOfShaders = sizeof(vertexShaders) / sizeof(char*);

float rotation = 0;
clock_t lastTime;

void initialize(int, char*[]);
void initWindow(int, char*[]);

void resizeFunction(int, int);
void renderFunction(void);
void timerFunction(int);
void idleFunction(void);

void keyboardFunction(unsigned char, int, int);
void specialKeyboardFunction(int key, int x, int y);
void specialKeyboardUpFunction(int key, int x, int y);

void mouseFunction(int button, int state, int x, int y);
void mouseMoveFunction(int x, int y);
Vector getArcballVector(int x, int y);

void createBunny(void);
void destroyBunny(void);
void drawBunny(void);

void generateUVMapping(Vertex* vertices, Face* faces, int** neighbourMap, int numOfVertices, int numOfFaces);

void buildShaders(void);

void enableLights(GLuint programId);
void enableTextures(GLuint programId);
void enableNormalMapTextures(GLuint programId);

int main(int argc, char* argv[])
{
	initialize(argc, argv);

	glutMainLoop();
	
	exit(EXIT_SUCCESS);
}

void initialize(int argc, char* argv[])
{
	GLenum glewInitResult;

	initWindow(argc, argv);

	glewExperimental = GL_TRUE;
	glewInitResult = glewInit();

	if (GLEW_OK != glewInitResult) {
		fprintf(stderr, "ERROR: %s\n", glewGetErrorString(glewInitResult));
		exit(EXIT_FAILURE);
	}
	
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));

	glGetError();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	exitOnGLError("ERROR: Could not set OpenGL depth testing options");

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	exitOnGLError("ERROR: Could not set OpenGL culling options");

	modelMatrix = IDENTITY_MATRIX;
	projectionMatrix = IDENTITY_MATRIX;
	viewMatrix = IDENTITY_MATRIX;

	translateMatrix(&viewMatrix, 0, 0, -1.0f);

	createBunny();
}

void initWindow(int argc, char* argv[])
{
	glutInit(&argc, argv);
	
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutSetOption(
		GLUT_ACTION_ON_WINDOW_CLOSE,
		GLUT_ACTION_GLUTMAINLOOP_RETURNS
	);
	
	glutInitWindowSize(currentWidth, currentHeight);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	windowHandle = glutCreateWindow(WINDOW_TITLE_PREFIX);

	if(windowHandle < 1) {
		fprintf(stderr, "ERROR: Could not create a new rendering window.\n");
		exit(EXIT_FAILURE);
	}
	
	glutReshapeFunc(resizeFunction);
	glutDisplayFunc(renderFunction);
	glutIdleFunc(idleFunction);
	glutTimerFunc(0, timerFunction, 0);
	glutCloseFunc(destroyBunny);

	glutKeyboardFunc(keyboardFunction);
	glutSpecialFunc(specialKeyboardFunction);
	glutSpecialUpFunc(specialKeyboardUpFunction);

	glutMouseFunc(mouseFunction);
	glutMotionFunc(mouseMoveFunction);
}

GLuint textureId[4];
GLuint normalMapTextureId[3];
bool wireframe = false;
void keyboardFunction(unsigned char key, int x, int y) {
	const static int numOfTextures = sizeof(textureId)/sizeof(GLuint);
	static int i = 0, j = 0;
	
	switch (toupper(key)) {
		case 'W':
			if (!wireframe) {
				fprintf(stderr, "Switching to wireframe mode\n");
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else {
				fprintf(stderr, "Switching to fill mode\n");
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			wireframe = !wireframe;
			break;
		case 'T':
			
			if (currentProgramId == shaderIds[numOfShaders-1]) {
				fprintf(stderr, "Switching texture to normal map texture %d\n", normalMapTextureId[0]);
				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

				textureUniformLocation = glGetUniformLocation(currentProgramId, "textureImage");
				normalMapUniformLocation = glGetUniformLocation(currentProgramId, "normalMapImage");

				glProgramUniform1i(currentProgramId, textureUniformLocation, ++i%numOfTextures);
				glProgramUniform1i(currentProgramId, normalMapUniformLocation, numOfTextures);

				glActiveTexture(GL_TEXTURE0+(i%numOfTextures));
				glBindTexture(GL_TEXTURE_CUBE_MAP, textureId[i%numOfTextures]);
				glActiveTexture(GL_TEXTURE0+numOfTextures);
				glBindTexture(GL_TEXTURE_CUBE_MAP, normalMapTextureId[0]);
			}
			else {
				fprintf(stderr, "Switching texture to %d\n", ++i%numOfTextures);
				textureUniformLocation = glGetUniformLocation(currentProgramId, "textureImage");
				glProgramUniform1i(currentProgramId, textureUniformLocation, i%numOfTextures);
				glActiveTexture(GL_TEXTURE0+(i%numOfTextures));
				glBindTexture(GL_TEXTURE_CUBE_MAP, textureId[i%numOfTextures]);
			}
			break;
		case 'E':
			fprintf(stderr, "Switching throbbing effect state\n");
			throbbing = ++throbbing % 2;
			break;
		case 'S':
			currentProgramId = shaderIds[++j%numOfShaders];
			fprintf(stderr, "Switching shaders, currentProgramId: %d\n", (int)currentProgramId);

			j = j%numOfShaders;
			glUseProgram(shaderIds[j]);

			if (j == numOfShaders-1) {
				fprintf(stderr, "Switching texture to normal map texture %d\n", normalMapTextureId[0]);
				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
				normalMapUniformLocation = glGetUniformLocation(currentProgramId, "normalMapImage");

				glProgramUniform1i(currentProgramId, normalMapUniformLocation, numOfTextures);

				glActiveTexture(GL_TEXTURE0+numOfTextures);
				glBindTexture(GL_TEXTURE_CUBE_MAP, normalMapTextureId[0]);
			}

			textureUniformLocation = glGetUniformLocation(currentProgramId, "textureImage");
			glProgramUniform1i(currentProgramId, textureUniformLocation, i%numOfTextures);
			glActiveTexture(GL_TEXTURE0+(i%numOfTextures));
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureId[i%numOfTextures]);

			modelMatrixUniformLocation = glGetUniformLocation(shaderIds[j], "modelMatrix");
			viewMatrixUniformLocation = glGetUniformLocation(shaderIds[j], "viewMatrix");
			projectionMatrixUniformLocation = glGetUniformLocation(shaderIds[j], "projectionMatrix");
			glProgramUniformMatrix4fv(currentProgramId, projectionMatrixUniformLocation, 1, GL_FALSE, projectionMatrix.m);

			exitOnGLError("ERROR: Could not get the shader uniform locations for matrices\n");

			glUseProgram(0);
			break;
		case '+':
			fprintf(stderr, "Scaling up\n");
			scaleMatrix(&modelMatrix, 1.5f, 1.5f, 1.5f);
			break;
		case '-':
			fprintf(stderr, "Scaling down\n");
			scaleMatrix(&modelMatrix, 0.666f, 0.666f, 0.666f);
			break;
		// ESC
		case 27:
			fprintf(stderr, "Exiting the program, destroying the bunny\n");
			glutLeaveMainLoop();
			fprintf(stderr, "All done, bye\n");
			glutExit();
			break;
		default:
			fprintf(stderr, "Key event: %c\n", toupper(key));
			break;
	}
}

enum {
	GLUT_SCROLL_UP = 3,
	GLUT_SCROLL_DOWN = 4,
	GLUT_KEY_CTRL = 114
};

void specialKeyboardFunction(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_LEFT:
			translateMatrix(&viewMatrix, 0.01f, 0.0f, 0.0f);
			break;
		case GLUT_KEY_RIGHT:
			translateMatrix(&viewMatrix, -0.01f, 0.0f, 0.0f);
			break;
		case GLUT_KEY_UP:
			translateMatrix(&viewMatrix, 0.0f, -0.01f, 0.0f);
			break;
		case GLUT_KEY_DOWN:
			translateMatrix(&viewMatrix, 0.0f, 0.01f, 0.0f);
			break;
		default:
			fprintf(stderr, "Special down key event: %d\n", key);
			break;
	}
}

void specialKeyboardUpFunction(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_CTRL:
			fprintf(stderr, "CTRL pressed\n", key);
			break;
		default:
			fprintf(stderr, "Special up key event: %d\n", key);
			break;
	}
}

int lastMouseX = 0, lastMouseY = 0, currentMouseX = 0, currentMouseY = 0;
bool mousePressed = false, leftPressed = false, rightPressed = false;

void mouseFunction(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON) && state == GLUT_DOWN) {
		mousePressed = true;
		if (button == GLUT_LEFT_BUTTON)
			leftPressed = true;
		else
			rightPressed = true;

		lastMouseX = currentMouseX = x;
		lastMouseY = currentMouseY = y;
		fprintf(stderr, "Current mouse coordinates x = %d, y = %d, last: x = %d, y = %d\n", currentMouseX, currentMouseY, lastMouseX, lastMouseY);
	}
	else if (button == GLUT_SCROLL_UP) {
		translateMatrix(&viewMatrix, 0.0f, 0.0f, ABS(viewMatrix.m[14]) * 0.03f);
		fprintf(stderr, "Scrolling up - zooming in\n");
	}
	else if (button == GLUT_SCROLL_DOWN) {
		translateMatrix(&viewMatrix, 0.0f, 0.0f, -0.05f);
		fprintf(stderr, "Scrolling down - zooming out\n");
	}
	else {
		mousePressed = leftPressed = rightPressed = false;
	}
}

// Rotates the model around the origin
void rotateModel(void) {
	Vector va = getArcballVector(lastMouseX, lastMouseY);
	Vector vb = getArcballVector(currentMouseX, currentMouseY);

	float angle = (float)acos(MIN(1.0, dotProduct(&va, &vb)));

	if (angle < 0.0009) return;

	float modelTranslations[3] = {modelMatrix.m[12], modelMatrix.m[13], modelMatrix.m[14]};

	Vector axisInCameraCoord = crossProduct(&va, &vb);
	axisInCameraCoord.coord[3]=0.0f;
	normalize(&axisInCameraCoord);
			
	Matrix viewInverse = inverseMatrix(&viewMatrix);
	Vector axisInGlobalCoord = multiplyMatrixVector(&viewInverse, &axisInCameraCoord);
	normalize(&axisInGlobalCoord);

	translateMatrix(&modelMatrix, -modelTranslations[0], -modelTranslations[1], -modelTranslations[2]);
	rotateAboutAxis(&modelMatrix, &axisInGlobalCoord, angle);
	translateMatrix(&modelMatrix, modelTranslations[0], modelTranslations[1], modelTranslations[2]);
	fprintf(stderr, "Rotating the object, angle: %f\n", angle);
}

// Rotates the camera around the model
void rotateCamera(void) {
	Vector va = getArcballVector(lastMouseX, lastMouseY);
	Vector vb = getArcballVector(currentMouseX, currentMouseY);
			
	float angle = (float)acos(MIN(1.0, dotProduct(&va, &vb)));

	if (angle < 0.0009) return;

	Vector axisInCameraCoord = crossProduct(&va, &vb);
	//float angle = dotProduct(&axisInCameraCoord, &axisInCameraCoord);
	axisInCameraCoord.coord[3]=0.0f;
	normalize(&axisInCameraCoord);

	Matrix inv = inverseMatrix(&viewMatrix);
	Vector axisInGlobalCoord = multiplyMatrixVector(&inv, &axisInCameraCoord);
	normalize(&axisInGlobalCoord);

	rotateAboutAxis(&inv, &axisInGlobalCoord,angle);
	viewMatrix = inverseMatrix(&inv);

	fprintf(stderr, "Rotating the camera, angle: %f\n", angle);
}

void mouseMoveFunction(int x, int y) {
	if (mousePressed) {

		currentMouseX = x;
		currentMouseY = y;

		fprintf(stderr, "Mouse coordinates x = %d, y = %d\n", currentMouseX, currentMouseY);

		if ((currentMouseX != lastMouseX || currentMouseY != lastMouseY)) {
			if (leftPressed)
				rotateModel();
			else
				rotateCamera();
		}

		lastMouseX = currentMouseX;
		lastMouseY = currentMouseY;
	}
}

Vector getArcballVector(int x, int y) {
	// Initialize the OP vector
	Vector OPVector = {(float)x / currentWidth*2.0f - 1.0f, (float)y / currentHeight*2.0f - 1.0f, 0.0f, 1.0f};

	OPVector.coord[1] = -OPVector.coord[1];
	
	float OPSquared = OPVector.coord[0] * OPVector.coord[0] + OPVector.coord[1] * OPVector.coord[1];
	if (OPSquared <= 1.0f)
		OPVector.coord[2] = sqrt(1.0f - OPSquared);
	else
		normalize(&OPVector);

	return OPVector;
}

void resizeFunction(int width, int height)
{
	currentWidth = width;
	currentHeight = height;

	glViewport(0, 0, currentWidth, currentHeight);

	projectionMatrix = createProjectionMatrix(60, (float)currentWidth / currentHeight, 0.1f, 100.0f);
	glUseProgram(currentProgramId);
	glProgramUniformMatrix4fv(currentProgramId, projectionMatrixUniformLocation, 1, GL_FALSE, projectionMatrix.m);
	glUseProgram(0);
}

void renderFunction(void)
{
	++frameCount;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawBunny();

	glutSwapBuffers();
	glutPostRedisplay();
}

void idleFunction(void)
{
	glutPostRedisplay();
}

void timerFunction(int value)
{
	if (0 != value) {
		char* tempString = (char*)malloc(512 + strlen(WINDOW_TITLE_PREFIX));

		sprintf(tempString, "%s: %d Frames Per Second @ %d x %d", WINDOW_TITLE_PREFIX, frameCount * 4, currentWidth, currentHeight);

		glutSetWindowTitle(tempString);
		free(tempString);
	}
	
	frameCount = 0;
	glutTimerFunc(250, timerFunction, 1);
}

void buildShaders(void) {
	shaderIds = (GLuint*)calloc(numOfShaders, sizeof(GLuint));
	vertexShaderIds = (GLuint*)calloc(numOfShaders, sizeof(GLuint));
	fragmentShaderIds = (GLuint*)calloc(numOfShaders, sizeof(GLuint));

	for (int i = 0; i < numOfShaders; i++) {
		// Create the shader program and load the shaders
		
		shaderIds[i] = glCreateProgram();
		exitOnGLError("ERROR: Could not create the shader program\n");

		vertexShaderIds[i] = loadShader(vertexShaders[i], GL_VERTEX_SHADER);
		fragmentShaderIds[i] = loadShader(fragmentShaders[i], GL_FRAGMENT_SHADER);

		// Attach the loaded shaders to the created shader program
		glAttachShader(shaderIds[i], vertexShaderIds[i]);
		glAttachShader(shaderIds[i], fragmentShaderIds[i]);
		exitOnGLError("ERROR: Could not attach the shaders to the shader program\n");

		glLinkProgram(shaderIds[i]);
		exitOnGLError("ERROR: Could not link the shader program\n");

		// Validate the shader program status
		int isValid;
		glValidateProgram(shaderIds[i]);
		glGetProgramiv(shaderIds[i], GL_VALIDATE_STATUS, &isValid);
		if (!isValid) {
			fprintf(stderr, "Validity check failed, there was a problem in building the shader program\n");
			fprintf(stderr, "Enter anything to exit..\n");
			scanf("%s");
			exit(EXIT_FAILURE);
		}

		modelMatrixUniformLocation = glGetUniformLocation(shaderIds[i], "modelMatrix");
		viewMatrixUniformLocation = glGetUniformLocation(shaderIds[i], "viewMatrix");
		projectionMatrixUniformLocation = glGetUniformLocation(shaderIds[i], "projectionMatrix");
		exitOnGLError("ERROR: Could not get the shader uniform locations for matrices\n");
		enableLights(shaderIds[i]);
	}
	currentProgramId = shaderIds[0];
	enableTextures(currentProgramId);
	enableNormalMapTextures(shaderIds[numOfShaders-1]);
}

void createBunny(void) {
    FILE* plyFile;
    plyFile = fopen("bun_zipper.ply", "rb");

    if (plyFile == NULL) {
        printf("Failed to open file at: %s\n", "bun_zipper.ply");
        exit(EXIT_FAILURE);
    }
    
	// Read the number of vertices and faces from the header 
    readPlyHeader(plyFile, &lineNumber, &numOfVertices, &numOfFaces);

	// Read the vertex and face data
	Vertex* VERTICES = readPlyVertexData(plyFile, &lineNumber, numOfVertices);
	Face* FACES = readPlyFaceData(plyFile, &lineNumber, &numOfFaces);

	// Calculate the face normals and create a mapping of vertices to its adjacent faces
	// finally, calculate the vertex normals accordingly
	calculateFaceNormals(FACES, VERTICES, numOfFaces);
	int** neighbourMap = createNeighbourMap(FACES, numOfFaces, numOfVertices);
	calculateVertexNormals(VERTICES, FACES, (const int**)neighbourMap, numOfVertices);

	// Generate a sphere mapped U,V mapping for texturing purposes
	generateUVMapping(VERTICES, FACES, neighbourMap, numOfVertices, numOfFaces);

	// TODO: COMPILE SHADERS
	buildShaders();
	
	glGenVertexArrays(1, &bufferIds[0]);
	exitOnGLError("ERROR: Could not generate the VAO");
	glBindVertexArray(bufferIds[0]);
	exitOnGLError("ERROR: Could not bind the VAO");

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	exitOnGLError("ERROR: Could not enable vertex attributes");

	glGenBuffers(2, &bufferIds[1]);
	exitOnGLError("ERROR: Could not generate the buffer objects");

	glBindBuffer(GL_ARRAY_BUFFER, bufferIds[1]);
	glBufferData(GL_ARRAY_BUFFER, numOfVertices * sizeof(Vertex), VERTICES, GL_STATIC_DRAW);
	exitOnGLError("ERROR: Could not bind the VBO to the VAO");

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(VERTICES[0]), (GLvoid*)0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VERTICES[0]), (GLvoid*)(sizeof(VERTICES[0].position)+sizeof(VERTICES[0].normal)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VERTICES[0]), (GLvoid*)(sizeof(VERTICES[0].position)));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VERTICES[0]), (GLvoid*)(sizeof(VERTICES[0].position)+sizeof(VERTICES[0].normal)+sizeof(VERTICES[0].color)));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VERTICES[0]), (GLvoid*)(sizeof(VERTICES[0].position)+sizeof(VERTICES[0].normal)+sizeof(VERTICES[0].color)+sizeof(VERTICES[0].uv)));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VERTICES[0]), (GLvoid*)(sizeof(VERTICES[0].position)+sizeof(VERTICES[0].normal)+sizeof(VERTICES[0].color)+sizeof(VERTICES[0].uv)+sizeof(VERTICES[0].tangent)));
	exitOnGLError("ERROR: Could not set VAO attributes");

	// Create IBO from faces
	int* IBO = (int*)malloc(numOfFaces * 3 * sizeof(int));
	for (int i = 0; i < numOfFaces; i++)
		for (int j = 0; j < 3; j++)
			IBO[(i*3) + j] = FACES[i].vertices[j];

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numOfFaces * 3 * sizeof(int), IBO, GL_STATIC_DRAW);
	exitOnGLError("ERROR: Could not bind the IBO to the VAO");

	glBindVertexArray(0);

	translateMatrix(&modelMatrix, 0.0f, -0.12f, 0.0f);

	fclose(plyFile);
	free(VERTICES);
	free(FACES);
	free(IBO);
	destroyNeighbourMap(neighbourMap, numOfVertices);
}

void generateUVMapping(Vertex* vertices, Face* faces, int** neighbourMap, int numOfVertices, int numOfFaces) {
	Vector* north = (Vector*)malloc(sizeof(Vector));
			north->coord[0] = 0.0f;
			north->coord[1] = 1.0f;
			north->coord[2] = 0.0f;
			north->coord[3] = 1.0f;
	Vector* west = (Vector*)malloc(sizeof(Vector));
			west->coord[0] = 1.0f;
			west->coord[1] = 0.0f;
			west->coord[2] = 0.0f;
			west->coord[3] = 1.0f;

	for (int i = 0; i < numOfVertices; i++) {
		(vertices[i]).uv[0] = dotProduct(&(vertices[i].position), north) * 1000.0f;
		(vertices[i]).uv[1] = dotProduct(&(vertices[i].position), west)  * 1000.0f;
	}

	for (int i = 0; i < numOfFaces; i++) {
		Vector v1 = vectorSubtract(&vertices[faces[i].vertices[1]].position, &vertices[faces[i].vertices[0]].position);
		Vector v2 = vectorSubtract(&vertices[faces[i].vertices[2]].position, &vertices[faces[i].vertices[0]].position);

		for (int j = 0; j < 4; j++)
			v1.coord[j] *=  1000.0f;
		for (int j = 0; j < 4; j++)
			v2.coord[j] *=  1000.0f;

		float st1[2] = {vertices[faces[i].vertices[1]].uv[0] - vertices[faces[i].vertices[0]].uv[0], vertices[faces[i].vertices[1]].uv[1] - vertices[faces[i].vertices[0]].uv[1]};
		float st2[2] = {vertices[faces[i].vertices[2]].uv[0] - vertices[faces[i].vertices[0]].uv[0], vertices[faces[i].vertices[2]].uv[1] - vertices[faces[i].vertices[0]].uv[1]};

		float divisor = (st1[0] * st2[1] - st1[1] * st2[0]);
		float coef = 1 / divisor;

		if (divisor == 0)
			fprintf(stderr, "ERROR: Zero divisor while creating the UV mapping\n");
		
		faces[i].tangent.coord[0] = coef * ((v1.coord[0] * st2[1]) + (v2.coord[0] * -st1[1]));
		faces[i].tangent.coord[1] = coef * ((v1.coord[1] * st2[1]) + (v2.coord[1] * -st1[1]));
		faces[i].tangent.coord[2] = coef * ((v1.coord[2] * st2[1]) + (v2.coord[2] * -st1[1]));
		faces[i].tangent.coord[3] = 0.0f;

		faces[i].binormal = crossProduct(&faces[i].normal, &faces[i].tangent);

	}
	for (int i = 0; i < numOfVertices; i++) {
		for (int j = 0; neighbourMap[i][j] != -1; j++) {

			vertices[i].tangent.coord[0] += faces[neighbourMap[i][j]].tangent.coord[0];
			vertices[i].tangent.coord[1] += faces[neighbourMap[i][j]].tangent.coord[1];
			vertices[i].tangent.coord[2] += faces[neighbourMap[i][j]].tangent.coord[2];

			vertices[i].binormal.coord[0] += faces[neighbourMap[i][j]].binormal.coord[0];
			vertices[i].binormal.coord[1] += faces[neighbourMap[i][j]].binormal.coord[1];
			vertices[i].binormal.coord[2] += faces[neighbourMap[i][j]].binormal.coord[2];
		}
		
		normalize(&vertices[i].tangent);
		vertices[i].tangent.coord[3] = 0.0f;
		
		normalize(&vertices[i].binormal);
		vertices[i].binormal.coord[3] = 0.0f;
	}

	free(north);
	free(west);
}

LightSource light0 = {{0.0f, 0.0f, 0.0f, 0.0f}, {2.0f, -3.0f, -1.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, 0}; // Directional light
LightSource light1 = {{0.0f, -1.0f, -2.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f, 1.0f}, 0.0f, 0.0f, 1};  // Point light
LightSource light2 = {{0.0f, 0.8f, -2.0f, 1.0f}, {0.0f, -0.8f, 2.0f}, {0.9f, 0.9f, 0.9f, 1.0f}, 30.f, 10.0f, 2}; // Spotlight
LightSource light3 = {{0.0f, 0.5f, 2.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 30.f, 10.0f, 2}; // Spotlight
LightSource lights[4] = {light0, light1, light2, light3};
static const int numOfLights = sizeof(lights) / sizeof(LightSource);
Material material = {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 15.0f};

void enableLights(GLuint programId) {
	lightUniformLocation = glGetUniformLocation(programId, "numOfLights");
	glProgramUniform1i(programId, lightUniformLocation, numOfLights);

	for (int i = 0; i < numOfLights; i++) {
		char prefix[20], fullName[40];
		sprintf(prefix, "lights[%d].", i);

		sprintf(fullName, "%s%s", prefix, "position");
		lightUniformLocation = glGetUniformLocation(programId, fullName);
		glProgramUniform4fv(programId, lightUniformLocation, 1, lights[i].position);

		sprintf(fullName, "%s%s", prefix, "direction");
		lightUniformLocation = glGetUniformLocation(programId, fullName);
		glProgramUniform3fv(programId, lightUniformLocation, 1, lights[i].direction);

		sprintf(fullName, "%s%s", prefix, "color");
		lightUniformLocation = glGetUniformLocation(programId, fullName);
		glProgramUniform4fv(programId, lightUniformLocation, 1, lights[i].color);

		sprintf(fullName, "%s%s", prefix, "spotCutoff");
		lightUniformLocation = glGetUniformLocation(programId, fullName);
		glProgramUniform1f(programId, lightUniformLocation, lights[i].spotCutoff);

		sprintf(fullName, "%s%s", prefix, "spotCoeff");
		lightUniformLocation = glGetUniformLocation(programId, fullName);
		glProgramUniform1f(programId, lightUniformLocation, lights[i].spotCoeff);

		sprintf(fullName, "%s%s", prefix, "type");
		lightUniformLocation = glGetUniformLocation(programId, fullName);
		glProgramUniform1i(programId, lightUniformLocation, lights[i].type);

		exitOnGLError("ERROR: Could not set the shader uniforms for lights");
	}

	materialUniformLocation = glGetUniformLocation(programId, "material.diffuse");
	glProgramUniform4fv(programId, materialUniformLocation, 1, material.diffuse);
	materialUniformLocation = glGetUniformLocation(programId, "material.specular");
	glProgramUniform4fv(programId, materialUniformLocation, 1, material.specular);
	materialUniformLocation = glGetUniformLocation(programId, "material.specularity");
	glProgramUniform1f(programId, materialUniformLocation, material.specularity);
	exitOnGLError("ERROR: Could not get the shader uniform locations for material");
}

void enableTextures(GLuint programId) {
	glUseProgram(programId);
	
	static const char* textures[4] = {"textures/texture1.sgi", "textures/texture2.sgi", "textures/texture5.sgi", "textures/texture6.sgi"};
	const int numOfTextures = sizeof(textures)/sizeof(char*);

	glGenTextures(numOfTextures, textureId);
	textureUniformLocation = glGetUniformLocation(programId, "textureImage");

	for (int i = 0; i < numOfTextures; i++) {
		RGBImage* texture = readRGBImage(textures[i]);
		
		if (texture == NULL) {
			fprintf(stderr, "Failed to read the texture file, textures not loaded\n");
			fprintf(stderr, "Enter anything to exit..\n");
			scanf("%s");
			exit(EXIT_FAILURE);
			return;
		}

		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureId[i]);


		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glProgramUniform1i(programId, textureUniformLocation, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureId[i]);

		destroyRGBImage(texture);
	}

	glUseProgram(0);
}

void enableNormalMapTextures(GLuint programId) {
	glUseProgram(programId);
	
	static const char* textures[] = {"textures/normal_map_texture.sgi"};
	const int numOfTextures = sizeof(textures)/sizeof(char*);

	normalMapUniformLocation = glGetUniformLocation(programId, "normalMapImage");

	if (normalMapUniformLocation == -1) {
		glGetError();
		return;
	}

	glGenTextures(numOfTextures, normalMapTextureId);

	for (int i = 0; i < numOfTextures; i++) {

		RGBImage* texture = readRGBImage(textures[i]);
		if (texture == NULL) {
			fprintf(stderr, "Failed to read the normal map texture file, normal map textures not loaded\n");
			fprintf(stderr, "Enter anything to exit..\n");
			scanf("%s");
			exit(EXIT_FAILURE);
			return;
		}

		glActiveTexture(GL_TEXTURE0+4+i);  // TODO MAGIC NUMBER
		glBindTexture(GL_TEXTURE_CUBE_MAP, normalMapTextureId[i]);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, texture->sizeX, texture->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->RGB);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glProgramUniform1i(programId, normalMapUniformLocation, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, normalMapTextureId[i]);
	
		destroyRGBImage(texture);
	}
	glUseProgram(0);
}

void destroyBunny(void) {
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);

	for (int i = 0; i < numOfShaders; i++) {
		glDetachShader(shaderIds[i], vertexShaderIds[i]);
		glDetachShader(shaderIds[i], fragmentShaderIds[i]);

		glDeleteShader(vertexShaderIds[i]);
		glDeleteShader(fragmentShaderIds[i]);
		glDeleteProgram(shaderIds[i]);
		exitOnGLError("ERROR: Could not destroy the shaders");
	}

	glDeleteBuffers(2, &bufferIds[1]);
	glDeleteVertexArrays(1, &bufferIds[0]);
	exitOnGLError("ERROR: Could not destroy the buffer objects");

	glDeleteTextures(sizeof(textureId)/sizeof(int), textureId);
	glDeleteTextures(1, normalMapTextureId);
	exitOnGLError("ERROR: Could not free the texture resources");
}

void drawBunny() {
	float bunnyAngle;
	clock_t now = clock();
	if (lastTime == 0)
		lastTime = now;

	rotation += 45.0f * ((float)(now - lastTime) / CLOCKS_PER_SEC);
	bunnyAngle = degreesToRadians(rotation);

	lastTime = now;
	glUseProgram(currentProgramId);
	glGetError();

	now = now * 1000 / CLOCKS_PER_SEC;

	modelMatrixUniformLocation = glGetUniformLocation(currentProgramId, "modelMatrix");
	viewMatrixUniformLocation = glGetUniformLocation(currentProgramId, "viewMatrix");
	projectionMatrixUniformLocation = glGetUniformLocation(currentProgramId, "projectionMatrix");
	textureUniformLocation = glGetUniformLocation(currentProgramId, "textureImage");
	exitOnGLError("ERROR: Could not use the shader program");
	

	timeUniformLocation = glGetUniformLocation(currentProgramId, "time");
	glProgramUniform1i(currentProgramId, timeUniformLocation, now);
	
	GLint throbbingUniformLocation = glGetUniformLocation(currentProgramId, "throbbing");
	glProgramUniform1i(currentProgramId, throbbingUniformLocation, throbbing);

	glProgramUniformMatrix4fv(currentProgramId, modelMatrixUniformLocation, 1, GL_FALSE, modelMatrix.m);
	glProgramUniformMatrix4fv(currentProgramId, viewMatrixUniformLocation, 1, GL_FALSE, viewMatrix.m);
	glProgramUniformMatrix4fv(currentProgramId, projectionMatrixUniformLocation, 1, GL_FALSE, projectionMatrix.m);

	exitOnGLError("ERROR: Could not set the shader uniforms for matrices");

	enableLights(currentProgramId);

	glBindVertexArray(bufferIds[0]);
	exitOnGLError("ERROR: Could not bind the VAO for drawing purposes");

	glDrawElements(GL_TRIANGLES, numOfFaces * 3, GL_UNSIGNED_INT, (GLvoid*)0);
	exitOnGLError("ERROR: Could not draw the bunny");

	glBindVertexArray(0);
	glUseProgram(0);

	// Moving the spotlights
	lights[2].position[0] = sin(now*1.0f / 1000.0f);
	lights[2].position[2] = cos(now*1.0f / 1000.0f);

	lights[2].direction[0] = -lights[2].position[0];
	lights[2].direction[2] = -lights[2].position[2];

	lights[3].position[1] = cos(now*1.0f / 1000.0f);
	lights[3].position[2] = sin(now*1.0f / 1000.0f);

	lights[3].direction[1] = -lights[3].position[1];
	lights[3].direction[2] = -lights[3].position[2];
}