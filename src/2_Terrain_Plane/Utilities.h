#pragma once
#include <vector>

const float HEIGHT_SCALING_FACTOR = 0.5f;
const float HORIZONTAL_SCALING_FACTOR = 0.5f;

struct VerticesData {
	VerticesData(float* vertices, unsigned int* indices, unsigned int verticesCount, unsigned int indicesCount) :
		vertices(vertices), indices(indices), verticesCount(verticesCount), indicesCount(indicesCount), stripsCount(0), numOfverticesPerStrip(0) {
	}
	float* vertices;
	unsigned int* indices;
	unsigned int verticesCount;
	unsigned int indicesCount;
	unsigned int stripsCount;
	unsigned int numOfverticesPerStrip;
};

VerticesData getVerticesFromHeightMap(float** data, unsigned int width);