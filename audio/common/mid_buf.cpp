#include "mid_buf.h"
#include <stdlib.h>
#include <string.h>

mid_buf::mid_buf(size_t size)
	:_size(size),
	_read_pos(0),
	_write_pos(0)
{
	_buf = (uint8_t*)malloc(_size);
	_temp_buf = (uint8_t*)malloc(_size);
}

void mid_buf::reset()
{
	_read_pos = 0;
	_write_pos = 0;
}

size_t mid_buf::push(const uint8_t* data, int len)
{
	if (empty_count() < len)
	{
		//重新整理内存
		memcpy(_temp_buf, _buf + _read_pos, usable_count());
		memcpy(_buf, _temp_buf, usable_count());
		_write_pos = usable_count();
		_read_pos = 0;
	}
	//
	if (empty_count() >= len)
	{
		memcpy(_buf + _write_pos, data, len);
		_write_pos += len;
		return len;
	}
	return 0;
}

uint8_t* mid_buf::data()
{
	return _buf + _read_pos;
}

int mid_buf::empty_count()
{
	return _size - _write_pos;
}

int mid_buf::usable_count()
{
	return _write_pos - _read_pos;
}

void mid_buf::set_read(int len)
{
	_read_pos += len;
	if (_read_pos >= _write_pos)
	{
		_read_pos = _write_pos;
	}
}

void mid_buf::resize(size_t new_len)
{
	//重新调整内存大小
}

mid_buf::~mid_buf()
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
