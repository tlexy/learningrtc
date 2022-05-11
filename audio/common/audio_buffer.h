#pragma once

#include <stdint.h>

class AudioBuffer
{
public:
	AudioBuffer(size_t size);
	~AudioBuffer();

	size_t push(const uint8_t* data, int len);
	uint8_t* read_ptr();
	uint8_t* write_ptr();
	//可用数据大小
	int readable_size();
	//剩余空间大小
	int writable_size();
	//读取了多少字节的数据
	void has_read(int len);

	int read(uint8_t* dst, int len);

	void has_written(int size);

	void resize(size_t new_len);
	void reset();

private:
	uint8_t* _buf{NULL};
	uint8_t* _temp_buf{NULL};
	int _size;
	int _read_pos;
	int _write_pos;
};

