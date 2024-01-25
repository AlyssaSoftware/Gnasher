#include "Gnasher.h"
uint8_t mode;
WOLFSSL_CTX* ctx=NULL;
WOLFSSL_CTX* ctx_server=NULL;
std::chrono::system_clock::time_point SessionBegin;// = std::chrono::system_clock::now();
FILE* out; char fbuf[20] = { 0 };
extern char* sslcert = NULL;
extern char* sslkey  = NULL;

int main(int argc, char** argv) {
	WSADATA wd; int wver = MAKEWORD(2, 2); Params p; p.argv = argv; p.argc = argc;
	if (WSAStartup(wver, &wd)) {
		std::terminate();
	}
	//Set the locale and stdout to Unicode
	fwide(stdout, 0); setlocale(LC_ALL, "");

	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " [mode] [parameters] (-flags). Try \"" << argv[0] << " help\" for basic usage.\n";
	}
	int _sz = strlen(argv[1]);
	for (uint8_t i = 0; i < _sz; i++) {// Make mode case insensitive
		argv[1][i] = tolower(argv[1][i]);
	}
	if (argv[1][0] == 's') { p.hasSSL = 1; memmove(&argv[1][0], &argv[1][1], _sz); }//Modes starting with s is ssl ones.
	uint8_t pc = 0; //Parameters count
	// Parse the mode
	if		(argv[1][0] == 'p') mode = 1;// Proxy
	else if (argv[1][0] == 'c') mode = 2;// Client
	else if (argv[1][0] == 'e') mode = 3;// EchoServer
	else if (argv[1][0] == 'l') mode = 4;// LoopServer
	else if (argv[1][0] == 'r') mode = 5;// ReverseClient
	else if (argv[1][0] == 'g') mode = 6;// gn2txt
	else if (argv[1][0] == 'v') {
		printf("Gnasher Network Development Utility version %s\n"
			"WolfSSL version: %s\n%s\n", version, WOLFSSL_VERSION, GPLLicense); return 0;
	}// Version
	else if (argv[1][0] == 'h') { std::cout << UsageString; return 0; }// Help
	else {
		std::cout << "E: Mode is not valid. See \"" << argv[0] << " help\" for valid modes.\n"; return -1;
	}
	// Check if parameter is not missing
	//if (argc < 3) {
	//	std::cout << "E: Parameter is missing. See \"" << argv[0] << " help\" for usage.\n"; return -1;
	//}
	// Parse the additional flags
	if (mode != 6) {
		for (size_t i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "-f")) {
				if (i + 1 > argc) { std::cout << "E: Usage: \"-f [file]\"\n"; return -1; }
				out = fopen(argv[i + 1], "rb");
				if (!out) { std::cout << "E: Opening capture file \"" << argv[i + 1] << "\" failed.\n"; return -1; } i++;
			}
			else if (!strcmp(argv[i], "-sslcert")) {
				if (i + 1 > argc) { std::cout << "E: Usage: \"-sslcert [certificate path]\"\n"; return -1; }
				sslcert = argv[i + 1]; i++;
			}
			else if (!strcmp(argv[i], "-sslkey")) {
				if (i + 1 > argc) { std::cout << "E: Usage: \"-sslkey [private key path]\"\n"; return -1; }
				sslkey = argv[i + 1]; i++;
			}
			else if (!strcmp(argv[i], "-p")) {
				if (i + 1 > argc) { std::cout << "E: Usage: \"-p [port]\"\n"; return -1; }
				p.port = std::atoi(argv[i + 1]); i++;
			}
		}
	}

	if (p.hasSSL) {
		wolfSSL_Init();
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method()); wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
		if (mode != 2) {// Set up server as well if mode is not client.
			ctx_server = wolfSSL_CTX_new(wolfSSLv23_server_method());
			wolfSSL_CTX_set_verify(ctx_server, SSL_VERIFY_NONE, NULL);
			if (wolfSSL_CTX_use_PrivateKey_file(ctx_server, sslkey, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
				std::terminate();
			}
			if (wolfSSL_CTX_use_certificate_file(ctx_server, sslcert, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
				std::terminate();
			}
		}
	}

	if (mode == 1) {
		ProxySession ps(&p); 
		ps.SessionLoop();
	}
	else if (mode == 2) {
		ClientSession c(&p,0);
		if (!c.InitSession()) c.SessionLoop();
	}
	else if (mode == 3) {
		EchoServer(&p);
	}
	else if (mode == 4) {
		LoopServer(&p);
	}
	else if (mode == 5) {
		ClientSession c(&p, 1);
		if (!c.InitSession()) c.SessionLoop();
	}
	else if(mode==6) return gn2txt(argv[2], argv[3]);
}
