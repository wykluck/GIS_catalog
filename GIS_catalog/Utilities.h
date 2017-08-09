#pragma once
#include <string>
#include "gdal.h"

#ifdef WIN32
#define time64_t __time64_t
#endif

class Utilities
{
public:
	Utilities();
	static time64_t GetLastModifiedTime(const std::string& filePath);
	static void CPL_STDCALL GDALErrorLogger(CPLErr eErrClass, int nError, const char * pszErrorMsg);
	virtual ~Utilities();
};


