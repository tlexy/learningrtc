#ifndef LOG4U_COMMON_LOG_H
#define LOG4U_COMMON_LOG_H

//#define log_info(format, ...) log4u::log_i(format, __FILE__, __LINE__, ##__VA_ARGS__)

namespace log4u
{
	void log(const char*);
	void loge(const char*);

	//void log_i(const char* format, const char* file, int line, ...);
}

#endif // !LOG4U_COMMON_LOG_H
