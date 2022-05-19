#include <iostream>
#include <stdio.h>
#include "common_log.h"
//#include <windows.h>
//
//#define LOG_LEVEL_FATAL 7
//
//#ifdef WIN32
//const static unsigned short LOG_COLOR[LOG_LEVEL_FATAL + 1] = {
//    0,
//    0,
//    FOREGROUND_BLUE | FOREGROUND_GREEN,
//    FOREGROUND_GREEN | FOREGROUND_RED,
//    FOREGROUND_RED,
//    FOREGROUND_GREEN,
//    FOREGROUND_RED | FOREGROUND_BLUE };
//#else
//
//const static char LOG_COLOR[LOG_LEVEL_FATAL + 1][50] = {
//    "\e[0m",
//    "\e[0m",
//    "\e[34m\e[1m",//hight blue
//    "\e[33m", //yellow
//    "\e[31m", //red
//    "\e[32m", //green
//    "\e[35m" };
//#endif

namespace log4u
{
	void log(const char* text, const char* file, int line)
	{
		std::cout << text << " Line: " << line << " File: " << file << std::endl;
	}

	void loge(const char* text, const char* file, int line)
	{
		std::cout << "[ERROR]" << text << " Line: " << line << " File: " << file << std::endl;
	}
}