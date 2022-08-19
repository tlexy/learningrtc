#include "net_random_emulator.h"

NetRandomEmulator::NetRandomEmulator()
	:_rnd(1, 100)
{}

void NetRandomEmulator::set_loss_rate(int rate)
{
	if (rate < 0 || rate > 100)
	{
		return;
	}
	_loss_rate = rate;
}

void NetRandomEmulator::send_data(const char* data, int len)
{
	if (get_rate() == 0)
	{
		on_send_data(data, len);
	}
}

int NetRandomEmulator::get_rate()
{
	int rnd = _rnd(_engine);
	return rnd <= _loss_rate ? 1 : 0;
}