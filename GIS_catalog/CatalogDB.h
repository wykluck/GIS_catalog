#pragma once
#include "DatasetStruct.h"
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>


class CatalogDB
{
public:
	CatalogDB();
	~CatalogDB();
	bool InsertOrUpdateDataset(const DatasetStruct& datasetStruct);
private:
	mongocxx::database	m_db;
	mongocxx::instance  m_instance;

};