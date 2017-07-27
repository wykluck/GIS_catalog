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

CatalogDB::CatalogDB()
{
	mongocxx::client client{ mongocxx::uri{} };
	m_db = client["CatalogDB"];
}

CatalogDB::~CatalogDB()
{
}

bool CatalogDB::InsertOrUpdateDataset(const DatasetStruct& datasetStruct)
{
	auto builder = bsoncxx::builder::stream::document{};
	auto in_array = builder
		<< "filePath" << datasetStruct.datasetPath
		<< "width" << datasetStruct.width
		<< "height" << datasetStruct.height
		<< "bandCount" << datasetStruct.bandCount
		<< "cellType" << datasetStruct.cellType
		<< "spatialId" << datasetStruct.spatialId
		<< "geoTransformParams" << bsoncxx::builder::stream::open_array;
	std::for_each(datasetStruct.geoTransformParams.begin(), datasetStruct.geoTransformParams.end(), [&](double param) {
		in_array << bsoncxx::types::b_double{ param };
	});
	auto docValue = in_array 
		<< bsoncxx::builder::stream::close_array
		<< bsoncxx::builder::stream::finalize;
	mongocxx::collection datasetCollection = m_db["Dataset"];
	datasetCollection.insert_one(docValue.view());


	return true;
}
