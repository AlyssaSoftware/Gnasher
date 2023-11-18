// Includes
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <WS2tcpip.h>
#include <wolfssl/ssl.h>
#pragma comment(lib,"ws2_32.lib")

struct EndpointInfo {
	SOCKET s; WOLFSSL* ssl;
	bool isServer;
	char* ipAddr = NULL;
	sockaddr_in sAddr;
};

class ProxySession
{
public:
	ProxySession(char* ServerTarget);
	~ProxySession();

	int InitSession();
	int SessionLoop();

private:
	EndpointInfo ends[2];
};

#pragma once
