#include "Utilities.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <memory>
#include "spdlog/spdlog.h"
#include "gdal.h"

#ifdef WIN32
#define stat _stat64
#endif

extern std::shared_ptr<spdlog::logger> logger;
Utilities::Utilities()
{
}


Utilities::~Utilities()
{
}

FileStats Utilities::GetFileStats(const std::string& filePath)
{
	struct stat result;
	FileStats fileStats = { 0, 0 };
	if (stat(filePath.c_str(), &result) == 0)
	{
		fileStats.lastModifiedTime = result.st_mtime;
		fileStats.fileSize = result.st_size;
	}
	return fileStats;
}

void CPL_STDCALL Utilities::GDALErrorLogger(CPLErr eErrClass, int nError, const char * pszErrorMsg)
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



