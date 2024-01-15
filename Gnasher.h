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
#pragma warning(disable:4996)
#include <conio.h>

#define version 0.3

struct EndpointInfo {
	SOCKET s = INVALID_SOCKET; WOLFSSL* ssl = NULL;
	bool isServer;
	char* ipAddr = NULL;
	sockaddr_in sAddr;
};

struct ProxyEnds {// 0 is the client that connects to us, 1 is the server that we connect to.
	EndpointInfo e[2]; 
};

class ProxySession {
	public:
		ProxySession(char* ServerTarget, bool hasSSL);
		~ProxySession();

		int SessionLoop();
	private:
		std::deque<ProxyEnds> ends;
		EndpointInfo target, listening;
		bool hasSSL = 0;
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

void EchoServer(bool hasSSL);
void LoopServer(bool hasSSL);

extern WOLFSSL_CTX* ctx;
extern WOLFSSL_CTX* ctx_server;

#pragma once
