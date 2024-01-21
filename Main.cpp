#include "Gnasher.h"
uint8_t mode;
WOLFSSL_CTX* ctx=NULL;
WOLFSSL_CTX* ctx_server=NULL;
std::chrono::system_clock::time_point SessionBegin;// = std::chrono::system_clock::now();
FILE* out; char fbuf[20] = { 0 };

int main(int argc, char** argv) {
	WSADATA wd; int wver = MAKEWORD(2, 2);
	if (WSAStartup(wver, &wd)) {
		std::terminate();
	}
	//Set the locale and stdout to Unicode
	fwide(stdout, 0); setlocale(LC_ALL, "");

	if (argc < 2) {
		printf("Usage: ...");
	}
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-p")) mode = 1;
		else if (!strcmp(argv[i], "-c")) mode = 2;
		else if (!strcmp(argv[i], "-e")) mode = 3;
		else if (!strcmp(argv[i], "-r")) mode = 4;
		else if (!strcmp(argv[i], "-sp")) mode = 5;
		else if (!strcmp(argv[i], "-sc")) mode = 6;
		else if (!strcmp(argv[i], "-se")) mode = 7;
		else if (!strcmp(argv[i], "-sr")) mode = 8;
		else if (!strcmp(argv[i], "-rs")) mode = 9;
		else if (!strcmp(argv[i], "-srs")) mode = 10;
		else if (!strcmp(argv[i], "-f")) {
			if (i + 1 > argc) { printf("missing argument amk\n"); return -1; }
			out = fopen(argv[i + 1], "wb"); if (!out)std::terminate();
		}
		else if (!strcmp(argv[i], "-gn2txt")) return gn2txt(argv[i + 1], argv[i + 2]);
	}
	

	if (mode == 1) {
		ProxySession p(argv[2],0); 
		p.SessionLoop();
	}
	else if (mode == 2) {
		ClientSession c(argv[2],0,0);
		if (!c.InitSession()) c.SessionLoop();
	}
	else if (mode == 3) {
		EchoServer(0);
	}
	else if (mode == 4) {
		LoopServer(0);
	}
	else if (mode == 5) {
		wolfSSL_Init();
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
		ctx_server = wolfSSL_CTX_new(wolfSSLv23_server_method());
		wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL); wolfSSL_CTX_set_verify(ctx_server, SSL_VERIFY_NONE, NULL);
		if (wolfSSL_CTX_use_PrivateKey_file(ctx_server, "key.key", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		if (wolfSSL_CTX_use_certificate_file(ctx_server, "crt.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		ProxySession p(argv[2], 1);
		p.SessionLoop();
	}
	else if (mode == 6) {
		wolfSSL_Init();
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
		wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
		ClientSession c(argv[2], 1, 0);
		if (!c.InitSession()) c.SessionLoop();
	}
	else if (mode == 7) {
		wolfSSL_Init();
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
		ctx_server = wolfSSL_CTX_new(wolfSSLv23_server_method());
		wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL); wolfSSL_CTX_set_verify(ctx_server, SSL_VERIFY_NONE, NULL);
		if (wolfSSL_CTX_use_PrivateKey_file(ctx_server, "key.key", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		if (wolfSSL_CTX_use_certificate_file(ctx_server, "crt.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		EchoServer(1);
	}
	else if (mode == 8) {
		wolfSSL_Init();
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
		ctx_server = wolfSSL_CTX_new(wolfSSLv23_server_method());
		wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL); wolfSSL_CTX_set_verify(ctx_server, SSL_VERIFY_NONE, NULL);
		if (wolfSSL_CTX_use_PrivateKey_file(ctx_server, "key.key", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		if (wolfSSL_CTX_use_certificate_file(ctx_server, "crt.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		LoopServer(1);
	}
	else if (mode == 9) {
		ClientSession c(argv[2], 0, 1);
		if (!c.InitSession()) c.SessionLoop();
	}
	else if (mode == 10) {
		wolfSSL_Init();
		ctx_server = wolfSSL_CTX_new(wolfSSLv23_server_method());
		wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL); wolfSSL_CTX_set_verify(ctx_server, SSL_VERIFY_NONE, NULL);
		if (wolfSSL_CTX_use_PrivateKey_file(ctx_server, "key.key", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		if (wolfSSL_CTX_use_certificate_file(ctx_server, "crt.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS) {
			std::terminate();
		}
		ClientSession c(argv[2], 1, 1);
		if (!c.InitSession()) c.SessionLoop();
	}
}
