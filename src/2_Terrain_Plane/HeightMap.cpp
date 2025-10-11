#include "HeightMap.h"
#include "Random.h"
#include <iostream>


HeightMap::HeightMap(unsigned int width) : width(width), currentWidth(width), data(nullptr) {
	generateHeightMap();
}

HeightMap::~HeightMap() {
	clearData();
}


void HeightMap::setWidth(unsigned int width) {
	this->width = width;
}

HeightMapData HeightMap::getData() const {
	if (data == nullptr) {
		return HeightMapData(nullptr, 0);
	}
	return HeightMapData(this->data, this->width);
}

void HeightMap::clearData() {
	if (data == nullptr) {
		return;
	}

	for (int i = 0; i < currentWidth; i++) {
		delete[] data[i];
	}

	delete[] data;

	data = nullptr;
}


void HeightMap::generateHeightMap() {
	clearData();
	currentWidth = width;

	if (((currentWidth - 1) % 2) != 0) {
		std::cout << "Incompatible width" << std::endl;
		return;
	}

	data = new float*[width];
	for (int i = 0; i < width; i++) {
		data[i] = new float[width];
		for (int j = 0; j < width; j++) {
			data[i][j] = 0.0f;
		}
	}

	Random::init();
	diamondSquare(data, width / 2, width);
}

 void HeightMap::diamondSquare(float** data, int size, unsigned int width) {
	int half = size / 2;
	if (half < 1) {
		return;
	}

	// Square steps
	for (int z = half; z < width; z += size) {
		for (int x = half; x < width; x += size) {
			squareStep(data, x % width, z % width, half, width);
		}
	}

	// Diamond steps
	int col = 0;
	for (int x = 0; x < width; x += half) {
		col++;
		if (col % 2) {
			for (int z = half; z < width; z += size) {
				diamondStep(data, x % width, z % width, half, width);
			}
		}
		else {
			for (int z = 0; z < width; z += size) {
				diamondStep(data, x % width, z % width, half, width);
			}
		}
	}

	diamondSquare(data, size / 2, width);
}

void HeightMap::squareStep(float** data, int x, int z, int reach, unsigned int width) {
	int count = 0;
	float average = 0.0f;
	if (x - reach >= 0 && z - reach >= 0) {
		average += data[x - reach][z - reach];
		count++;
	}

	if (x - reach >= 0 && z + reach < width) {
		average += data[x - reach][z + reach];
		count++;
	}
	if (x + reach < width && z - reach >= 0) {
		average += data[x + reach][z - reach];
		count++;
	}
	if (x + reach < width && z + reach < width) {
		average += data[x + reach][z + reach];
		count++;
	}
	average += Random::randFloat(reach);
	average /= count;
	data[x][z] = average;
}

void HeightMap::diamondStep(float** data, int x, int z, int reach, unsigned int width) {
	int count = 0;
	float average = 0.0f;
	if (x - reach >= 0) {
		average += data[x - reach][z];
		count++;
	}
	if (x + reach < width) {
		average += data[x + reach][z];
		count++;
	}
	if (z - reach >= 0) {
		average += data[x][z - reach];
		count++;
	}
	if (z + reach < width) {
		average += data[x][z + reach];
		count++;
	}
	average += Random::randFloat(reach);
	average /= count;
	data[x][z] = average;
}