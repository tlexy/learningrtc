#include <iostream>
#include <memory>
#include "learnrtc_server.h"

#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Userenv.lib")

int main(int argc, char* argv[])
{
	auto server = std::make_shared<LearnRtcServer>();
	server->start(5678);

	std::cin.get();

	return 0;
}