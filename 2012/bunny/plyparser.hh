#ifndef PLY_PARSER_HH
#define PLY_PARSER_HH

#include <stdio.h>

typedef struct Vector_s Vector;
typedef struct Vertex_s Vertex;
typedef struct Face_s Face;

void readPlyHeader(FILE* plyFile, int* lineNumber, int* numOfVertices, int* numOfFaces);

Vertex* readPlyVertexData(FILE* plyFile, int* lineNumber, int numOfVertices);
Face* readPlyFaceData(FILE* plyFile, int* lineNumber, int* numOfFaces);

Vector calculateFaceNormal(const Face* face, const Vertex* vertexData);
void calculateFaceNormals(Face* faceData, const Vertex* vertexData, int numOfFaces);

Vertex calculateVertexNormal(const Vertex* faceData, const int* vertexNeighbours);
void calculateVertexNormals(Vertex* vertexData, const Face* faceData, const int** neighbourMap, int numOfVertices);

int** createNeighbourMap(const Face* faceData, int numOfFaces, int numOfVertices);
void destroyNeighbourMap(int** neighbourMap, int numOfVertices);

#endif