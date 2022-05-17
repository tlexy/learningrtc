#ifndef LOG4U_COMMON_LOG_H
#define LOG4U_COMMON_LOG_H

#define log_info(format) log4u::log(format, __FILE__, __LINE__)
#define log_error(format) log4u::loge(format, __FILE__, __LINE__)

namespace log4u
{
	void log(const char*, const char* file, int line);
	void loge(const char*, const char* file, int line);
}

#endif // !LOG4U_COMMON_LOG_H
