#include "audio_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

AudioBuffer::AudioBuffer(size_t size)
	:_size(size),
	_read_pos(0),
	_write_pos(0)
{
	_buf = (uint8_t*)malloc(_size);
	_temp_buf = (uint8_t*)malloc(_size);
}

void AudioBuffer::reset()
{
	_read_pos = 0;
	_write_pos = 0;
}

size_t AudioBuffer::push(const uint8_t* data, int len)
{
	if (writable_size() < len)
	{
		//重新整理内存
		printf("rearrange buffer...");
		memcpy(_temp_buf, _buf + _read_pos, readable_size());
		memcpy(_buf, _temp_buf, readable_size());
		_write_pos = readable_size();
		_read_pos = 0;
	}
	//
	if (writable_size() >= len)
	{
		memcpy(_buf + _write_pos, data, len);
		_write_pos += len;
		return len;
	}
	return 0;
}

uint8_t* AudioBuffer::read_ptr()
{
	return _buf + _read_pos;
}

uint8_t* AudioBuffer::write_ptr()
{
	return _buf + _write_pos;
}

int AudioBuffer::writable_size()
{
	return _size - _write_pos;
}

int AudioBuffer::readable_size()
{
	return _write_pos - _read_pos;
}

void AudioBuffer::has_read(int len)
{
	_read_pos += len;
	if (_read_pos >= _write_pos)
	{
		_read_pos = _write_pos;
	}
}

int AudioBuffer::read(uint8_t* dst, int len)
{
	int min = len > readable_size() ? readable_size() : len;
	memcpy(dst, read_ptr(), min);
	has_read(min);
	return min;
}

void AudioBuffer::has_written(int size)
{
	if (_write_pos + size <= _size)
	{
		_write_pos += size;
	}
}

void AudioBuffer::resize(size_t new_len)
{
	//重新调整内存大小
}

AudioBuffer::~AudioBuffer()
{
	if (_buf)
	{
		free(_buf);
		_buf = NULL;
	}
	if (_temp_buf)
	{
		free(_temp_buf);
		_temp_buf = NULL;
	}
	_size = 0;
	_read_pos = 0;
	_write_pos = 0;
}
