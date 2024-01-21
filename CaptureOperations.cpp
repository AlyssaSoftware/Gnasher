#include "Gnasher.h"
#include <fstream>
int gn2txt(char* in, char* out) {
	char* rbuf = new char[32768]; memset(rbuf, 0, 32768);
	int sz = 0, pos = 0;
	float timestamp = 0; in_addr src_ip = { 0 }, dest_ip = { 0 }; unsigned short src_port = 0, dest_port = 0;
	// BUGBUG: standard file APIs (both stdio and fstreams, which used to be certainly working) 
	// cannot properly read portions of file on Windows (but is able to read the whole file at once) for some fucking reason
	// (and yes i indeed enabled binary flag on both)
	// 
	// As a workaround Windows uses Win32 API instead. I have no idea what to do on *nix part, I just hope it is working
#ifdef _WIN32
	HANDLE r, w; DWORD rbytes=0;
	r = CreateFileA(in, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	w = CreateFileA(out, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (r == INVALID_HANDLE_VALUE || w == INVALID_HANDLE_VALUE) std::terminate();
	while (true) {
		ReadFile(r, rbuf, 20, &rbytes, NULL);
		if (rbytes != 20) if (!rbytes) break; else std::terminate();
		memcpy(&timestamp, rbuf, 4); memcpy(&src_ip, &rbuf[4], 4); memcpy(&src_port, &rbuf[8], 2);
		memcpy(&dest_ip, &rbuf[10], 4); memcpy(&dest_port, &rbuf[14], 2); memcpy(&sz, &rbuf[16], 4);
		rbytes=sprintf(rbuf, "[%f] %s:%d -> %s:%d (%d bytes): ", timestamp, inet_ntoa(src_ip), src_port, inet_ntoa(dest_ip), dest_port, sz);
		WriteFile(w, rbuf, rbytes, &rbytes, NULL);
		ReadFile(r, rbuf, sz, &rbytes, NULL);
		if (rbytes != sz) if (rbytes) std::terminate();
		sz += sprintf(&rbuf[sz], "(end of %d bytes)\r\n", sz);
		WriteFile(w, rbuf, sz, &rbytes, NULL);
	}
	CloseHandle(r); CloseHandle(w);
#else
	std::ifstream r(in, std::ios::binary); std::ofstream w(out, std::ios::binary);

	while (true) {
		if (sz=r.read(rbuf, 44).gcount() < 20) std::terminate();
		memcpy(&timestamp, rbuf, 4); memcpy(&src_ip, &rbuf[4], 4); memcpy(&src_port, &rbuf[8], 2);
		memcpy(&dest_ip, &rbuf[10], 4); memcpy(&dest_port, &rbuf[14], 2); memcpy(&sz, &rbuf[16], 4);
		if (sz < 32728) {
			r.read(rbuf, sz);
			w << "[" << timestamp << "] " << inet_ntoa(src_ip) << ":" << src_port << " -> " << inet_ntoa(dest_ip) << ":" << dest_port << " (" << sz << " bytes):";
			sz += sprintf(&rbuf[20 + sz], "(end of %d bytes)\n", sz); w.write(rbuf, sz);
		}
		else std::terminate();
		if (r.eof()) break;
	}
#endif
	return 0;
}
