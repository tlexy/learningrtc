#include <iostream>
#include <stdio.h>
#include "common_log.h"

namespace log4u
{
	void log(const char* text)
	{
		std::cout << text << " Line: " << __LINE__ << " File: " << __FILE__ << std::endl;
	}

	void loge(const char* text)
	{
		std::cout << "[ERROR]" << text << " Line: " << __LINE__ << " File: " << __FILE__ << std::endl;
	}

	//void log_i(const char* format, const char* file, int line, ...)
	//{
	//	printf("File: %s, Line[]: %d", file, line);
	//}
}