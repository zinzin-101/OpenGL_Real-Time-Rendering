#include "VerticesData.h"

VerticesData::VerticesData(float* vertices, unsigned int* indices, unsigned int verticesCount, unsigned int indicesCount) :
	vertices(vertices), indices(indices), verticesCount(verticesCount), indicesCount(indicesCount), stripsCount(0), numOfverticesPerStrip(0) {
}

VerticesData::~VerticesData() {
	//delete vertices;
	//delete indices;
}

float* VerticesData::getVertices() const {
	return this->vertices;
}

unsigned int* VerticesData::getIndices() const {
	return this->indices;
}