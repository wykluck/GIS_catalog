#include "CatalogDB.h"
#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include "spdlog/spdlog.h"

#include <algorithm>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

CatalogDB::CatalogDB(const std::shared_ptr<spdlog::logger>& logger) 
	:m_pool(mongocxx::uri{}), m_logger(logger)
{
	m_logger->info("Connected to the database successfully.\n");
}

CatalogDB::~CatalogDB()
{
	m_logger->info("Disconnected to the database successfully.\n");
}

time64_t CatalogDB::getDatasetLastModifiedTime(const std::string datasetPath)
{
	auto connection = m_pool.acquire();
	auto datasetCollection = (*connection)["CatalogDB"]["Dataset"];
	bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result =
		datasetCollection.find_one( document{} << "filePath" << bsoncxx::types::b_utf8(datasetPath) << finalize);
	if (maybe_result) {
		auto lastModifiedTime = maybe_result->view()["lastModifiedTime"];
		return lastModifiedTime.get_int64();
	}
	else
		return 0;
}

void InsertPolygon(const std::string& polygonName, const std::vector<cv::Point2d>& polygon, bsoncxx::builder::stream::key_context< bsoncxx::builder::stream::key_context<> >& previousContext )
{
	document polygonDoc;
	auto tempArr = polygonDoc << "type" << bsoncxx::types::b_utf8("Polygon")
		<< "coordinates" << bsoncxx::builder::stream::open_array;
	for (auto const& point : polygon)
	{
		tempArr << bsoncxx::builder::stream::open_array
			<< bsoncxx::types::b_double{ point.x } << bsoncxx::types::b_double{ point.y } << bsoncxx::builder::stream::close_array;
	}
	tempArr << bsoncxx::builder::stream::close_array;
	previousContext << polygonName << polygonDoc;
}

bool CatalogDB::InsertOrUpdateDataset(const DatasetStruct& datasetStruct, const FileStats& fileStats,  const std::vector<unsigned char>& thumbnailBuffer)
{
	auto builder = document{};
	auto documentStream = builder << "$set" << open_document
		<< "filePath" << bsoncxx::types::b_utf8(datasetStruct.datasetPath)
		<< "width" << datasetStruct.width
		<< "height" << datasetStruct.height
		<< "bandCount" << datasetStruct.bandCount
		<< "dataTypeSizeInBits" << datasetStruct.dataTypeSizeInBits
		<< "spatialId" << bsoncxx::types::b_utf8(datasetStruct.spatialId)
		<< "units" << bsoncxx::types::b_utf8(datasetStruct.units)
		<< "fileSize" << fileStats.fileSize
		<< "lastModifiedTime" << fileStats.lastModifiedTime;
	if (!datasetStruct.geoTransformParams.empty())
	{
		bsoncxx::builder::stream::array geoTransformArray;

		std::for_each(datasetStruct.geoTransformParams.begin(), datasetStruct.geoTransformParams.end(), [&](double param) {
			geoTransformArray << bsoncxx::types::b_double{ param };
		});
		documentStream << "geoTransformParams" << geoTransformArray;
	}

	if (!datasetStruct.nativeBoundPolygon.empty())
	{
		InsertPolygon("nativeBoundPolygon", datasetStruct.geodeticBoundPolygon, documentStream);
	}
	if (!datasetStruct.geodeticBoundPolygon.empty())
	{
		InsertPolygon("geodeticBoundPolygon", datasetStruct.geodeticBoundPolygon, documentStream);
	}
	if (!datasetStruct.googleBoundPolygon.empty())
	{
		InsertPolygon("googleBoundPolygon", datasetStruct.googleBoundPolygon, documentStream);
	}
	
	if (!thumbnailBuffer.empty())
	{
		bsoncxx::types::b_binary thumbnailBinary;
		thumbnailBinary.sub_type = bsoncxx::binary_sub_type::k_binary;
		thumbnailBinary.bytes = thumbnailBuffer.data();
		thumbnailBinary.size = thumbnailBuffer.size();
		documentStream << "thumbnail" << thumbnailBinary;
	}
	
	auto docValue = documentStream << close_document;
	auto connection = m_pool.acquire();
	auto datasetCollection = (*connection)["CatalogDB"]["Dataset"];
	mongocxx::options::update updateOptions;
	updateOptions.upsert(true);
	auto filterDocument = document{};
	filterDocument << "filePath" << bsoncxx::types::b_utf8(datasetStruct.datasetPath);
	datasetCollection.update_one(filterDocument.view(), builder.view(), updateOptions);
	return true;
}
