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

#include <algorithm>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

CatalogDB::CatalogDB():m_pool(m_uri)
{
	printf("Connected to the database successfully at %s\n", m_uri.to_string().c_str());
}

CatalogDB::~CatalogDB()
{
	printf("Disconnected to the database successfully at %s\n", m_uri.to_string().c_str());
}

bool CatalogDB::InsertOrUpdateDataset(const DatasetStruct& datasetStruct, const std::vector<unsigned char>& thumbnailBuffer)
{
	auto builder = bsoncxx::builder::stream::document{};
	auto in_array = builder << "$set" << open_document
		<< "filePath" << bsoncxx::types::b_utf8(datasetStruct.datasetPath)
		<< "width" << datasetStruct.width
		<< "height" << datasetStruct.height
		<< "bandCount" << datasetStruct.bandCount
		<< "dataTypeSizeInBits" << datasetStruct.dataTypeSizeInBits
		<< "spatialId" << bsoncxx::types::b_utf8(datasetStruct.spatialId)
		<< "updatedTime" << datasetStruct.updatedTime
		<< "geoTransformParams" << bsoncxx::builder::stream::open_array;
	std::for_each(datasetStruct.geoTransformParams.begin(), datasetStruct.geoTransformParams.end(), [&](double param) {
		in_array << bsoncxx::types::b_double{ param };
	});
	auto closed_array = in_array << bsoncxx::builder::stream::close_array;
	if (!thumbnailBuffer.empty())
	{
		bsoncxx::types::b_binary thumbnailBinary;
		thumbnailBinary.sub_type = bsoncxx::binary_sub_type::k_binary;
		thumbnailBinary.bytes = thumbnailBuffer.data();
		thumbnailBinary.size = thumbnailBuffer.size();
		closed_array << "thumbnail" << thumbnailBinary;
	}
	auto docValue = closed_array << close_document;
	auto connection = m_pool.acquire();
	auto datasetCollection = (*connection)["CatalogDB"]["Dataset"];
	mongocxx::options::update updateOptions;
	updateOptions.upsert(true);
	auto filterDocument = document{};
	filterDocument << "filePath" << bsoncxx::types::b_utf8(datasetStruct.datasetPath);
	datasetCollection.update_one(filterDocument.view(), builder.view(), updateOptions);
	return true;
}
