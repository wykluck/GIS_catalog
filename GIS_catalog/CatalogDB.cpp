#include "CatalogDB.h"
#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <algorithm>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

CatalogDB::CatalogDB()
{
	//m_pInstance = new mongocxx::instance();// This should be done only once.
	//mongocxx::client client{ mongocxx::uri{} };
	//m_pDB = client["CatalogDB"];
}

CatalogDB::~CatalogDB()
{
	delete m_pInstance;
	delete m_pDB;
}

bool CatalogDB::InsertOrUpdateDataset(const DatasetStruct& datasetStruct)
{
	auto builder = bsoncxx::builder::stream::document{};
	auto in_array = builder
		<< "filePath" << datasetStruct.filePath
		<< "width" << datasetStruct.width
		<< "height" << datasetStruct.height
		<< "bandCount" << datasetStruct.bandCount
		<< "cellType" << datasetStruct.cellType
		<< "spatialId" << datasetStruct.spatialId
		<< "geoTransformParams" << bsoncxx::builder::stream::open_array;
	std::for_each(datasetStruct.geoTransformParams.begin(), datasetStruct.geoTransformParams.end(), [&](double param) {
		in_array << bsoncxx::types::b_double{ param };
	});
	auto docValue = in_array << bsoncxx::builder::stream::close_array << bsoncxx::builder::stream::finalize;
	mongocxx::collection datasetCollection = (*m_pDB)["Dataset"];
	datasetCollection.insert_one(docValue.view());


	return true;
}
