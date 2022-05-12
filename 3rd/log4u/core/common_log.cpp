#include <iostream>
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
}