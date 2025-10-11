#pragma once
#include <vector>

const float HEIGHT_SCALING_FACTOR = 3.0f;
const float HORIZONTAL_SCALING_FACTOR = 1.0f;

static float* getVertsFromHeightMap(float** data, unsigned int width) {
	unsigned int numOfVerts = width * width;
	float* verts = new float[numOfVerts * 3];
	for (int i = 0; i < numOfVerts; i++) {
		verts[i * 3 + 0] = (float)(i % width) * HORIZONTAL_SCALING_FACTOR;
		verts[i * 3 + 1] = data[i / width][i % width] * HEIGHT_SCALING_FACTOR;
		verts[i * 3 + 2] = (float)(i / width) * HORIZONTAL_SCALING_FACTOR;
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
}