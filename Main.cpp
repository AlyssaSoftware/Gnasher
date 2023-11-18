#include "Gnasher.h"
uint8_t mode;
int main(int argc, char** argv) {
	WSADATA wd; int wver = MAKEWORD(2, 2);
	WSAStartup(wver, &wd);

	if (argc < 2) {
		printf("Usage: ...");
	}
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-p")) mode = 1;
		else if (!strcmp(argv[i], "-c")) mode = 2;
	}
	if (mode == 1) {
		ProxySession p(argv[2]); 
		if (!p.InitSession()) p.SessionLoop();
	}
}