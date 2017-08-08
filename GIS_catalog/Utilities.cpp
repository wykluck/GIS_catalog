#include "Utilities.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#define stat _stat
#endif



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




