#include "udp_channel.h"
#include <iostream>

UdpChannel::UdpChannel(ChannelType ct)
	:_type(ct)
{}

int UdpChannel::bind(std::shared_ptr<uvcore::UdpServer> server,
	uvcore::Udp::UdpReceiveCallback2 cb, const uvcore::IpAddress& ip)
{
	using namespace std::placeholders;
	if (!cb)
	{
		return 2;
	}
	_addr = ip;
	_addr.set_valid(true);
	_cb = cb;
	if (_addr.getPort() == 0 && _type == ChannelType::Server)
	{
		std::cerr << "bind port is 0, but channel type is server." << std::endl;
	}
	_udp = server->addBind(_addr, std::bind(&UdpChannel::on_data_receive, this, _1, _2));
	if (_udp)
	{
		return 0;
	}
	return 1;
}

void UdpChannel::on_data_receive(uvcore::Udp* udp, const struct sockaddr* addr)
{
	if (addr)
	{
		if (!_remote_addr())
		{
			uvcore::IpAddress raddr = uvcore::IpAddress::fromRawSocketAddress((sockaddr*)addr, sizeof(sockaddr_in));
			_remote_addr = raddr;
		}
	}
	if (!addr)
	{
		if (udp)
		{
			//udp总是有效的
			delete udp;
		}
		_udp = nullptr;
	}
	//最终会回调用户传回来的参数
	if (_cb)
	{
		//_udp与addr都有可能为nullptr，只要有一个为nullptr，就说明出错了
		_cb(_udp, addr);
	}
}

void UdpChannel::set_remote_addr(const uvcore::IpAddress& raddr)
{
	_remote_addr = raddr;
}

int UdpChannel::send(const char* data, int len)
{
	if (!_remote_addr())
	{
		//客户端发送必须有一个有效的远端地址
		return -1;
	}
	if (!_udp)
	{
		return -2;
	}
	_udp->send2(data, len, _remote_addr);
	return 0;
}