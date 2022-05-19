#include <iostream>
#include <memory>
#include "core/io_server.h"

#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Userenv.lib")

int main(int argc, char* argv[])
{
	auto server = std::make_shared<IoServer>();
	server->async_io_start("0.0.0.0", 5678);

	std::cin.get();

	return 0;
}