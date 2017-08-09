﻿#include <thread>
#include <mutex>
#include <chrono>
#include <experimental/filesystem>
#include "grfmt_gdal.hpp"
#include "concurrentqueue.h"
#include "DatasetStruct.h"
#include "CatalogDB.h"
#include "Utilities.h"
#include "spdlog/spdlog.h"
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

std::shared_ptr<spdlog::logger> logger;

NAN_METHOD(Init) {
	//TODO: GDAL plugin dir and data dir should be passed, disable it for now
	//LoadLibrary("C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\NCSEcwd.dll");
	//CPLSetConfigOption("GDAL_DRIVER_PATH", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\Debug\\gdalplugins");
	//CPLSetConfigOption("GDAL_DATA", "C:\\Users\\ywang\\Documents\\Visual Studio 2015\\Projects\\GIS_catalog\\GIS_catalog\\build\\data");
	if (!info[0]->IsString())
	{
		Nan::ThrowTypeError("Wrong arguments at logFilePath");
		return;
	}
	v8::String::Utf8Value temp(info[0]);
	std::string logFilePath = *temp;

	//std::this_thread::sleep_for(std::chrono::milliseconds(14000));
	// Create a file rotating logger with 5mb size max and 3 rotated files
	logger = spdlog::rotating_logger_mt("rotate_logger", logFilePath, 1048576 * 5, 3);
	GDALAllRegister();
	CPLSetErrorHandler(&Utilities::GDALErrorLogger);
	//CPLSetConfigOption("CPL_LOG", logFilePath.c_str());
	logger->flush_on(spdlog::level::info);
	s_catalogDB = new CatalogDB(logger);
	
	
}

NAN_METHOD(BeginUpdate) {
	if (!info[0]->IsBoolean())
	{
		Nan::ThrowTypeError("Wrong arguments at forceUpdate");
		return;
	}
	s_forceUpdate = info[0]->BooleanValue();
	crawlStatus = QueueProcessStatus::NotStarted;
}


NAN_METHOD(UpdateDatasetInfo) {
	if (!info[0]->IsString())
	{
		Nan::ThrowTypeError("Wrong arguments at datasetPath");
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
					try
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
						else
						{
							logger->error("Problem occurs when opening dataset at ({0})", datasetPath.c_str());
						}
					}
					catch (std::exception& ex)
					{
						logger->error("Problem occurs when processing dataset at ({0}) with error ({1}).", datasetPath.c_str(),
							ex.what());
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
	NAN_EXPORT(target, Init);
	NAN_EXPORT(target, BeginUpdate);
	NAN_EXPORT(target, UpdateDatasetInfo);
	NAN_EXPORT(target, EndUpdate);
}

// Create the module called "addon" and initialize it with `Initialize` function (created with NAN_MODULE_INIT macro)
NODE_MODULE(addon, Initialize);