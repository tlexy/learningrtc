#pragma once

#include <stdint.h>

class mid_buf
{
public:
	mid_buf(size_t size);
	~mid_buf();

	size_t push(const uint8_t* data, int len);
	uint8_t* data();
	//可用数据大小
	int usable_count();
	//剩余空间大小
	int empty_count();
	//读取了多少字节的数据
	void set_read(int len);

	void resize(size_t new_len);
	void reset();

private:
	uint8_t* _buf{NULL};
	uint8_t* _temp_buf{NULL};
	int _size;
	int _read_pos;
	int _write_pos;
};

