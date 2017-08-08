#include <thread>
#include <mutex>
#include <chrono>
#include <experimental/filesystem>
#include "grfmt_gdal.hpp"
#include "concurrentqueue.h"
#include "DatasetStruct.h"
#include "CatalogDB.h"
#include "Utilities.h"
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
static CatalogDB *s_catalogDB;
static bool s_forceUpdate = false;


NAN_METHOD(GdalInit) {
	//TODO: GDAL plugin dir and data dir should be passed, disable it for now
	//LoadLibrary("C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\NCSEcwd.dll");
	//CPLSetConfigOption("GDAL_DRIVER_PATH", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\gdalplugins");
	//CPLSetConfigOption("GDAL_DATA", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\data");
	GDALAllRegister();
	s_catalogDB = new CatalogDB();
	//std::this_thread::sleep_for(std::chrono::milliseconds(14000));
}

NAN_METHOD(BeginUpdate) {
	if (!info[0]->IsBoolean())
	{
		Nan::ThrowTypeError("Wrong arguments");
		return;
	}
	s_forceUpdate = info[0]->BooleanValue();
	crawlStatus = QueueProcessStatus::NotStarted;
}


NAN_METHOD(UpdateDatasetInfo) {
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
					auto lastModifiedTime = Utilities::GetLastModifiedTime(datasetPath);
					if (!s_forceUpdate)
					{
						if (s_catalogDB->getDatasetLastModifiedTime(datasetPath) >= lastModifiedTime)
						{
							//if forceUpdate is false and datasetUpdatetime is late than the file's
							//last_modification_time, just skip without update
							continue;
						}
					}
					
					cvGIS::GdalDecoder gdalDecoder;
					gdalDecoder.setSource(datasetPath);
					if (gdalDecoder.readHeader())
					{
						const DatasetStruct& datasetStruct = gdalDecoder.getMetaData();
						std::vector<uchar> thumbnailBuffer;
						int thumbnailRatioScaleRatio = ceil(datasetStruct.width / thumbnailMaxWidth);
						gdalDecoder.generateThumbnail(datasetStruct.width / thumbnailRatioScaleRatio, datasetStruct.height / thumbnailRatioScaleRatio,
							thumbnailBuffer);
						s_catalogDB->InsertOrUpdateDataset(datasetStruct, lastModifiedTime, thumbnailBuffer);
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

NAN_METHOD(EndUpdate) {
	crawlStatus = QueueProcessStatus::CrawlFinished;
	for (auto& th : threadVec)
		th.join();
	threadVec.clear();
	s_forceUpdate = false;
	return;
}

// Module initialization logic
NAN_MODULE_INIT(Initialize) {
	// Export the `GdalInit` function (equivalent to `export function Hello (...)` in JS)
	NAN_EXPORT(target, GdalInit);
	NAN_EXPORT(target, BeginUpdate);
	NAN_EXPORT(target, UpdateDatasetInfo);
	NAN_EXPORT(target, EndUpdate);
}

// Create the module called "addon" and initialize it with `Initialize` function (created with NAN_MODULE_INIT macro)
NODE_MODULE(addon, Initialize);