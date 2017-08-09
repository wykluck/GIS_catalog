#pragma once
#include <string>
#include "cpl_config.h"
#define time64_t __time64_t
class Utilities
{
public:
	Utilities();
	static time64_t GetLastModifiedTime(const std::string& filePath);
	static CPL_STDCALL GDALErrorLogger(CPLErr eErrClass, int nError, const char * pszErrorMsg);
	virtual ~Utilities();
};

