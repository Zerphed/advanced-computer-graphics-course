#include "plyparser.hh"
#include "utils.hh"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/* Header reading operations */
void readPlyHeader(FILE* plyFile, int* lineNumber, int* numOfVertices, int* numOfFaces) {

  char lineBuffer[120] = {'\0'};

    do {
        if (fgets(lineBuffer, 120, plyFile) == NULL) {
            printf("Unexpected end of file, while reading the header @ line %d.\n", *lineNumber);
            fclose(plyFile);
            exit(EXIT_FAILURE);
        }

        if (strncmp(lineBuffer, "element vertex", 14) == 0) {
            *numOfVertices = atoi(lineBuffer + 15);
        }
        else if (strncmp(lineBuffer, "element face", 12) == 0) {
            *numOfFaces = atoi(lineBuffer + 12);
        }

        (*lineNumber)++;

    } while (strcmp(lineBuffer, "end_header\n") != 0);
}

Vertex* readPlyVertexData(FILE* plyFile, int* lineNumber, int numOfVertices) {
	Vertex* vertexData = (Vertex*)calloc(numOfVertices, sizeof(Vertex));

    char lineBuffer[120] = {'\0'};
    float x, y, z, confidence, intensity;

    for (int i = 0; i < numOfVertices; i++, (*lineNumber)++) {

        if (fscanf(plyFile, "%f %f %f %f %f", &x, &y, &z, &confidence, &intensity) != 5) {
            printf("Failed to read vertex data @ line %d\n", *lineNumber);
            free(vertexData);
            fclose(plyFile);
            exit(EXIT_FAILURE);
        }

		// Initialize vertex: position, normal, color, uv
        Vertex v = { {x, y, z, 1.0}, {0.0, 0.0, 0.0, 0.0}, {0.5, 0.5, 0.5, 1.0}, {0.0, 0.0} };
        vertexData[i] = v;
    }

	return vertexData;
}

Face* readPlyFaceData(FILE* plyFile, int* lineNumber, int* numOfFaces) {
	Face* faceData = (Face*)calloc(*numOfFaces, sizeof(Face));
    int type, v1, v2, v3, v4;
	bool reallocated = false;
	int additionalFaces = 0;

    for (int i = 0; i < (*numOfFaces) + additionalFaces; i++, (*lineNumber)++) {
		if (fscanf(plyFile, "%d", &type) != 1) {
			printf("Failed to read index buffer data @ line %d\n", *lineNumber);
            free(faceData);
            fclose(plyFile);
            exit(EXIT_FAILURE);
		}
		if (type == 3) {
			if (fscanf(plyFile, "%d %d %d", &v1, &v2, &v3) != 3) {
				printf("Failed to read index buffer data @ line %d\n", *lineNumber);
				free(faceData);
				fclose(plyFile);
				exit(EXIT_FAILURE);
			}

			faceData[i].vertices[0] = v1;
			faceData[i].vertices[1] = v2;
			faceData[i].vertices[2] = v3;

			faceData[i].normal.coord[0] = 0.0;
			faceData[i].normal.coord[1] = 0.0;
			faceData[i].normal.coord[2] = 0.0;
			faceData[i].normal.coord[3] = 0.0;
		}
		if (type == 4) {
			if (!reallocated) {
				faceData = (Face*)realloc(faceData, (*numOfFaces)*2 * sizeof(Face));
				reallocated = true;
			}

			if (fscanf(plyFile, "%d %d %d %d", &v1, &v2, &v3, &v4) != 4) {
				printf("Failed to read index buffer data @ line %d\n", *lineNumber);
				free(faceData);
				fclose(plyFile);
				exit(EXIT_FAILURE);
			}

			faceData[i].vertices[0] = v1;
			faceData[i].vertices[1] = v2;
			faceData[i].vertices[2] = v3;

			faceData[i].normal.coord[0] = 0.0;
			faceData[i].normal.coord[1] = 0.0;
			faceData[i].normal.coord[2] = 0.0;
			faceData[i].normal.coord[3] = 0.0;

			i++, additionalFaces++;

			faceData[i].vertices[0] = v1;
			faceData[i].vertices[1] = v3;
			faceData[i].vertices[2] = v4;

			faceData[i].normal.coord[0] = 0.0;
			faceData[i].normal.coord[1] = 0.0;
			faceData[i].normal.coord[2] = 0.0;
			faceData[i].normal.coord[3] = 0.0;
		}
    }

	*numOfFaces = *numOfFaces + additionalFaces;

	return faceData;
}

Vector calculateFaceNormal(const Face* face, const Vertex* vertexData) {
    // Hold the result of vectors from A to B and A to C
    Vector temp[2];

    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 3; j++)
			temp[i].coord[j] = vertexData[face->vertices[2-i]].position.coord[j] - vertexData[face->vertices[0]].position.coord[j];

    Vector normal = crossProduct(&temp[0], &temp[1]);
	normalize(&normal);

    return normal;
}

void calculateFaceNormals(Face* faceData, const Vertex* vertexData, int numOfFaces) {
    for (int i = 0; i < numOfFaces; i++)
        faceData[i].normal = calculateFaceNormal(&faceData[i], vertexData);
}

Vector calculateVertexNormal(const Face* faceData, const int* vertexNeighbours) {
	Vector ret = {0.0, 0.0, 0.0, 0.0};

    for (int i = 0; vertexNeighbours[i] != -1; i++) {
		ret.coord[0] += faceData[vertexNeighbours[i]].normal.coord[0];
        ret.coord[1] += faceData[vertexNeighbours[i]].normal.coord[1];
        ret.coord[2] += faceData[vertexNeighbours[i]].normal.coord[2];
    }

    normalize(&ret);
    return ret;
}

void calculateVertexNormals(Vertex* vertexData, const Face* faceData, const int** neighbourMap, int numOfVertices) {
    for (int i = 0; i < numOfVertices; i++)
		vertexData[i].normal = calculateVertexNormal(faceData, neighbourMap[i]);
}

void addFace(int** neighbourMap, int faceIndex) {
    int size = 0;
    for (size = 0; (*neighbourMap)[size] != -1; size++);

    *neighbourMap = (int*)realloc(*neighbourMap, (size+2)*sizeof(int));
    (*neighbourMap)[size] = faceIndex;
    (*neighbourMap)[size+1] = -1;
}

int** createNeighbourMap(const Face* faceData, int numOfFaces, int numOfVertices) {
	// Initialize the neighbour map, each vertex is mapped to a list of face indices
    int** neighbourMap = (int**)calloc((numOfVertices) * sizeof(int*), 1);
    for (int i = 0; i < numOfVertices; i++) {
        neighbourMap[i] = (int*)malloc(1 * sizeof(int));
		neighbourMap[i][0] = -1;
	}

    for (int i = 0; i < numOfFaces; i++)
        for (int j = 0; j < 3; j++) {
			//fprintf(stderr, "Adding face %d to vertex %d", i, );
			addFace(&neighbourMap[faceData[i].vertices[j]], i);
		}

    return neighbourMap;
}

void destroyNeighbourMap(int** neighbourMap, int numOfVertices) {
	for (int i = 0; i < numOfVertices; i++)
		free(neighbourMap[i]);
	free(neighbourMap);
}