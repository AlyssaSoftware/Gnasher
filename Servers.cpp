// This file contains the server implementations (i.e. echo server)
#include "Gnasher.h"

void EchoServer(bool hasSSL) {
	std::vector<pollfd> arr; std::deque<sockaddr_in> adarr; adarr.emplace_back(); std::deque<WOLFSSL*> ssl;
	SOCKET listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); sockaddr_in sin;
	// Set listening socket. Hardcoded to socket 666 for now.
	sin.sin_port = htons(667); sin.sin_family = AF_INET;
	int _ret = inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
	socklen_t sz = sizeof(sin);
	_ret = bind(listening, (sockaddr*)&sin, sz);
	_ret = listen(listening, SOMAXCONN);
	if (_ret) std::terminate(); int scnt = 0, received = 0;
	arr.emplace_back(pollfd{ listening,POLLIN,0 });
	char* buf = new char[2048]; char error[80] = { 0 };

	printf("I: started successfully. Listening on %d\n", ntohs(sin.sin_port));
	SessionBegin = std::chrono::system_clock::now();

	while (true) {
		scnt = WSAPoll(arr.data(), arr.size(), -1);
		if (arr[0].revents & POLLIN) {// Check for listening socket first. if true, there's an incoming connection.
			sockaddr_in cliaddr; int clisz = sizeof cliaddr;
			SOCKET sk = accept(listening, (sockaddr*)&cliaddr, &clisz);
			if (sk == INVALID_SOCKET) goto listenOut;
			if (hasSSL) {
				WOLFSSL* s = wolfSSL_new(ctx_server); wolfSSL_set_fd(s, sk);
				if (wolfSSL_accept(s) != 1) {
					printf("[%f] E: SSL handshake failed: %s\n", time, wolfSSL_ERR_error_string(wolfSSL_get_error(s, -1), error)); goto listenOut;
				}
				ssl.emplace_back(s);
			}
			arr.emplace_back(pollfd{sk,POLLIN,0}); adarr.emplace_back(cliaddr);
			printf("[%f] I: %s:%d is connected.\n", time, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		}
listenOut:
		// Check for the rest of sockets.
		for (int i = 1; i < arr.size() && scnt; i++) {
			if (arr[i].revents & POLLIN) {// Incoming data.
				if (hasSSL) { received = wolfSSL_send(ssl[i], buf, 2048, 0); wolfSSL_send(ssl[i], buf, received, 0); }
				else		{ received = recv(arr[i].fd, buf, 2048, 0); send(arr[i].fd, buf, received, 0); }
				scnt--; buf[received] = 0;
				printf("[%f] I: %s:%d sent data (%d bytes): %s\n", time, inet_ntoa(adarr[i].sin_addr), ntohs(adarr[i].sin_port), received, buf);
			}
			else if (arr[i].revents & POLLHUP) {// Connection lost, remove from sockets array.
				printf("[%f] I: %s:%d has disconnected from server.\n", time, inet_ntoa(adarr[i].sin_addr), ntohs(adarr[i].sin_port));
				shutdown(arr[i].fd, 2); closesocket(arr[i].fd);
				arr.erase(arr.begin() + i); adarr.erase(adarr.begin() + i);
				if (hasSSL) {
					wolfSSL_free(ssl[i]); ssl.erase(ssl.begin() + i);
				}
				i--; scnt--;
			}
		}
	}
}

void LoopServer(bool hasSSL) {
	std::vector<pollfd> arr; std::deque<sockaddr_in> adarr; adarr.emplace_back(); std::deque<WOLFSSL*> ssl;
	SOCKET listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); sockaddr_in sin;
	// Set listening socket. Hardcoded to socket 666 for now.
	sin.sin_port = htons(666); sin.sin_family = AF_INET;
	int _ret = inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
	socklen_t sz = sizeof(sin);
	_ret = bind(listening, (sockaddr*)&sin, sz);
	_ret = listen(listening, SOMAXCONN);
	if (_ret) std::terminate(); int scnt = 0, received = 0;
	arr.emplace_back(pollfd{ listening,POLLIN,0 });
	const char* buf = "Dummy data.";
	int timeout = 1000; char error[80] = { 0 };

	printf("I: started successfully. Listening on %d, payload is: \"%s\" (%d bytes)\n", 666, buf, strlen(buf));
	SessionBegin = std::chrono::system_clock::now();

	while (true) {
		for (int i = 0; i < timeout; i+=100) {// Check for incoming connection every 100 msecs until timeout expires
			if (WSAPoll(arr.data(), 1, 100)) {
				sockaddr_in cliaddr; int clisz = sizeof cliaddr;
				SOCKET sk = accept(listening, (sockaddr*)&cliaddr, &clisz);
				if (sk == INVALID_SOCKET) continue;
				if (hasSSL) {
					WOLFSSL* s = wolfSSL_new(ctx_server); wolfSSL_set_fd(s, sk);
					if (wolfSSL_accept(s) != 1) {
						printf("[%f] E: SSL handshake failed: %s\n", time, wolfSSL_ERR_error_string(wolfSSL_get_error(s, -1), error)); continue;
					}
					ssl.emplace_back(s);
				}
				arr.emplace_back(pollfd{ sk,POLLIN,0 }); adarr.emplace_back(cliaddr);
				printf("[%f] I: %s:%d is connected.\n", time, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
			}
		}
		scnt = WSAPoll(arr.data(), arr.size(), -1);
		printf("[%f] I: timeout has expired, sending payload to %d clients.", time, arr.size() - 1);
		// Check for the rest of sockets if they are ready to write.
		for (int i = 1; i < arr.size() && scnt; i++) {
			if (arr[i].revents & POLLOUT) {// Ready to write.
				if (hasSSL) wolfSSL_send(ssl[i], buf, strlen(buf), 0);
				else		send(arr[i].fd, buf, strlen(buf), 0);
				scnt--;
			}
			else if (arr[i].revents & POLLHUP) {// Connection lost, remove from sockets array.
				printf("[%f] I: %s:%d has disconnected from server.\n", time, inet_ntoa(adarr[i].sin_addr), ntohs(adarr[i].sin_port));
				shutdown(arr[i].fd, 2); closesocket(arr[i].fd);
				arr.erase(arr.begin() + i); adarr.erase(adarr.begin() + i);
				if (hasSSL) {
					wolfSSL_free(ssl[i]); ssl.erase(ssl.begin() + i);
				}
				i--; scnt--;
			}
		}
	}
}
