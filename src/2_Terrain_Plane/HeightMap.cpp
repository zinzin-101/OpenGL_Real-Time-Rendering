#include "HeightMap.h"
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
	return HeightMapData(this->data, this->width);
}

void HeightMap::clearData() {
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
	}
}

void HeightMap::diamondSquare(float** data, unsigned int width) {

}

void HeightMap::squareStep(float** data, unsigned int width) {

}

void HeightMap::diamondStep(float** data, unsigned int width) {

}