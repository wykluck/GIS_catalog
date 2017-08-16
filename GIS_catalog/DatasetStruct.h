#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <opencv2/core/types.hpp>


struct DatasetStruct {
	int width;
	int height;
	int bandCount;
	int dataTypeSizeInBits;
	std::string datasetPath;
	std::string spatialId;
	std::vector<double> geoTransformParams;
	std::vector<double> nativeBoundingBoxVec; //follow in minx, miny, maxx, maxy order
	std::string units;
};