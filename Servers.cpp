// This file contains the server implementations (i.e. echo server)
#include "Gnasher.h"

void EchoServer() {
	std::vector<pollfd> arr; //std::deque<sockaddr> adarr; adarr.emplace_back(); // We don't need memory integrity for sockaddrs.
	SOCKET listening = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); sockaddr_in sin;
	// Set listening socket. Hardcoded to socket 666 for now.
	sin.sin_port = htons(666); sin.sin_family = AF_INET;
	int _ret = inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
	socklen_t sz = sizeof(sin);
	_ret = bind(listening, (sockaddr*)&sin, sz);
	_ret = listen(listening, SOMAXCONN);
	if (_ret) std::terminate(); int scnt = 0, received = 0;
	arr.emplace_back(pollfd{ listening,POLLIN,0 });
	char* buf = new char[2048];

	std::wcout << "[Server] started succesfuly. Listening on " << 666 << std::endl;

	while (true) {
		scnt = WSAPoll(arr.data(), arr.size(), -1);
		if (arr[0].revents & POLLIN) {// Check for listening socket first. if true, there's an incoming connection.
			sockaddr_in cliaddr; int clisz = sizeof cliaddr;
			arr.emplace_back(pollfd{ accept(listening,(sockaddr*)&cliaddr,&clisz),POLLIN,0 });
		}
		// Check for the rest of sockets.
		for (int i = 1; i < arr.size() && scnt; i++) {
			if (arr[i].revents & POLLIN) {// Incoming data.
				received = recv(arr[i].fd, buf, 2048, 0);
				send(arr[i].fd, buf, received, 0); scnt--;
			}
			else if (arr[i].revents & POLLHUP) {// Connection lost, remove from sockets array.
				shutdown(arr[i].fd, 2); closesocket(arr[i].fd);
				arr.erase(arr.begin() + i); i--; scnt--;
			}
		}
	}
}

void LoopServer() {
	std::vector<pollfd> arr; //std::deque<sockaddr> adarr; adarr.emplace_back(); // We don't need memory integrity for sockaddrs.
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
	int timeout = 1000;

	std::wcout << "[Server] started succesfuly. Listening on " << 666 << std::endl;

	while (true) {
		for (int i = 0; i < timeout; i+=100) {// Check for incoming connection every 100 msecs until timeout expires
			if (WSAPoll(arr.data(), 1, 100)) {
				sockaddr_in cliaddr; int clisz = sizeof cliaddr;
				arr.emplace_back(pollfd{ accept(listening,(sockaddr*)&cliaddr,&clisz),POLLOUT,0 });
			}
		}
		scnt = WSAPoll(arr.data(), arr.size(), -1);
		if (arr[0].revents & POLLIN) {// Check for listening socket again.
			sockaddr_in cliaddr; int clisz = sizeof cliaddr;
			arr.emplace_back(pollfd{ accept(listening,(sockaddr*)&cliaddr,&clisz),POLLOUT,0 });
		}
		// Check for the rest of sockets if they are ready to write.
		for (int i = 1; i < arr.size() && scnt; i++) {
			if (arr[i].revents & POLLOUT) {// Incoming data.
				send(arr[i].fd, buf, strlen(buf), 0); scnt--;
			}
			else if (arr[i].revents & POLLHUP) {// Connection lost, remove from sockets array.
				shutdown(arr[i].fd, 2); closesocket(arr[i].fd);
				arr.erase(arr.begin() + i); i--; scnt--;
			}
		}
	}
}