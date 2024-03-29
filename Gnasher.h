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
#include <chrono>

#define version "0.6.1"
#define time std::chrono::duration<float>(std::chrono::system_clock::now() - SessionBegin).count()

struct EndpointInfo {
	SOCKET s = INVALID_SOCKET; WOLFSSL* ssl = NULL;
	bool isServer = 0;
	char* ipAddr = NULL;
	sockaddr_in sAddr = { 0 };
};

struct ProxyEnds {// 0 is the client that connects to us, 1 is the server that we connect to.
	EndpointInfo e[2]; 
};

struct Params {
	int argc = 0; char** argv = NULL;
	bool hasSSL = 0; unsigned short port = 22222;
};

class ProxySession {
	public:
		ProxySession(Params* p);
		~ProxySession();

		int SessionLoop();
	private:
		std::deque<ProxyEnds> ends;
		EndpointInfo target, listening;
		bool hasSSL = 0;
};

class ClientSession {
	public:
		ClientSession(Params* p, bool amIServer);
		~ClientSession();

		int InitSession();
		int SessionLoop();
	
	private:
		EndpointInfo serv; wchar_t* InputBuffer; sockaddr_in me;
		char* RecvBuffer; char* SendBufConv;
};

void EchoServer(Params* p);
void LoopServer(Params* p);
int gn2txt(char* in, char* out);

extern WOLFSSL_CTX* ctx;
extern WOLFSSL_CTX* ctx_server;

extern std::chrono::system_clock::time_point SessionBegin;
extern FILE* out;
extern char fbuf[20];
// SSL key and certificate paths.
extern char* sslcert;
extern char* sslkey;
extern char GPLLicense[];
extern char UsageString[];

#pragma once
