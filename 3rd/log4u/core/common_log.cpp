#include <iostream>
#include <stdio.h>
#include "common_log.h"

namespace log4u
{
	void log(const char* text, const char* file, int line)
	{
		std::cout << text << " Line: " << file << " File: " << line << std::endl;
	}

	void loge(const char* text, const char* file, int line)
	{
		std::cout << "[ERROR]" << text << " Line: " << file << " File: " << line << std::endl;
	}
}