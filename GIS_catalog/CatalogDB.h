#pragma once
#include "DatasetStruct.h"
#include "Utilities.h"
#include <mongocxx/pool.hpp>
#include <mongocxx/instance.hpp>
namespace spdlog
{
	class logger;
}

class CatalogDB
{
public:
	CatalogDB(const std::shared_ptr<spdlog::logger>& logger);
	~CatalogDB();
	bool InsertOrUpdateDataset(const DatasetStruct& datasetStruct, 
		const FileStats& fileStats, const std::vector<unsigned char>& thumbnailBuffer);
	time64_t getDatasetLastModifiedTime(const std::string datasetPath);
private:
	mongocxx::instance  m_instance;
	//the pool is recommended by mongocxx driver to handle multi-threading
	mongocxx::uri m_uri;
	mongocxx::pool m_pool;
	std::shared_ptr<spdlog::logger> m_logger;

};