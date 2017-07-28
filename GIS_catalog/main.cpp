#include <thread>
#include <mutex>
#include <chrono>
#include <experimental/filesystem>
#include "gdal.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "concurrentqueue.h"
#include "gdal_translate.h"
#include "DatasetStruct.h"
#include "CatalogDB.h"
#include <nan.h>

static moodycamel::ConcurrentQueue<std::string> s_datasetPathQueue;
#ifdef DEBUG
static const int threadNum = 1;
#else
static const int threadNum = 16;
#endif
static const float thumbnailMaxWidth = 320.0;
static std::vector<std::thread> threadVec;
enum class QueueProcessStatus{
	NotStarted,
	Processing,
	CrawlFinished,
	ProcessingLeftover,
};
static QueueProcessStatus crawlStatus = QueueProcessStatus::NotStarted;
static std::mutex outputMutex;
static CatalogDB *s_catalogDB;


NAN_METHOD(GdalInit) {
	//TODO: GDAL plugin dir and data dir should be passed, disable it for now
	//LoadLibrary("C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\NCSEcwd.dll");
	//CPLSetConfigOption("GDAL_DRIVER_PATH", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\gdalplugins");
	//CPLSetConfigOption("GDAL_DATA", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\data");
	GDALAllRegister();
	
	//std::this_thread::sleep_for(std::chrono::milliseconds(14000));
	s_catalogDB = new CatalogDB();
}



NAN_METHOD(RetrieveDatasetInfo) {
	if (!info[0]->IsString())
	{
		Nan::ThrowTypeError("Wrong arguments");
		return;
	}
	v8::String::Utf8Value utf8Path(info[0]);
	s_datasetPathQueue.enqueue(std::string(*utf8Path));
	
	if (crawlStatus == QueueProcessStatus::NotStarted)
		crawlStatus = QueueProcessStatus::Processing;
	if (threadVec.size() < threadNum)
	{
		threadVec.push_back(std::thread([&]() {
			std::string datasetPath;
			while (1)
			{
				if (s_datasetPathQueue.try_dequeue(datasetPath))
				{
					GDALDataset *poDataset = static_cast<GDALDataset *>(GDALOpenShared(datasetPath.c_str(), GA_ReadOnly));
					if (poDataset != NULL)
					{
						DatasetStruct datasetStruct;
						datasetStruct.width = poDataset->GetRasterXSize();
						datasetStruct.height = poDataset->GetRasterYSize();
						datasetStruct.bandCount = poDataset->GetRasterCount();
						datasetStruct.datasetPath = datasetPath;
						//TODO: here only use the first band's datatype. need to consider all bands.
						datasetStruct.dataTypeSizeInBits = GDALGetDataTypeSize(poDataset->GetRasterBand(1)->GetRasterDataType());
						outputMutex.lock();
						printf("Path: %s, Width: %d, Height: %d, BandCount: %d", 
							datasetStruct.datasetPath.c_str(), datasetStruct.width, datasetStruct.height, datasetStruct.bandCount);
						double geoTransform[6];
						if (poDataset->GetGeoTransform(geoTransform) == CE_None)
						{
							for (auto i = 0; i < 6; i++)
								datasetStruct.geoTransformParams.push_back(geoTransform[i]);
							printf(" GeoTransform: %f, %f, %f, %f, %f, %f\n", geoTransform[0], geoTransform[1], geoTransform[2], 
								geoTransform[3], geoTransform[4], geoTransform[5]);
						}
						else
						{
							printf("\n");
						}
						OGRSpatialReference ogr(poDataset->GetProjectionRef());
					
						auto authorityName = ogr.GetAuthorityName("PROJCS");
						if (authorityName != NULL)
						{
							auto authorityCode = ogr.GetAuthorityCode("PROJCS");
							datasetStruct.spatialId.append(authorityName).append(":")
								.append(authorityCode);
						}
						datasetStruct.updatedTime = std::time(nullptr);
						outputMutex.unlock();
						//generate thumbnails
						std::string thumbnailPath = "c:\\datasets\\thumbnail\\";
						thumbnailPath += std::experimental::filesystem::path(datasetPath).stem().string();
						thumbnailPath.append(".png");
						int thumbnailRatioScaleRatio = ceil(datasetStruct.width / thumbnailMaxWidth);
						gdal_translate((GDALDatasetH)poDataset, datasetStruct.width / thumbnailRatioScaleRatio, datasetStruct.height / thumbnailRatioScaleRatio, thumbnailPath);
						s_catalogDB->InsertOrUpdateDataset(datasetStruct);
						GDALClose((GDALDatasetH)poDataset);
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
					if (crawlStatus == QueueProcessStatus::CrawlFinished)
					{
						crawlStatus = QueueProcessStatus::ProcessingLeftover;
					}
					else if (crawlStatus == QueueProcessStatus::ProcessingLeftover)
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
	crawlStatus = QueueProcessStatus::CrawlFinished;
	for (auto& th : threadVec)
		th.join();
	delete s_catalogDB;
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