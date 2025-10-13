#include "Utilities.h"
#include <glm/glm.hpp>

VerticesData getVerticesFromHeightMap(float** data, unsigned int width) {
	unsigned int numOfVerts = width * width;
	float* verts = new float[numOfVerts * 3];
	float* normals = new float[numOfVerts * 3];
	for (int i = 0; i < numOfVerts; i++) {
		int x = i % width;
		int z = i / width;
		// Vertices
		verts[i * 3 + 0] = (float)(x) * HORIZONTAL_SCALING_FACTOR;
		verts[i * 3 + 1] = data[i / width][i % width] * HEIGHT_SCALING_FACTOR;
		verts[i * 3 + 2] = (float)(z) * HORIZONTAL_SCALING_FACTOR;

		// Normals
		float heightLeft = (x > 0) ? data[z][x - 1] : data[z][x];
		float heightRight = (x < width - 1) ? data[z][x + 1] : data[z][x];
		float heightDown = (z > 0) ? data[z - 1][x] : data[z][x];
		float heightUp = (z < width - 1) ? data[z + 1][x] : data[z][x];
		float horizontalDifference = 2.0f * HORIZONTAL_SCALING_FACTOR;
		glm::vec3 dx = glm::vec3(horizontalDifference, HEIGHT_SCALING_FACTOR * (heightRight - heightLeft), 0.0f);
		glm::vec3 dz = glm::vec3(0.0f, HEIGHT_SCALING_FACTOR * (heightUp - heightDown), horizontalDifference);
		glm::vec3 normal = glm::normalize(glm::cross(dz, dx));
		normals[i * 3 + 0] = normal.x;
		normals[i * 3 + 1] = normal.y;
		normals[i * 3 + 2] = normal.z;
	}

	std::vector<float> tempVertsAndNormals;
	for (unsigned int i = 0; i < numOfVerts; i++) {
		tempVertsAndNormals.emplace_back(verts[i * 3 + 0]);
		tempVertsAndNormals.emplace_back(verts[i * 3 + 1]);
		tempVertsAndNormals.emplace_back(verts[i * 3 + 2]);

		tempVertsAndNormals.emplace_back(normals[i * 3 + 0]);
		tempVertsAndNormals.emplace_back(normals[i * 3 + 1]);
		tempVertsAndNormals.emplace_back(normals[i * 3 + 2]);
	}
	unsigned int numOfVertsAndNormals = tempVertsAndNormals.size();
	float* vertsAndNormals = new float[numOfVertsAndNormals];
	for (int i = 0; i < numOfVertsAndNormals; i++) {
		vertsAndNormals[i] = tempVertsAndNormals[i];
	}

	std::vector<unsigned int> indicesVector;
	for (unsigned int i = 0; i < width - 1; i++) {
		for (unsigned int j = 0; j < width; j++) {
			for (unsigned int k = 0; k < 2; k++) {
				indicesVector.emplace_back(j + width * (i + k));
			}
		}
	}
	unsigned int numOfIndices = indicesVector.size();
	unsigned int* indices = new unsigned int[numOfIndices];
	for (int i = 0; i < numOfIndices; i++) {
		indices[i] = indicesVector[i];
	}

	VerticesData verticesData(vertsAndNormals, indices, numOfVerts, numOfIndices);
	verticesData.stripsCount = width - 1;
	verticesData.numOfverticesPerStrip = width * 2;

	delete[] verts;
	delete[] normals;

	return verticesData;
}