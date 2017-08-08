#pragma once
#include "DatasetStruct.h"
#include "Utilities.h"
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>


class CatalogDB
{
public:
	CatalogDB();
	~CatalogDB();
	bool InsertOrUpdateDataset(const DatasetStruct& datasetStruct, 
		time64_t lastModifiedTime, const std::vector<unsigned char>& thumbnailBuffer);
	time64_t getDatasetLastModifiedTime(const std::string datasetPath);
private:
	mongocxx::instance  m_instance;
	//the pool is recommended by mongocxx driver to handle multi-threading
	mongocxx::uri m_uri;
	mongocxx::pool m_pool;
};