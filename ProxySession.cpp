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
	this->hasSSL = hasSSL; return;
}

ProxySession::~ProxySession() {
	/*shutdown(this->ends[0].s, 2); closesocket(this->ends[0].s);
	shutdown(this->ends[1].s, 2); closesocket(this->ends[1].s);
	if (this->ends[0].ssl) wolfSSL_free(this->ends[0].ssl);
	if (this->ends[1].ssl) wolfSSL_free(this->ends[1].ssl);*/
}

//int ProxySession::InitSession() {
//	std::cout << "I: Waiting for a connection... " << std::endl;
//	pollfd pf[1]; pf[0].fd = this->ends[0].s; pf->events = POLLIN;
//	/*
//		You point to the trail where the blossoms have fallen
//		But all I can see is the poll()en, fucking me up
//		Everything moves too fast but I've
//		Been doing the same thing a thousand times over
//		But I'm brought to my knees by the clover
//		And it feels like, it's just the poll()en
//	*/
//	if (!WSAPoll(&pf[0], 1, 10000)) { // Wait for 10 secs for client to connect.
//		std::cout << "E: Client connection timed out." << std::endl; return -2;
//	}
//	else if (pf[0].revents & POLLIN) { // Got a connection
//#ifndef _WIN32
//		unsigned int clientSize = sizeof(this->ends[0].sAddr);
//#else
//		int clientSize = sizeof(this->ends[0].sAddr);
//#endif
//		this->ends[0].s = accept(pf[0].fd, (sockaddr*)&this->ends[0].sAddr, &clientSize); wolfSSL_set_fd(this->ends[0].ssl, this->ends[0].s);
//		if (ends[0].ssl) {// Do SSL handshake with client.
//			if (wolfSSL_accept(ends[0].ssl) != 1) {// If fails, try again.
//				char error[80] = { 0 };
//				std::cout << "E: SSL handshake with client failed: " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->ends[0].ssl, -1), error) << ". Trying again..." <<std::endl;
//				if (!WSAPoll(&pf[0], 1, 1000)) goto SSLFail; // Client didn't connect again, give up.
//
//				this->ends[0].s = accept(pf[0].fd, (sockaddr*)&this->ends[0].sAddr, &clientSize); wolfSSL_free(this->ends[0].ssl);
//				this->ends[0].ssl = wolfSSL_new(ctx_server); wolfSSL_set_fd(this->ends[0].ssl, this->ends[0].s);
//
//				if (ends[0].ssl) {// Do SSL handshake with client again.
//					if (wolfSSL_accept(ends[0].ssl) != 1) {// Failed again, give up.
//SSLFail:
//						std::cout << "E: SSL handshake with client failed (attempt 2, giving up.): " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->ends[0].ssl, -1), error) << std::endl; return -1;
//					}
//				}
//			}
//		}
//		std::cout << "I: Client connected, connecting to server..." << std::endl;
//		// Connect to server after client connects.
//		if (connect(this->ends[1].s, (sockaddr*)&this->ends[1].sAddr, sizeof sockaddr_in)) {
//			std::cout << "E: Connection to server failed: " << WSAGetLastError() << std::endl; return -1;
//		}
//		if (ends[1].ssl) {// // Do SSL handshake with server.
//			if (wolfSSL_connect(ends[1].ssl) != 1) {
//				char error[80] = { 0 };
//				std::cout << "E: SSL handshake with server failed: " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->ends[1].ssl, -1), error) << std::endl; return -1;
//			}
//		}
//		std::cout << "I: Proxy session initiated successfully." << std::endl;
//		return 0;
//	}
//	else { // Error.
//		std::cout << "E: Client connection polling failed." << std::endl; return -2;
//	}
//}

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
			printf("I: incoming connection from %s:%d\n", cli.ipAddr, ntohs(cli.sAddr.sin_port));
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
				printf("E: connection from %s:%d to server failed: %d.\n", cli.ipAddr, ntohs(cli.sAddr.sin_port), WSAGetLastError());
acceptFail:
				if(cli.ssl) { wolfSSL_free(cli.ssl); } shutdown(cli.s, 2); closesocket(cli.s); goto acceptOut;
			}
			if (this->hasSSL) {// Do SSL handshake with server.
				srv.ssl = wolfSSL_new(ctx); wolfSSL_set_fd(srv.ssl, srv.s);
				if (wolfSSL_connect(srv.ssl) != 1) {
					char error[80] = { 0 };
					printf("E: SSL handshake between %s:%d and server failed: %s\n", cli.ipAddr, ntohs(cli.sAddr.sin_port),
						wolfSSL_ERR_error_string(wolfSSL_get_error(srv.ssl, -1), error)); wolfSSL_free(srv.ssl); closesocket(srv.s); goto acceptFail;
				}
			}
			ends.emplace_back(client); pf.emplace_back(pollfd{ cli.s,POLLIN,0 }); pf.emplace_back(pollfd{ srv.s,POLLIN,0 });
			printf("I: connection between %s:%d and server etablished successfully.\n", cli.ipAddr, ntohs(cli.sAddr.sin_port)); nfds += 2;
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
					if (i % 2) printf("I: %s:%d sent data to server (%d bytes): %s\n==end of data (%d bytes)==\n", ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port), received, buf, received);
					else	   printf("I: server sent data to %s:%d (%d bytes): %s\n==end of data (%d bytes)==\n", ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port), received, buf, received);
					if (this->hasSSL) wolfSSL_send(ends[(i - 1)/2].e[(i % 2) ? 1 : 0].ssl, buf, received, 0);
					else send(ends[(i - 1) / 2].e[(i % 2) ? 1 : 0].s, buf, received, 0);
				}
				else if (pf[i].revents & (POLLERR | POLLHUP)) {
					//printf("%s disconnected. Ending session...\n", (i) ? "[Server]" : "[Client]");
					if (i % 2) printf("I: %s:%d closed the connection with server\n", ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port));
					else	   printf("I: server closed the connection with %s:%d\n", ends[(i - 1) / 2].e[0].ipAddr, ntohs(ends[(i - 1) / 2].e[0].sAddr.sin_port));
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
