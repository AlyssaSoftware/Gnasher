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
#include "user_settings.h"
#include <wolfssl/ssl.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wolfssl.lib")
#include <conio.h>

#define version 0.3

struct EndpointInfo {
	SOCKET s = INVALID_SOCKET; WOLFSSL* ssl = NULL;
	bool isServer;
	char* ipAddr = NULL;
	sockaddr_in sAddr;
};

class ProxySession {
	public:
		ProxySession(char* ServerTarget, bool hasSSL);
		~ProxySession();

		int InitSession();
		int SessionLoop();

	private:
		EndpointInfo ends[2]; // 0 is the client that connects to us, 1 is the server that we connect to.
};

class ClientSession {
	public:
		ClientSession(char* ServerTarget, bool hasSSL);
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

extern WOLFSSL_CTX* ctx;
extern WOLFSSL_CTX* ctx_server;

#pragma once
