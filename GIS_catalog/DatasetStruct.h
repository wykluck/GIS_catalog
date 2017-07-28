#pragma once
#include <string>
#include <vector>
#include <ctime>
struct DatasetStruct {
	int width;
	int height;
	int bandCount;
	int dataTypeSizeInBits;
	std::string datasetPath;
	std::string spatialId;
	std::vector<double> geoTransformParams;
	std::time_t updatedTime;
};