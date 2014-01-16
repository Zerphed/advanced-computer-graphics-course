#ifndef UTILS_HH
#define UTILS_HH

#include <GL/glew.h>
#include <GL/freeglut.h>

#define MIN(x, y) ((x)<(y) ? (x):(y))
#define MAX(x, y) ((x)>(y) ? (x):(y))
#define ABS(x) ((x)<(0) ? (-x):(x))

static const double PI = 3.14159265358979323846;

typedef struct Vector_s {
	float coord[4];
} Vector;

typedef struct Vertex_s {
	Vector position;
	Vector normal;
	float color[4];
	float uv[2];
	Vector tangent;
	Vector binormal;
} Vertex;

typedef struct Face_s {
	int vertices[3];
	Vector normal;
	Vector tangent;
	Vector binormal;
} Face;

typedef struct Matrix_s {
	float m[16];		// Graphics is mostly involved with 4x4 matrices
} Matrix;

/* Vector operations */
Vector crossProduct(const Vector* v1, const Vector* v2);
Vector vectorSubtract(const Vector* v1, const Vector* v2);
float dotProduct(const Vector* v1, const Vector* v2);
void normalize(Vector* v);

/* Matrix operations */
static const Matrix IDENTITY_MATRIX = {
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	}
};

// Basic matrix calculation functions
Matrix multiplyMatrices(const Matrix* m1, const Matrix* m2);
Vector multiplyMatrixVector(const Matrix* m, const Vector* v);
Matrix inverseMatrix(const Matrix* m);
void rotateAboutX(Matrix* m, float angle);
void rotateAboutY(Matrix* m, float angle);
void rotateAboutZ(Matrix* m, float angle);
void rotateAboutAxis(Matrix* m, const Vector* v, float angle);
void scaleMatrix(Matrix* m, float x, float y, float z);
void translateMatrix(Matrix* m, float x, float y, float z);
void printMatrix(const Matrix* m);

// Creates a valid projection matrix according to parameters
Matrix createProjectionMatrix(float fovy, float aspectRatio, float nearPlane, float farPlane);

/* Useful calculus */

// Gives the co-tangent of the angle
float cotagent(float angle);

// Converts degrees to radians
float degreesToRadians(float degrees);

// Converts radians to degrees
float radiansToDegrees(float radians);

// Exits on GL error and displays the message
void exitOnGLError(const char* errorMessage);

// Loads a shader into the GPU from a specified file
GLuint loadShader(const char* filename, GLenum shaderType);

#endif