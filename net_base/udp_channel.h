#ifndef UDP_CHANNEL_H
#define UDP_CHANNEL_H

#include <memory>
#include <functional>
#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <stdint.h>

/// <summary>
/// 发送与接收udp的类。
/// 1. 特别注意在非io线程中进行bind，当bind失败的时候，bind也会返回一个有效的指针，
/// 同时在io线程会调用on_data_receive
/// 2. 在io线程中调用bind的时候，当bind失败的时候，bind也会返回一个空指针，
/// 同时也会在io线程会调用on_data_receive
/// 3.当new一个Udp对象失败时，首先会在调用线程中调用on_data_receive，然后返回nullptr
/// </summary>
class UdpChannel
{
public:
	//using UdpDataCb = std::function<void(uvcore::Udp*, const struct sockaddr*)>;
	enum class ChannelType
	{
		Client,
		Server
	};
	/// <summary>
	/// 目前来看，这个参数没什么用？？？
	/// </summary>
	/// <param name=""></param>
	UdpChannel(ChannelType = ChannelType::Client);

	/// <summary>
	/// 在io线程中调用bind，如果返回0，则说明成功了，
	/// 如果在非io线程中调用bind，即使返回0，可有可能是失败的
	/// </summary>
	/// <param name="server"></param>
	/// <param name=""></param>
	/// <param name="ip"></param>
	/// <returns></returns>
	int bind(std::shared_ptr<uvcore::UdpServer> server, 
		uvcore::Udp::UdpReceiveCallback2, const uvcore::IpAddress& ip = uvcore::IpAddress());

	/// <summary>
	/// 作为一个客户端，发送前应该设置远端地址
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	void set_remote_addr(const uvcore::IpAddress&);
	
	int send(const char* data, int len);

private:
	void on_data_receive(uvcore::Udp*, const struct sockaddr*);

private:
	ChannelType _type;
	uvcore::Udp* _udp{nullptr};
	uvcore::IpAddress _addr;
	uvcore::Udp::UdpReceiveCallback2 _cb;
	uvcore::IpAddress _remote_addr;
};

#endif