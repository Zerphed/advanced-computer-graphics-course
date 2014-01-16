#include "utils.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

Vector crossProduct(const Vector* v1, const Vector* v2) {
    assert(v1 && v2);

    Vector ret;
    ret.coord[0] = v1->coord[1] * v2->coord[2] - v2->coord[1] * v1->coord[2];
    ret.coord[1] = v1->coord[2] * v2->coord[0] - v2->coord[2] * v1->coord[0];
    ret.coord[2] = v1->coord[0] * v2->coord[1] - v2->coord[0] * v1->coord[1];
	ret.coord[3] = 1.0;

    return ret;
}

Vector vectorSubtract(const Vector* v1, const Vector* v2) {
	assert(v1 && v2);

	Vector ret;

	for (int i = 0; i < 4; i++) {
		ret.coord[i] = v1->coord[i] - v2->coord[i];
	}
	return ret;
}

float dotProduct(const Vector* v1, const Vector* v2) {
    assert(v1 && v2);

    float ret = 0;
    for (int i = 0; i < 3; i++)
        ret += v1->coord[i] * v2->coord[i];
    return ret;
}

void normalize(Vector* v) {
    assert(v);

    float length = MAX(sqrt(v->coord[0]*v->coord[0] + v->coord[1]*v->coord[1] + v->coord[2]*v->coord[2]), 0.0000001f);
    v->coord[0] /= length;
    v->coord[1] /= length;
    v->coord[2] /= length;
}

float cotangent(float angle) {
	return (float)(1.0 / tan(angle));
}

float degreesToRadians(float degrees) {
	return degrees * (float)(PI / 180.0);
}

float radiansToDegrees(float radians) {
	return radians * (float)(180 / PI);
}

Matrix multiplyMatrices(const Matrix* m1, const Matrix* m2) {
	assert(m1 && m2);

	Matrix ret = IDENTITY_MATRIX;
	unsigned int row, col, rowOffset;

	for (row = 0, rowOffset = row * 4; row < 4; row++, rowOffset = row * 4)
		for (col = 0; col < 4; col++)
			ret.m[rowOffset + col] = 
				(m2->m[rowOffset+0] * m1->m[col+0]) +
				(m2->m[rowOffset+1] * m1->m[col+4]) +
				(m2->m[rowOffset+2] * m1->m[col+8]) +
				(m2->m[rowOffset+3] * m1->m[col+12]);
	return ret;
}

Vector multiplyMatrixVector(const Matrix* m, const Vector* v) {
	assert(m && v);

	Vector ret;

	ret.coord[0] = m->m[0] * v->coord[0] + m->m[4] * v->coord[1] + m->m[8] * v->coord[2] + m->m[12] * v->coord[3];
	ret.coord[1] = m->m[1] * v->coord[0] + m->m[5] * v->coord[1] + m->m[9] * v->coord[2] + m->m[13] * v->coord[3];
	ret.coord[2] = m->m[2] * v->coord[0] + m->m[6] * v->coord[1] + m->m[10] * v->coord[2] + m->m[14] * v->coord[3];
	ret.coord[3] = m->m[3] * v->coord[0] + m->m[7] * v->coord[1] + m->m[11] * v->coord[2] + m->m[15] * v->coord[3];

	return ret;
}

// Only works for a 4x4 transformation matrix
Matrix inverseMatrix(const Matrix* m) {
	assert(m);

	Matrix inverted = IDENTITY_MATRIX;

	// 3x3 top-left sub-matrix inverse
	float det = (m->m[0]*m->m[5]*m->m[10] + m->m[1]*m->m[6]*m->m[8] + m->m[2]*m->m[4]*m->m[9]) - (m->m[0]*m->m[6]*m->m[9] + m->m[2]*m->m[5]*m->m[8] + m->m[1]*m->m[4]*m->m[10]);
	
	// 1. column
	inverted.m[0] = (1.0f / det) * (m->m[5] * m->m[10] - m->m[9] * m->m[6]);
	inverted.m[1] = (1.0f / det) * (m->m[9] * m->m[2] - m->m[1] * m->m[10]);
	inverted.m[2] = (1.0f / det) * (m->m[1] * m->m[6] - m->m[5] * m->m[2]);
	inverted.m[3] = 0.0f;

	// 2. column
	inverted.m[4] = (1.0f / det) * (m->m[8] * m->m[6] - m->m[4] * m->m[10]);
	inverted.m[5] = (1.0f / det) * (m->m[0] * m->m[10] - m->m[8] * m->m[2]);
	inverted.m[6] = (1.0f / det) * (m->m[4] * m->m[2] - m->m[0] * m->m[6]);
	inverted.m[7] = 0.0f;

	// 3. column
	inverted.m[8] = (1.0f / det) * (m->m[4] * m->m[9] - m->m[8] * m->m[5]);
	inverted.m[9] = (1.0f / det) * (m->m[8] * m->m[1] - m->m[0] * m->m[9]);
	inverted.m[10] = (1.0f / det) * (m->m[0] * m->m[5] - m->m[4] * m->m[1]);
	inverted.m[11] = 0.0f;

	// 4. column
	inverted.m[12] = -inverted.m[0]*m->m[12]-inverted.m[4]*m->m[13]-inverted.m[8]*m->m[14];
	inverted.m[13] = -inverted.m[1]*m->m[12]-inverted.m[5]*m->m[13]-inverted.m[9]*m->m[14];
	inverted.m[14] = -inverted.m[2]*m->m[12]-inverted.m[6]*m->m[13]-inverted.m[10]*m->m[14];
	inverted.m[15] = 1.0f;

	return inverted;
}

void scaleMatrix(Matrix* m, float x, float y, float z) {
	assert(m);

	Matrix scale = IDENTITY_MATRIX;

	scale.m[0] = x;
	scale.m[5] = y;
	scale.m[10] = z;

	memcpy(m->m, multiplyMatrices(&scale, m).m, sizeof(m->m));
}

void translateMatrix(Matrix* m, float x, float y, float z) {
	assert(m);

	Matrix translation = IDENTITY_MATRIX;

	translation.m[12] = x;
	translation.m[13] = y;
	translation.m[14] = z;

	memcpy(m->m, multiplyMatrices(&translation, m).m, sizeof(m->m));
}

void rotateAboutX(Matrix* m, float angle) {
	assert(m);

	Matrix rotation = IDENTITY_MATRIX;
	float sine = (float)sin(angle);
	float cosine = (float)cos(angle);

	rotation.m[5] = cosine;
	rotation.m[6] = -sine;
	rotation.m[9] = sine;
	rotation.m[10] = cosine;

	memcpy(m->m, multiplyMatrices(&rotation, m).m, sizeof(m->m));
}

void rotateAboutY(Matrix* m, float angle) {
	assert(m);

	Matrix rotation = IDENTITY_MATRIX;
	float sine = (float)sin(angle);
	float cosine = (float)cos(angle);

	rotation.m[0] = cosine;
	rotation.m[8] = sine;
	rotation.m[2] = -sine;
	rotation.m[10] = cosine;

	memcpy(m->m, multiplyMatrices(&rotation, m).m, sizeof(m->m));
}

void rotateAboutZ(Matrix* m, float angle) {
	assert(m);

	Matrix rotation = IDENTITY_MATRIX;
	float sine = (float)sin(angle);
	float cosine = (float)cos(angle);

	rotation.m[0] = cosine;
	rotation.m[1] = -sine;
	rotation.m[4] = sine;
	rotation.m[5] = cosine;

	memcpy(m->m, multiplyMatrices(&rotation, m).m, sizeof(m->m));
}

void rotateAboutAxis(Matrix* m, const Vector* v, float angle) {
	assert(m && v);

	Matrix rotation = IDENTITY_MATRIX;

	float cosa = cos(angle), sina = sin(angle);
	float ux = v->coord[0], uy = v->coord[1], uz = v->coord[2];

	// 1. column
	rotation.m[0] = cosa + ux*ux * (1-cosa);
	rotation.m[1] = uy*ux*(1-cosa) + uz*sina;
	rotation.m[2] = uz*ux*(1-cosa) - uy*sina;
	rotation.m[3] = 0.0;

	// 2. column
	rotation.m[4] = ux*uy*(1-cosa) - uz*sina;
	rotation.m[5] = cosa + uy*uy * (1-cosa);
	rotation.m[6] = uz*uy*(1-cosa) + ux*sina;
	rotation.m[7] = 0.0;

	// 3. column
	rotation.m[8] = ux*uz*(1-cosa) + uy*sina;
	rotation.m[9] = uy*uz*(1-cosa) - ux*sina;
	rotation.m[10] = cosa + uz*uz*(1-cosa);
	rotation.m[11] = 0.0;

	// 4. column
	rotation.m[12] = 0.0;
	rotation.m[13] = 0.0;
	rotation.m[14] = 0.0;
	rotation.m[15] = 1.0;

	memcpy(m->m, multiplyMatrices(&rotation, m).m, sizeof(m->m));
}

Matrix createProjectionMatrix(float fovy, float aspectRatio, float nearPlane, float farPlane) {
	Matrix ret = { { 0 } };

	const float
			yScale = cotangent(degreesToRadians(fovy / 2)),
			xScale = yScale / aspectRatio,
			frustumLength = farPlane - nearPlane;

	ret.m[0] = xScale;
	ret.m[5] = yScale;
	ret.m[10] = -((farPlane + nearPlane) / frustumLength);
	ret.m[11] = -1;
	ret.m[14] = -((2 * nearPlane * farPlane) / frustumLength);

	return ret;
}

void printMatrix(const Matrix* m) {
	fprintf(stderr, "Matrix debug\n");
	for (int i = 0; i < 4; i++)
		fprintf(stderr, "%f %f %f %f\n", m->m[i], m->m[i+4], m->m[i+8], m->m[i+12]);
}

void exitOnGLError(const char* errorMessage) {
	const GLenum errorValue = glGetError();

	if (errorValue != GL_NO_ERROR) {
		fprintf(stderr, errorMessage);
		fprintf(stderr, "\nGL ERROR: %s\n", gluErrorString(errorValue));
		fprintf(stderr, "Enter anything to exit..\n");
		scanf("%s");
		exit(EXIT_FAILURE);
	}
}

GLuint loadShader(const char* filename, GLenum shaderType) {
	assert(filename);

	GLuint shaderId = 0;
	FILE* file;
	long fileSize = -1;
	GLchar* glslSource;

	if (NULL != (file = fopen(filename, "rb")) && 0 == fseek(file, 0, SEEK_END) && -1 != (fileSize = ftell(file))) 
	{
		rewind(file);
		if (NULL != (glslSource = (GLchar*)malloc(fileSize + 1 * sizeof(GLchar)))) {
			if (fileSize == (long)fread(glslSource, sizeof(char), fileSize, file)) 
			{
				glslSource[fileSize] = '\0';

				if (0 != (shaderId = glCreateShader(shaderType))) {
					glShaderSource(shaderId, 1, (const GLchar**)&glslSource, NULL);
					glCompileShader(shaderId);
					exitOnGLError("ERROR: Could not compile a shader\n");
				}
				else
					fprintf(stderr, "ERROR: could not create a shader\n");
			}
			else
				fprintf(stderr, "ERROR: could not read file %s\n", filename);

			free(glslSource);
		}
		else
			fprintf(stderr, "ERROR: could not allocate %ld bytes\n", fileSize);

		fclose(file);
	}
	else
		fprintf(stderr, "ERROR: could not open file %s\n", filename);
	
	return shaderId;
}