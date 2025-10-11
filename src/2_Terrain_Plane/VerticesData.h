#pragma once

class VerticesData {
private:
	float* vertices;
	unsigned int* indices;
public:
	VerticesData(float* vertices, unsigned int* indices, unsigned int verticesCount, unsigned int indicesCount);
	~VerticesData();
	float* getVertices() const;
	unsigned int* getIndices() const;
	unsigned int verticesCount;
	unsigned int indicesCount;
	unsigned int stripsCount;
	unsigned int numOfverticesPerStrip;
};