// Includes
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <stdio.h>
#include <string.h>
#include <WS2tcpip.h>
#include <wolfssl/ssl.h>
#pragma comment(lib,"ws2_32.lib")
#include <conio.h>

#define version 0.0.2

struct EndpointInfo {
	SOCKET s; WOLFSSL* ssl;
	bool isServer;
	char* ipAddr = NULL;
	sockaddr_in sAddr;
};

class ProxySession {
	public:
		ProxySession(char* ServerTarget);
		~ProxySession();

		int InitSession();
		int SessionLoop();

	private:
		EndpointInfo ends[2];
};

class ClientSession {
	public:
		ClientSession(char* ServerTarget);
		~ClientSession();

		int InitSession();
		int SessionLoop();
	
	private:
		EndpointInfo serv; wchar_t* InputBuffer;
		char* RecvBuffer; char* SendBufConv;
		std::mutex ConMtx; unsigned short CurPos = 0, InputSz = 0, BufSz = 0;
};

void EchoServer();
void LoopServer();


#pragma once
