#include "Utilities.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <memory>
#include "spdlog/spdlog.h"

#ifdef WIN32
#define stat _stat
#endif

extern std::shared_ptr<spdlog::logger> logger;

Utilities::Utilities()
{
}


Utilities::~Utilities()
{
}

time64_t Utilities::GetLastModifiedTime(const std::string& filePath)
{
	struct stat result;
	if (stat(filePath.c_str(), &result) == 0)
	{
		return result.st_mtime;
	}
	return 0;
}


static void CPL_STDCALL GDALErrorLogger(CPLErr eErrClass, int nError, const char * pszErrorMsg)
{
	static int       nCount = 0;
	static int       nMaxErrors = 1000;

	if (eErrClass != CE_Debug)
	{
		nCount++;
		if (nCount > nMaxErrors)
			return;
	}

	if (eErrClass == CE_Debug) {
		logger->debug(("GDAL DEBUG: {0}"), pszErrorMsg);
	}
	else if (eErrClass == CE_Warning) {
		logger->warn(("GDAL WARNING: {0}"), pszErrorMsg);
	}
	else {
		logger->error(("GDAL ERROR: {0}"), pszErrorMsg);
	}

}



