#include "Gnasher.h"

ProxySession::ProxySession(char* ServerTarget, bool hasSSL) {
	int _ret;
	// Init listening socket
	listening.s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	socklen_t sz = sizeof(listening.sAddr); listening.sAddr.sin_family = AF_INET;
	listening.sAddr.sin_port = htons(666);
	_ret = inet_pton(AF_INET, "0.0.0.0", &listening.sAddr.sin_addr);
	sz = sizeof(listening.sAddr);
	_ret = bind(listening.s, (sockaddr*)&listening.sAddr, sz);
	_ret = listen(listening.s, SOMAXCONN);
	// Set target.
	char* pos; 
	if (pos = std::find(ServerTarget, ServerTarget + strlen(ServerTarget), ':')) {
		target.sAddr.sin_port = htons(std::atoi(pos+1)); pos[0] = '\0';
	}
	_ret=inet_pton(AF_INET, ServerTarget, &target.sAddr.sin_addr); target.sAddr.sin_family = AF_INET;
	// Set SSL structures for listening socket.
	if (hasSSL) {
		listening.ssl = wolfSSL_new(ctx_server);
		wolfSSL_set_fd(listening.ssl, listening.s);
	}
	this->hasSSL = hasSSL; SessionBegin = std::chrono::system_clock::now();
	return;
}

ProxySession::~ProxySession() {
	/*shutdown(this->ends[0].s, 2); closesocket(this->ends[0].s);
	shutdown(this->ends[1].s, 2); closesocket(this->ends[1].s);
	if (this->ends[0].ssl) wolfSSL_free(this->ends[0].ssl);
	if (this->ends[1].ssl) wolfSSL_free(this->ends[1].ssl);*/
}

int ProxySession::SessionLoop() {
	std::vector<pollfd> pf = { pollfd{listening.s,POLLIN,0} };
	char* buf = new char[32768]; memset(buf, 0, 32768);
	int pollcnt = 0, received = 0;
	unsigned short nfds = 1;
	while ((pollcnt = WSAPoll(&pf[0], nfds, -1)) > 0) {
		// Check for listening socket, accept the connection if there is a event.
		if (pf[0].revents & POLLIN) {
			pollcnt--; ProxyEnds client;
#define cli client.e[0]
#define srv client.e[1]
#ifndef _WIN32
			unsigned int clientSize = sizeof(cli.sAddr);
#else
			int clientSize = sizeof(cli.sAddr);
#endif
			cli.s = accept(listening.s, (sockaddr*)&cli.sAddr, &clientSize);
#pragma warning(disable:4996)
			cli.ipAddr = inet_ntoa(cli.sAddr.sin_addr);
			printf("[%f] I: incoming connection from %s:%d\n", time, cli.ipAddr, ntohs(cli.sAddr.sin_port));
			if (listening.ssl) {// Do SSL handshake with client.
				cli.ssl = wolfSSL_new(ctx_server); wolfSSL_set_fd(cli.ssl, cli.s);
				if (wolfSSL_accept(cli.ssl) != 1) {
					char error[80] = { 0 };
					printf("E: SSL handshake with %s:%d failed: %s.\n", cli.ipAddr, ntohs(cli.sAddr.sin_port),
						wolfSSL_ERR_error_string(wolfSSL_get_error(cli.ssl, -1), error));
					wolfSSL_free(cli.ssl); shutdown(cli.s, 2); closesocket(cli.s); goto acceptOut;
				}
			}
			// Connect to server.
			srv.sAddr = target.sAddr; srv.s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (connect(srv.s, (sockaddr*)&srv.sAddr, sizeof sockaddr_in)) {
				printf("[%f] E: connection from %s:%d to server failed: %d.\n", time, cli.ipAddr, ntohs(cli.sAddr.sin_port), WSAGetLastError());
acceptFail:
				if(cli.ssl) { wolfSSL_free(cli.ssl); } shutdown(cli.s, 2); closesocket(cli.s); goto acceptOut;
			}
			if (this->hasSSL) {// Do SSL handshake with server.
				srv.ssl = wolfSSL_new(ctx); wolfSSL_set_fd(srv.ssl, srv.s);
				if (wolfSSL_connect(srv.ssl) != 1) {
					char error[80] = { 0 };
					printf("[%f] E: SSL handshake between %s:%d and server failed: %s\n", time, cli.ipAddr, ntohs(cli.sAddr.sin_port),
						wolfSSL_ERR_error_string(wolfSSL_get_error(srv.ssl, -1), error)); wolfSSL_free(srv.ssl); closesocket(srv.s); goto acceptFail;
				}
			}
			ends.emplace_back(client); pf.emplace_back(pollfd{ cli.s,POLLIN,0 }); pf.emplace_back(pollfd{ srv.s,POLLIN,0 });
			printf("[%f] I: connection between %s:%d and server etablished successfully.\n", time, cli.ipAddr, ntohs(cli.sAddr.sin_port)); nfds += 2;
		}
#undef cli
#undef srv
		acceptOut:
		for (uint16_t i = 1; i < nfds; i++) {
			if (pf[i].revents) {// There's an event in such socket.
				pollcnt--;
				// if i is odd, it's a client. and i+1 is it's server endpoint.
				// same way, if i is even, it's a server endpoint and i-1 is it's client.
				if (pf[i].revents & POLLIN) {// Socket sent data.
					if (this->hasSSL) received = wolfSSL_recv(ends[(i - 1) / 2].e[(i % 2) ? 0 : 1].ssl, buf, 32767, 0);
					else received = recv(pf[i].fd, buf, 32767, 0); buf[received] = '\0';
					if (received < 0) std::terminate();
					// Send that data to other socket
					if (i % 2) printf("[%f] I: %s:%d sent data to server (%d bytes): %s\n==end of data (%d bytes)==\n", time, ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port), received, buf, received);
					else	   printf("[%f] I: server sent data to %s:%d (%d bytes): %s\n==end of data (%d bytes)==\n", time, ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port), received, buf, received);
					if (this->hasSSL) wolfSSL_send(ends[(i - 1)/2].e[(i % 2) ? 1 : 0].ssl, buf, received, 0);
					else send(ends[(i - 1) / 2].e[(i % 2) ? 1 : 0].s, buf, received, 0);
					if (out) {
						float amk = time;
						memcpy(fbuf, &amk, 4); memcpy(&fbuf[4], &ends[(i - 1) / 2].e[(i % 2) ? 0 : 1].sAddr.sin_addr, 4); memcpy(&fbuf[8], &ends[(i - 1) / 2].e[(i % 2)?0:1].sAddr.sin_port, 2);
						memcpy(&fbuf[10], &ends[(i - 1) / 2].e[(i % 2) ? 1 : 0].sAddr.sin_addr, 4); memcpy(&fbuf[14], &ends[(i - 1) / 2].e[(i % 2) ? 1 : 0].sAddr.sin_port, 2); memcpy(&fbuf[16], &received, 4);
						swab(&fbuf[8], &fbuf[8], 2); swab(&fbuf[14], &fbuf[14], 2); fwrite(fbuf, 20, 1, out); fwrite(buf, received, 1, out); fflush(out);
					}
				}
				else if (pf[i].revents & (POLLERR | POLLHUP)) {
					//printf("%s disconnected. Ending session...\n", (i) ? "[Server]" : "[Client]");
					if (i % 2) printf("[%f] I: %s:%d closed the connection with server\n", time,  ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port));
					else	   printf("[%f] I: server closed the connection with %s:%d\n", time,  ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port));
					if (this->hasSSL) { wolfSSL_shutdown(ends[(i - 1) / 2].e[0].ssl); wolfSSL_free(ends[(i - 1) / 2].e[0].ssl);
										wolfSSL_shutdown(ends[(i - 1) / 2].e[0].ssl); wolfSSL_free(ends[(i - 1) / 2].e[0].ssl); }
					shutdown(ends[(i - 1) / 2].e[0].s, 2); closesocket(ends[(i - 1) / 2].e[0].s);
					shutdown(ends[(i - 1) / 2].e[1].s, 2); closesocket(ends[(i - 1) / 2].e[1].s);
					std::vector<pollfd>::iterator pfb = pf.begin();
					ends.erase(ends.begin() + (i - 1) / 2); 
					//pf.erase((i % 2) ? pfb + i, pfb + i + 2 : pfb + i - 1, pfb + i + 1);
					//pf.erase(pfb + i, pfb + i + 2);
					if (i % 2) pf.erase(pfb + i, pfb + i + 2);
					else	   pf.erase(pfb + i - 1, pfb + i + 1);
					nfds -= 2; i--;
				}
				if (!pollcnt) break;
			}
		}
	}
	// Jumped out of poll, not supposed to happen.
	std::terminate();
	return 0;
}
