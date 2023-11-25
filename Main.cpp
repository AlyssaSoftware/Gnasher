#include "Gnasher.h"
uint8_t mode;
int main(int argc, char** argv) {
	WSADATA wd; int wver = MAKEWORD(2, 2);
	WSAStartup(wver, &wd);
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
	}

	if (mode == 1) {
		ProxySession p(argv[2]); 
		if (!p.InitSession()) p.SessionLoop();
	}
	else if (mode == 2) {
		ClientSession c(argv[2]);
		if (!c.InitSession()) c.SessionLoop();
	}
	else if (mode == 3) {
		EchoServer();
	}
	else if (mode == 4) {
		LoopServer();
	}
}