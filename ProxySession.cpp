#include "Gnasher.h"

ProxySession::ProxySession(char* ServerTarget, bool hasSSL) {
	memset(&this->ends, 0, sizeof(this->ends)); int _ret;
	this->ends[1].ipAddr = ServerTarget; this->ends[1].isServer = 1;
	// Init sockets.
	this->ends[0].s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	this->ends[1].s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin; sin.sin_family = AF_INET; 
	// Set server socket which we will connect to.
	char* pos; 
	if (pos = std::find(ServerTarget, ServerTarget + strlen(ServerTarget), ':')) {
		sin.sin_port = htons(std::atoi(pos+1)); pos[0] = '\0';
	}
	_ret=inet_pton(AF_INET, ServerTarget, &sin.sin_addr);
	socklen_t sz = sizeof(sin);
	this->ends[1].sAddr = sin;
	// Set client socket to listening. Hardcoded to socket 666 for now.
	sin.sin_port = htons(666);
	_ret=inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
	sz = sizeof(sin);
	_ret=bind(this->ends[0].s, (sockaddr*)&sin, sz);
	_ret=listen(this->ends[0].s, SOMAXCONN);
	// Set SSL structures
	if (hasSSL) {
		this->ends[0].ssl = wolfSSL_new(ctx_server);
		this->ends[1].ssl = wolfSSL_new(ctx);
		wolfSSL_set_fd(this->ends[1].ssl, this->ends[1].s);
	}
	return;
}

ProxySession::~ProxySession() {
	shutdown(this->ends[0].s, 2); closesocket(this->ends[0].s);
	shutdown(this->ends[1].s, 2); closesocket(this->ends[1].s);
	if (this->ends[0].ssl) wolfSSL_free(this->ends[0].ssl);
	if (this->ends[1].ssl) wolfSSL_free(this->ends[1].ssl);
}

int ProxySession::InitSession() {
	std::cout << "[System] I: Waiting for a connection... " << std::endl;
	pollfd pf[1]; pf[0].fd = this->ends[0].s; pf->events = POLLIN;
	/*
		You point to the trail where the blossoms have fallen
		But all I can see is the poll()en, fucking me up
		Everything moves too fast but I've
		Been doing the same thing a thousand times over
		But I'm brought to my knees by the clover
		And it feels like, it's just the poll()en
	*/
	if (!WSAPoll(&pf[0], 1, 10000)) { // Wait for 10 secs for client to connect.
		std::cout << "[System] E: Client connection timed out." << std::endl; return -2;
	}
	else if (pf[0].revents & POLLIN) { // Got a connection
#ifndef _WIN32
		unsigned int clientSize = sizeof(this->ends[0].sAddr);
#else
		int clientSize = sizeof(this->ends[0].sAddr);
#endif
		this->ends[0].s = accept(pf[0].fd, (sockaddr*)&this->ends[0].sAddr, &clientSize); wolfSSL_set_fd(this->ends[0].ssl, this->ends[0].s);
		if (ends[0].ssl) {// Do SSL handshake with client.
			if (wolfSSL_accept(ends[0].ssl) != 1) {// If fails, try again.
				char error[80] = { 0 };
				std::cout << "[System] E: SSL handshake with client failed: " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->ends[0].ssl, -1), error) << ". Trying again..." <<std::endl;
				if (!WSAPoll(&pf[0], 1, 1000)) goto SSLFail; // Client didn't connect again, give up.

				this->ends[0].s = accept(pf[0].fd, (sockaddr*)&this->ends[0].sAddr, &clientSize); wolfSSL_free(this->ends[0].ssl);
				this->ends[0].ssl = wolfSSL_new(ctx_server); wolfSSL_set_fd(this->ends[0].ssl, this->ends[0].s);

				if (ends[0].ssl) {// Do SSL handshake with client again.
					if (wolfSSL_accept(ends[0].ssl) != 1) {// Failed again, give up.
SSLFail:
						std::cout << "[System] E: SSL handshake with client failed (attempt 2, giving up.): " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->ends[0].ssl, -1), error) << std::endl; return -1;
					}
				}
			}
		}
		std::cout << "[System] I: Client connected, connecting to server..." << std::endl;
		// Connect to server after client connects.
		if (connect(this->ends[1].s, (sockaddr*)&this->ends[1].sAddr, sizeof sockaddr_in)) {
			std::cout << "[System] E: Connection to server failed: " << WSAGetLastError() << std::endl; return -1;
		}
		if (ends[1].ssl) {// // Do SSL handshake with server.
			if (wolfSSL_connect(ends[1].ssl) != 1) {
				char error[80] = { 0 };
				std::cout << "[System] E: SSL handshake with server failed: " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->ends[1].ssl, -1), error) << std::endl; return -1;
			}
		}
		std::cout << "[System] I: Proxy session initiated successfully." << std::endl;
		return 0;
	}
	else { // Error.
		std::cout << "[System] E: Client connection polling failed." << std::endl; return -2;
	}
}

int ProxySession::SessionLoop() {
	pollfd pf[2]; pf[0].fd = this->ends[0].s; pf[0].events = POLLIN;
				  pf[1].fd = this->ends[1].s; pf[1].events = POLLIN;
	char* buf = new char[32768]; unsigned short received = 0;
	memset(buf, 0, 32768);
	short pollcnt = 0;
	while ((pollcnt = WSAPoll(&pf[0], 2, -1)) > 0) {
		for (int8_t i = 0; i < 2; i++) {
			if (pf[i].revents) {// There's an event in such socket.
				// i==1 -> it's the server, else it's the client.
				if (pf[i].revents & POLLIN) {// Socket sent data.
					if (this->ends[i].ssl) received = wolfSSL_recv(this->ends[i].ssl, buf, 32767, 0);
					else received = recv(pf[i].fd, buf, 32767, 0); 
					buf[received] = '\0';
					printf("%s sent data (%d bytes): %s\n==end of data (%d bytes)==\n", (i) ? "[Server]" : "[Client]", received, buf, received);
					// Send that data to other socket
					if (this->ends[!i].ssl) wolfSSL_send(this->ends[!i].ssl, buf, received, 0);
					else send(pf[!i].fd, buf, received, 0);
				}
				else if (pf[i].revents & (POLLERR | POLLHUP)) {
					printf("%s disconnected. Ending session...\n", (i) ? "[Server]" : "[Client]");
					return 0;
				}
			}
		}
	}
	// Jumped out of poll, not supposed to happen.
	std::terminate();
	return 0;
}
