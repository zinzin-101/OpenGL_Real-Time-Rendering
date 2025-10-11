#pragma once

struct HeightMapData {
	HeightMapData(float** data, unsigned int width): data(data), width(width) {}
	float** data;
	unsigned int width;
};

class HeightMap {
	private:
		unsigned int width;
		unsigned int currentWidth;
		float** data;
		void clearData();

		// Diamond-Square algorithm
		static void diamondSquare(float** data, unsigned int width);
		static void squareStep(float** data, unsigned int width);
		static void diamondStep(float** data, unsigned int width);

	public:
		HeightMap(unsigned int width);
		~HeightMap();
		void setWidth(unsigned int width);
		void generateHeightMap();
		HeightMapData getData() const;
};