#include "sutil.h"
#include <cstdlib>
#include <time.h>
#include <chrono>

#ifndef _WIN32
#include<sys/time.h>
#include <unistd.h>
#endif

#define Between(X, L, H) \
	(((X) >= (L) && (X) <= (H)))

namespace SUtil
{
	const static char kHexStr[] = { '0','1','2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	const char* enc_key = "12345678asdflkjh";

#ifndef _WIN32
	static struct timeval gtv;
#endif

	unsigned int getCurrentTime()
	{
		time_t t = time(NULL);
		return t;
	}

	std::string trim(const std::string& str)
	{
		int i, j;
		for (i = 0; i < str.size(); ++i)
		{
			if (str[i] != ' ')
			{
				break;
			}
		}
		for (j = str.size() - 1; j >= 0; --j)
		{
			if (str[j] != ' ')
			{
				break;
			}
		}
		return str.substr(i, j + 1);
	}

	//去掉空格和换行
	std::string toCompactString(const std::string &str)
	{
		bool in_string = false;
		std::string tmp;
		for (std::string::const_iterator iter = str.begin(); iter != str.end(); iter++)
		{
			char ch = *iter;
			int a = (unsigned)(ch + 1);
			if (Between(a, 0, 256) && isspace(ch) && in_string == false)
			{
				continue;
			}

			tmp.insert(tmp.end(), ch);
			if (ch == '\"')
			{
				in_string = !in_string;
			}
		}
		return tmp;
	}

	void toCompactStringWith(const std::string &in_str, std::string& out_str, size_t header_len)
	{
		out_str.reserve(header_len + in_str.size() / 2);
		for (int i = 0; i < header_len; ++i)
		{
			out_str.push_back('\0');
		}
		bool in_string = false;
		for (int i = 0; i < in_str.size(); ++i)
		{
			if (isspace(in_str[i]) && in_string == false)
			{
				continue;
			}
			if ((in_str[i] == '\n' || in_str[i] == '\t') && in_string == false)
			{
				continue;
			}
			out_str.push_back(in_str[i]);
			if (in_str[i] == '\"')
			{
				in_string = !in_string;
			}
		}
	}

	size_t getFileLength(std::ifstream &_f)
	{
		using namespace std;
		ifstream::pos_type old = _f.tellg();
		_f.seekg(0, ifstream::beg);
		ifstream::pos_type start = _f.tellg();
		_f.seekg(0, ifstream::end);
		ifstream::pos_type end = _f.tellg();
		_f.seekg(old);
		return end - start;
	}

	size_t readAll(std::ifstream &_ifs, char* _buff, int _buff_len)
	{
		std::filebuf* pbuf = _ifs.rdbuf();

		std::size_t size = pbuf->pubseekoff(0, _ifs.end, _ifs.in);
		pbuf->pubseekpos(0, _ifs.in);
		if (size > _buff_len)
		{
			return -1;
		}
		pbuf->sgetn(_buff, size);
		return size;
	}

	size_t readTextFile(const char* path, std::string& buf)
	{
		std::ifstream ifile(path);
		if (!ifile)
		{
			return 0;
		}
		size_t len = getFileLength(ifile);
		char* cbuf = (char*)malloc(len);
		int ret_len = readAll(ifile, cbuf, len);
		buf.append(cbuf, cbuf + ret_len);
		free(cbuf);
		return ret_len;
	}

	size_t readBinaryFile(const char* path, std::string& buf)
	{
		std::ifstream ifile(path, std::ifstream::binary);
		if (!ifile)
		{
			return 0;
		}
		size_t len = getFileLength(ifile);
		buf.resize(len);
		char* cbuf = (char*)buf.data();
		return readAll(ifile, cbuf, len);
	}

	std::string toWinStylePath(const std::string path)
	{
		std::string temp = path;
		size_t pos = 0;
		size_t off = 0;
		while ((pos = temp.find('/', off)) != std::string::npos)
		{
			temp.replace(pos, 1, "\\");
			off = pos + 2;
		}
		return temp;
	}

	void writeFileBinary(const char* path, const char* buf, size_t len)
	{
		std::ofstream of(path, std::ostream::out | std::ostream::binary);
		if (!of.is_open())
		{
			return;
		}
		of.write(buf, len);
		of.close();
	}

	static const uint16_t crc16tab[256] = {
		0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
		0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
		0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
		0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
		0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
		0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
		0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
		0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
		0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
		0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
		0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
		0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
		0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
		0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
		0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
		0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
		0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
		0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
		0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
		0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
		0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
		0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
		0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
		0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
		0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
		0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
		0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
		0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
		0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
		0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
		0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
		0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
	};

	uint16_t crc16(const char *buf, int len)
	{
		int counter;
		uint16_t crc = 0;
		for (counter = 0; counter < len; counter++)
			crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *buf++) & 0x00FF];
		return crc;
	}

	std::string str2Hex(const char* str, unsigned int len)
	{
		std::string outstr = "";
		for (unsigned int i = 0; i < len; i++) 
		{
			char tmp[8];
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp) - 1, "%02x", (unsigned char)str[i]);
			std::string tmpstr = tmp;
			outstr = outstr + tmpstr;

		}
		return outstr;
	}

	std::string hex2Str(const char* str, unsigned int len)
	{
		std::string outbuffer;
		const char *ptr = str;
		unsigned int  length = len / 2;

		char *p = NULL;
		for (unsigned int i = 0; i< length; i++) 
		{
			char tmp[4];
			memset(tmp, 0, sizeof(tmp));
			tmp[0] = ptr[i * 2];
			tmp[1] = ptr[i * 2 + 1];
			outbuffer.push_back((char)strtol(tmp, &p, 16));
		}
		return outbuffer;
	}

	std::string buffer2Hex(const char* _buffer, size_t _len)
	{
		std::string hex;
		if (_buffer == NULL || _len == 0)
		{
			return hex;
		}
		for (size_t i = 0; i < _len; ++i)
		{
			char c1 = _buffer[i];
			int index1 = (c1 & 0xf0) >> 4;
			int index2 = c1 & 0x0f;
			hex.push_back(kHexStr[index1]);
			hex.push_back(kHexStr[index2]);
		}
		return hex;
	}

	std::string toDateTimeString(const time_t time)
	{
		tm *tm_ = localtime(&time);                // 将time_t格式转换为tm结构体
		int year, month, day, hour, minute, second;// 定义时间的各个int临时变量。
		year = tm_->tm_year + 1900;                // 临时变量，年，由于tm结构体存储的是从1900年开始的时间，所以临时变量int为tm_year加上1900。
		month = tm_->tm_mon + 1;                   // 临时变量，月，由于tm结构体的月份存储范围为0-11，所以临时变量int为tm_mon加上1。
		day = tm_->tm_mday;                        // 临时变量，日。
		hour = tm_->tm_hour;                       // 临时变量，时。
		minute = tm_->tm_min;                      // 临时变量，分。
		second = tm_->tm_sec;                      // 临时变量，秒。
		char yearStr[5], monthStr[3], dayStr[3], hourStr[3], minuteStr[3], secondStr[3];// 定义时间的各个char*变量。
		sprintf(yearStr, "%d", year);              // 年。
		sprintf(monthStr, "%d", month);            // 月。
		sprintf(dayStr, "%d", day);                // 日。
		sprintf(hourStr, "%d", hour);              // 时。
		sprintf(minuteStr, "%d", minute);          // 分。
		if (minuteStr[1] == '\0')                  // 如果分为一位，如5，则需要转换字符串为两位，如05。
		{
			minuteStr[2] = '\0';
			minuteStr[1] = minuteStr[0];
			minuteStr[0] = '0';
		}
		sprintf(secondStr, "%d", second);          // 秒。
		if (secondStr[1] == '\0')                  // 如果秒为一位，如5，则需要转换字符串为两位，如05。
		{
			secondStr[2] = '\0';
			secondStr[1] = secondStr[0];
			secondStr[0] = '0';
		}
		char s[20];                                // 定义总日期时间char*变量。
		sprintf(s, "%s-%s-%s %s:%s:%s", yearStr, monthStr, dayStr, hourStr, minuteStr, secondStr);// 将年月日时分秒合并。
		std::string str(s);                             // 定义string变量，并将总日期时间char*变量作为构造函数的参数传入。
		return str;
	}

	int64_t getTimeStampMilli()
	{
#ifdef _WIN32
		std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
		auto tt = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
		return tt.count();
#else
		gettimeofday(&gtv, NULL);
		int64_t micros = gtv.tv_sec * 1000 + gtv.tv_usec / 1000;
		return micros;
#endif
	}

	void split(const std::string& text, const std::string& splitter, std::vector<std::string>& vecs)
	{
		size_t offset = 0;
		size_t pos = text.find(splitter);
		while (pos != std::string::npos)
		{
			if (pos - offset > 0)
			{
				vecs.push_back(text.substr(offset, pos - offset));
			}
			offset = pos + splitter.size();
			pos = text.find(splitter, offset);
		}
		if (offset < text.size())
		{
			vecs.push_back(text.substr(offset));
		}
	}

	bool readJson(const char* filename, Json::Value& json)
	{
		std::string buf;
		size_t len = readBinaryFile(filename, buf);
		if (len <= 0)
		{
			return false;
		}
		return parseJson(buf.c_str(), json);
	}

	bool parseJson(const char* text, Json::Value& json)
	{
		Json::Reader reader(Json::Features::strictMode());
		char* temp = (char*)text;
		//处理带BOM的UTF-8编码
		if ((0xef == (unsigned char)text[0]) && (0xbb == (unsigned char)text[1]) && (0xbf == (unsigned char)text[2]))
		{
			temp = temp + 3;
		}
		if (strlen(temp) <= 0)
		{
			return false;
		}
		else
		{
			if (reader.parse(temp, json))
			{
				return true;
			}
		}
		return false;
	}

	bool checkJsonConfig(const Json::Value& json, const std::string& key)
	{
		if (json[key].isNull())
		{
			return false;
		}
		return true;
	}

	void savePidToFile(const char* filename)
	{
#ifndef _WIN32
		int pid = (int)getpid();
		std::ofstream of(filename, std::ostream::out | std::ostream::binary);
		if (!of.is_open())
		{
			return;
		}
		of << pid << std::endl;
		of.close();
#endif
	}

	void removeFile(const char* filename)
	{
		remove(filename);
	}
}
