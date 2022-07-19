#ifndef DC_LOOP_PCM_BUFFER_H
#define DC_LOOP_PCM_BUFFER_H

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <mutex>
#include "mid_buf.h"


class DcPcmBuffer
{
public:
	DcPcmBuffer(int buff_size);

public:
	uint8_t* buffer[1];
	int buff_size;
};

class DcLoopPcmBuffer
{
public:
	DcLoopPcmBuffer(int queue_size);

	DcPcmBuffer* get_empty_buffer2();
	void push_empty_buffer2(DcPcmBuffer*);

	DcPcmBuffer* get_full_buffer();
	bool push_full_buffer(DcPcmBuffer*);

	void push_unspecified2(const char* data, int len);

private:
	void write_to_buffer2();

private:
	std::vector<DcPcmBuffer*> _empty_buff;
	std::vector<DcPcmBuffer*> _full_buff;
	int _full_write_pos{0};
	int _full_read_pos{0};
	std::mutex _full_mutex;
	std::mutex _empty_mutex;
	int _empty_top{-1};
	int _frame_size{4096};

	mid_buf* _inner_mid;
};

#endif