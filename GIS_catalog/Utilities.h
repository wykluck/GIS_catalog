#pragma once
#include <string>
#define time64_t __time64_t
class Utilities
{
public:
	Utilities();
	static time64_t GetLastModifiedTime(const std::string& filePath);
	virtual ~Utilities();
};

