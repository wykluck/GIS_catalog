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
	std::vector<cv::Point2d> nativeBoundPolygon; 
	std::vector<cv::Point2d> geodeticBoundPolygon;
	std::vector<cv::Point2d> googleBoundPolygon;
	std::string units;
};