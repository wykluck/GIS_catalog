#include "CatalogDB.h"
#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <mongocxx/uri.hpp>

#include <algorithm>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

CatalogDB::CatalogDB():m_client(mongocxx::uri{})
{
}

CatalogDB::~CatalogDB()
{
}

bool CatalogDB::InsertOrUpdateDataset(const DatasetStruct& datasetStruct, const std::vector<unsigned char>& thumbnailBuffer)
{
	auto builder = bsoncxx::builder::stream::document{};
	auto in_array = builder
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
	auto docValue = closed_array
		<< bsoncxx::builder::stream::finalize;
	auto datasetCollection = m_client["CatalogDB"]["Dataset"];
	datasetCollection.insert_one(docValue.view());


	return true;
}
