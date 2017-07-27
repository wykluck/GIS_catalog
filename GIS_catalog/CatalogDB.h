#pragma once
#include "DatasetStruct.h"

namespace mongocxx {
	class database;
	class instance;
};

class CatalogDB
{
public:
	CatalogDB();
	~CatalogDB();
	bool InsertOrUpdateDataset(const DatasetStruct& datasetStruct);
private:
	mongocxx::database	*m_pDB;
	mongocxx::instance  *m_pInstance;

};