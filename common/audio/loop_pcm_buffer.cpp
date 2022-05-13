#include "loop_pcm_buffer.h"

DcPcmBuffer::DcPcmBuffer(int size)
	:buff_size(size)
{
	buffer[0] = (uint8_t*)malloc(size);
	memset(buffer[0], 0x0, size);
}

DcLoopPcmBuffer::DcLoopPcmBuffer(int queue_size)
	:_frame_size(4096)
{
	for (int i = 0; i < queue_size; ++i)
	{
		_empty_buff.push_back(new DcPcmBuffer(_frame_size));
		//_full_buff.push_back(new DcPcmBuffer(_frame_size));
	}
	_empty_top = queue_size - 1;
	_inner_mid = new mid_buf(1024 * 1024);
}

void DcLoopPcmBuffer::push_unspecified2(const char* data, int len)
{
	if (len < 1)
	{
		return;
	}
	_inner_mid->push((const uint8_t*)data, len);
	if (_inner_mid->usable_count() >= _frame_size)
	{
		write_to_buffer2();
	}
}

void DcLoopPcmBuffer::write_to_buffer2()
{
	while (_inner_mid->usable_count() >= _frame_size)
	{
		DcPcmBuffer* ptr = get_empty_buffer2();
		if (ptr != NULL)
		{
			memcpy(ptr->buffer[0], _inner_mid->data(), _frame_size);
			_inner_mid->set_read(4096);
			bool flag = push_full_buffer(ptr);
			if (!flag)
			{
				//写入失败，归还buffer
				push_empty_buffer2(ptr);
			}
		}
		else
		{
			break;
		}
	}
}

bool DcLoopPcmBuffer::push_full_buffer(DcPcmBuffer* ptr)
{
	if (!ptr || _full_write_pos < 0)
	{
		return true;
	}
	std::lock_guard<std::mutex> lock(_full_mutex);
	if (_full_write_pos >= _full_buff.size())
	{
		//有空间可用
		if (_full_read_pos > 0)
		{
			for (int i = 0; i < _full_write_pos - _full_read_pos; ++i)
			{
				_full_buff[i] = _full_buff[i + _full_read_pos];
			}
			_full_write_pos = _full_write_pos - _full_read_pos;
			_full_read_pos = 0;
		}
		else
		{
			return false;
		}
	}
	_full_buff[_full_write_pos++] = ptr;
	return true;
}

DcPcmBuffer* DcLoopPcmBuffer::get_full_buffer()
{
	std::lock_guard<std::mutex> lock(_full_mutex);
	DcPcmBuffer* ptr = NULL;
	if (_full_read_pos < 0 || _full_read_pos >= _full_write_pos)
	{
		return ptr;
	}
	if (_full_read_pos < _full_write_pos && _full_read_pos < _full_buff.size())
	{
		ptr = _full_buff[_full_read_pos++];
	}
	return ptr;
}

DcPcmBuffer* DcLoopPcmBuffer::get_empty_buffer2()
{
	std::lock_guard<std::mutex> lock(_empty_mutex);
	if (_empty_top > -1 && _empty_top < _empty_buff.size())
	{
		return _empty_buff[_empty_top--];
	}
	return NULL;
}

void DcLoopPcmBuffer::push_empty_buffer2(DcPcmBuffer* ptr)
{
	std::lock_guard<std::mutex> lock(_empty_mutex);
	if (_empty_top >= -1 && _empty_top < _empty_buff.size() - 1)
	{
		_empty_buff[++_empty_top] = ptr;
	}
	else
	{
		_empty_top = 0;
		_empty_buff[_empty_top] = ptr;
	}
}