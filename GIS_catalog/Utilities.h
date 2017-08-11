#pragma once
#include <string>
#include "gdal.h"

#ifdef WIN32
#define time64_t __time64_t
#endif

struct FileStats
{
	time64_t lastModifiedTime;
	long long fileSize;
};

class Utilities
{
public:
	Utilities();
	static FileStats GetFileStats(const std::string& filePath);
	static void CPL_STDCALL GDALErrorLogger(CPLErr eErrClass, int nError, const char * pszErrorMsg);
	virtual ~Utilities();
};


