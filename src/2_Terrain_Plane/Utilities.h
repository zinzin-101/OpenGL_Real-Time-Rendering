#pragma once
#include <vector>

const float HEIGHT_SCALING_FACTOR = 8.0f;
const float HORIZONTAL_SCALING_FACTOR = 2.0f;

struct VerticesData {
	VerticesData(float* vertsAndNormals, unsigned int* indices, unsigned int verticesCount, unsigned int indicesCount) :
		vertsAndNormals(vertsAndNormals), indices(indices), verticesCount(verticesCount), indicesCount(indicesCount), stripsCount(0), numOfverticesPerStrip(0) {
	}
	float* vertsAndNormals;
	unsigned int* indices;
	unsigned int verticesCount;
	unsigned int indicesCount;
	unsigned int stripsCount;
	unsigned int numOfverticesPerStrip;
};

VerticesData getVerticesFromHeightMap(float** data, unsigned int width);