#pragma once
int gdal_translate(GDALDatasetH hDataset, int nOXSize, int nOYSize, const std::string& thumbnailFilePath);