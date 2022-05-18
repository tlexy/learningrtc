#include "io_server.h"
#include <uvnet/core/tcp_connection.h>
#include <uvnet/core/tcp_server.h>
#include <uvnet/core/event_loop.h>
#include <uvnet/core/timer.h>
#include <iostream>
//#include <pActor/core/ptr_buffer.h>
#include <common/util/sutil.h>

#define PACKET_HEADER_LEN_NODE 16

IoServer::IoServer()
{}

void IoServer::on_newconnection(std::shared_ptr<uvcore::TcpConnection> ptr)
{
	_conn_map[ptr->id()] = ptr;
	std::cout << "on new connection...id: " << ptr->id() << std::endl;
}

void IoServer::timer_event(uvcore::Timer*)
{
	
}

void IoServer::on_message(std::shared_ptr<uvcore::TcpConnection> ptr)
{
	if (ptr->error() != 0)
	{
		ptr->close();
		return;
	}
	int payload_len = 0;
	while (true)
	{
		int payload_len = uvcore::PacketHelpers::unpacket_test(ptr->get_inner_buffer()->read_ptr(),
			ptr->get_inner_buffer()->readable_size());
		if (payload_len < 0 || payload_len > 16384*1024)
		{
			break;
		}
		memset(_buff, 0x0, sizeof(_buff));
		int ret = uvcore::PacketHelpers::unpack(_packet_header, (uint8_t*)_buff, sizeof(_buff),
				ptr->get_inner_buffer()->read_ptr(), ptr->get_inner_buffer()->readable_size());
		if (ret >= 0)
		{
			ptr->get_inner_buffer()->has_read(ret + PACKET_HEADER_LEN_NODE);
			
		}
		else
		{
			//loge("unpack error, payload_len: %d, error: %d", payload_len, ret);
		}
	}
}

void IoServer::on_connection_close(std::shared_ptr<uvcore::TcpConnection> ptr)
{
	_conn_map.erase(ptr->id());
}
