#ifndef NET_RANDOM_EMULATOR_H
#define NET_RANDOM_EMULATOR_H

#include <stdint.h>
#include <random>

//网络随机丢包发生器
class NetRandomEmulator
{
public:
	NetRandomEmulator();
	void set_loss_rate(int rate);

	void send_data(const char* data, int len);

	//没有发生丢包时，会调用这个函数进行真实的发送
	virtual void on_send_data(const char* data, int len) = 0;

private:
	/// <summary>
	/// 需要丢弃时，返回1
	/// </summary>
	/// <returns></returns>
	int get_rate();

private:
	std::default_random_engine _engine;
	std::uniform_int_distribution<int> _rnd;
	int _loss_rate{0};
};

#endif