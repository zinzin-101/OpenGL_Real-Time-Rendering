#pragma once
#include "VerticesData.h"
#include <vector>

const float HEIGHT_SCALING_FACTOR = 0.5f;
const float HORIZONTAL_SCALING_FACTOR = 0.5f;


VerticesData getVerticesFromHeightMap(float** data, unsigned int width);