#ifndef IPORT_CALLBACK_H
#define IPORT_CALLBACK_H

class IPortCallBack
{
public:
	IPortCallBack() {}
	~IPortCallBack() {}

	virtual void stream_cb(const void* input, unsigned long frameCount, int sampleSize) = 0;

};

#endif