#pragma once
#include <string>
#include <vector>
struct DatasetStruct {
	int width;
	int height;
	int bandCount;
	int cellType;
	std::string filePath;
	std::string spatialId;
	std::vector<double> geoTransformParams;
};