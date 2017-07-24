﻿#include <nan.h>
#include <thread>
#include <mutex>
#include "gdal.h"
#include "gdal_priv.h"
#include "concurrentqueue.h"

static moodycamel::ConcurrentQueue<std::string> s_datasetPathQueue;
static const int threadNum = 16;
static std::vector<std::thread> threadVec;
enum class CrawlStatus{
	NotStarted,
	Started,
	Finished
};
static CrawlStatus crawlStatus = CrawlStatus::NotStarted;
static std::mutex outputMutex;

NAN_METHOD(GdalInit) {
	//TODO: GDAL plugin dir and data dir should be passed, disable it for now
	//LoadLibrary("C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\NCSEcwd.dll");
	//CPLSetConfigOption("GDAL_DRIVER_PATH", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\gdalplugins");
	//CPLSetConfigOption("GDAL_DATA", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\data");
	GDALAllRegister();
}



NAN_METHOD(RetrieveDatasetInfo) {
	if (!info[0]->IsString())
	{
		Nan::ThrowTypeError("Wrong arguments");
		return;
	}
	v8::String::Utf8Value utf8Path(info[0]);
	s_datasetPathQueue.enqueue(std::string(*utf8Path));
	
	if (crawlStatus == CrawlStatus::NotStarted)
		crawlStatus = CrawlStatus::Started;
	if (threadVec.size() < threadNum)
	{
		threadVec.push_back(std::thread([&]() {
			std::string datasetPath;
			while (1)
			{
				if (s_datasetPathQueue.try_dequeue(datasetPath))
				{
					GDALDataset *poDataset = static_cast<GDALDataset *>(GDALOpen(datasetPath.c_str(), GA_ReadOnly));
					if (poDataset != NULL)
					{
						auto width = poDataset->GetRasterXSize();
						auto height = poDataset->GetRasterYSize();
						auto bandCount = poDataset->GetRasterCount();
						outputMutex.lock();
						printf("Path: %s, Width: %d, Height: %d, BandCount: %d", datasetPath.c_str(), width, height, bandCount);
						double geoTransform[6];
						if (poDataset->GetGeoTransform(geoTransform) == CE_None)
						{
							printf(" GeoTransform: %f, %f, %f, %f, %f, %f\n", geoTransform[0], geoTransform[1], geoTransform[2], 
								geoTransform[3], geoTransform[4], geoTransform[5]);
						}
						else
						{
							printf("\n");
						}
						GDALClose(poDataset);
						outputMutex.unlock();
					}
					else
					{
						outputMutex.lock();
						printf("Unable to open dataset at %s\n", datasetPath.c_str());
						outputMutex.unlock();
					}
					
				}
				else
				{
					if (crawlStatus == CrawlStatus::Finished)
					{
						return;
					}
					else
					{
						std::this_thread::sleep_for(std::chrono::microseconds(100));
					}
				}
			}
		}));
	}
}

NAN_METHOD(FinishCrawl) {
	crawlStatus = CrawlStatus::Finished;
	return;
}

// Module initialization logic
NAN_MODULE_INIT(Initialize) {
	// Export the `GdalInit` function (equivalent to `export function Hello (...)` in JS)
	NAN_EXPORT(target, GdalInit);
	NAN_EXPORT(target, RetrieveDatasetInfo);
	NAN_EXPORT(target, FinishCrawl);
}

// Create the module called "addon" and initialize it with `Initialize` function (created with NAN_MODULE_INIT macro)
NODE_MODULE(addon, Initialize);