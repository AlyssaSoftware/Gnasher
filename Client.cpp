#include "Gnasher.h"

ClientSession::ClientSession(char* ServerTarget, bool hasSSL, bool amIServer) {
	// 0.5: Now we can run as ReverseClient, which means clients get connects to us and
	// we send them data from console just like as we were a client.
	memset(&this->serv, 0, sizeof(this->serv)); int _ret;
	// Init sockets.
	this->serv.s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin; sin.sin_family = AF_INET;  socklen_t sz = sizeof(sin);
	if (amIServer) {//ReverseClient
		sin.sin_port = htons(atol(ServerTarget));
		_ret = inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
		bind(serv.s, (sockaddr*)&sin, sz);
		listen(serv.s, SOMAXCONN);
		if (hasSSL) {
			this->serv.ssl = wolfSSL_new(ctx_server); wolfSSL_set_fd(this->serv.ssl, this->serv.s);
		}
	}
	else {// Plain client
		this->serv.ipAddr = ServerTarget; this->serv.isServer = 1;
		char* pos;
		if (pos = std::find(ServerTarget, ServerTarget + strlen(ServerTarget), ':')) {
			sin.sin_port = htons(std::atoi(pos + 1)); pos[0] = '\0';
		}
		_ret = inet_pton(AF_INET, ServerTarget, &sin.sin_addr);
		// Set SSL
		if (hasSSL) {
			this->serv.ssl = wolfSSL_new(ctx);
		}
	}
	// Allocate buffers
	this->serv.sAddr = sin;
	this->InputBuffer = new wchar_t[2048]; memset(InputBuffer, 0, 4096);
	this->RecvBuffer = new char[2048]; memset(RecvBuffer, 0, 2048);
	this->SendBufConv = new char[4096]; memset(SendBufConv, 0, 4096);
	return;
}

ClientSession::~ClientSession() {
	shutdown(this->serv.s, 2); closesocket(this->serv.s);
	delete[] this->InputBuffer; delete[] this->RecvBuffer; delete[] this->SendBufConv;
	if (this->serv.ssl) wolfSSL_free(this->serv.ssl);
}

int ClientSession::InitSession() {
	if (serv.isServer) {// Other peer is a server, so we are a client
		if (connect(this->serv.s, (sockaddr*)&this->serv.sAddr, sizeof sockaddr_in)) {
			std::cout << "[System] E: Connection to server failed: " << WSAGetLastError() << std::endl; return -1;
		}
		if (wolfSSL_connect(this->serv.ssl) == -1) {
			char error[80] = { 0 };
			std::cout << "[System] E: SSL handshake failed: " << wolfSSL_ERR_error_string(wolfSSL_get_error(this->serv.ssl, -1), error) << std::endl; return -1;
		}
		socklen_t namelen = sizeof(me);
		getsockname(serv.s, (sockaddr*)&me, &namelen);
	}
	else {//Remote peer is a client, so we are a ReverseClient
		printf("[System] I: ReverseClient session started successfully. Listening on %d\n", ntohs(serv.sAddr.sin_port));
		sockaddr_in client; socklen_t sz = sizeof client;
		SOCKET s = accept(serv.s, (sockaddr*)&client, &sz);
		if (s == INVALID_SOCKET) std::terminate();
		printf("[System] I: %s:%d is connecting...\n", inet_ntoa(serv.sAddr.sin_addr), ntohs(serv.sAddr.sin_port));
		if (serv.ssl) {
			wolfSSL_set_fd(serv.ssl, s);
			if (wolfSSL_accept(serv.ssl) == -1) {
				char error[80] = { 0 };
				printf("[System] E: SSL handshake with client failed: %s\n", wolfSSL_ERR_error_string(wolfSSL_get_error(serv.ssl, -1), error));  
				wolfSSL_free(serv.ssl); closesocket(s);
				s = accept(serv.s, (sockaddr*)&client, &sz);
				if (s == INVALID_SOCKET) std::terminate();
				serv.ssl = wolfSSL_new(ctx_server); wolfSSL_set_fd(serv.ssl, s);
				if (wolfSSL_accept(serv.ssl) == -1) {
					printf("[System] E: SSL handshake with client failed again, giving up: %s\n", wolfSSL_ERR_error_string(wolfSSL_get_error(serv.ssl, -1), error));
					// wolfSSL_free(serv.ssl); 
					closesocket(s); closesocket(serv.s); return -1;
				}
			}
		}
		serv.s = s; serv.sAddr = client;
		printf("[System] I: Connection established successfully.\n");
	}
	SessionBegin = std::chrono::system_clock::now(); return 0;
}

int ClientSession::SessionLoop() {// This function handles the whole console interface and reading data from client.
	HANDLE inHnd = GetStdHandle(STD_INPUT_HANDLE); DWORD readnum, received;
	HANDLE outHnd = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleMode(inHnd, (!ENABLE_LINE_INPUT & !ENABLE_WINDOW_INPUT & !ENABLE_MOUSE_INPUT)); // Disable cooked mode on console.
	CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 }; INPUT_RECORD ir[5]; pollfd pf = { serv.s,POLLIN,0 };
	while (true) {
		//if (!WaitForSingleObject(inHnd, 10)) goto PollSocket;
		GetNumberOfConsoleInputEvents(inHnd, &readnum); if (!readnum) goto PollSocket;

		if (!ReadConsoleInputW(inHnd, ir, 5, &readnum)) {
			std::cout << "ReadConsoleInputW  failed: " << GetLastError();
			std::terminate();
		}
		//this->ConMtx.lock();
		for (size_t i = 0; i < readnum; i++) {
#define ke ir[i].Event.KeyEvent
			if (ir[i].EventType == KEY_EVENT) {
				if (!ir[i].Event.KeyEvent.bKeyDown) continue;

				if (ir[i].Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) {
					switch (ke.wVirtualKeyCode) {
						case VK_LEFT: //std::wcout << "Left arrow\n"; 
							if (!CurPos) break;// Don't shift beyond beginning.
							CurPos--; GetConsoleScreenBufferInfo(outHnd, &csbi);
							if (!csbi.dwCursorPosition.X) {// if at the beginning of line.
								csbi.dwCursorPosition.X = csbi.dwSize.X; csbi.dwCursorPosition.Y--;
							}
							else csbi.dwCursorPosition.X--;
							SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
							break;
						case VK_RIGHT: //std::wcout << "Right arrow\n"; 
							if (CurPos==InputSz) break;// Don't shift beyond end.
							CurPos++; GetConsoleScreenBufferInfo(outHnd, &csbi);
							if (csbi.dwCursorPosition.X==csbi.dwSize.X) {// if at the end of line.
								csbi.dwCursorPosition.X = 0; csbi.dwCursorPosition.Y++;
							}
							else csbi.dwCursorPosition.X++;
							SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
							break;
						case VK_UP: GetConsoleScreenBufferInfo(outHnd, &csbi); break;
						case VK_DOWN: break;
						case VK_RETURN: goto EnterKey; break;
						default: break;
					}
				}
				else if (ke.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
					switch (ke.uChar.AsciiChar) {
						case '\r': goto EnterKey; break;
						case 19 /*CTRL+S*/: 
						{
							int conv=WideCharToMultiByte(CP_UTF8, NULL, InputBuffer, wcslen(InputBuffer), SendBufConv, 4096, NULL, NULL);
							if (!conv) std::wcout << GetLastError();
							if (!this->serv.ssl) send(serv.s, SendBufConv, conv, 0);
							else wolfSSL_send(this->serv.ssl, SendBufConv, conv, 0);
							printf("\r[%f][%s] sent data (%d bytes):\n%ws\n", std::chrono::duration<float>(std::chrono::system_clock::now() - SessionBegin).count(),
								(serv.isServer) ? "Client" : "Server", conv, InputBuffer);
							if (out) {
								float amk = time;
								memcpy(fbuf, &amk, 4); memcpy(&fbuf[4], &me.sin_addr, 4); memcpy(&fbuf[8], &me.sin_port, 2);
								memcpy(&fbuf[10], &serv.sAddr.sin_addr, 4); memcpy(&fbuf[14], &serv.sAddr.sin_port, 2); memcpy(&fbuf[16], &conv, 4);
								swab(&fbuf[8], &fbuf[8], 2); swab(&fbuf[14], &fbuf[14], 2); fwrite(fbuf, 20, 1, out); fwrite(SendBufConv, conv, 1, out); fflush(out);
							}
							memset(InputBuffer, 0, wcslen(InputBuffer) * 2); CurPos = 0; InputSz = 0;
							break;
						}
						case 3 /*CTRL+C*/: break;
						case 1 /*CTRL+A*/: break;
						default /*Some other control key*/: break;
					}
				}
				else if(ke.wVirtualKeyCode==VK_RETURN) {// ENTER key
EnterKey:
					//std::wcout << "\nENTER key\n"; 
					/*memcpy(&SendBuffer[BufPos], InputBuffer, CurPos * 2); BufPos += CurPos; BufSz += CurPos;
					memset(InputBuffer, 0, CurPos * 2); CurPos = 0;*/
					InputBuffer[CurPos] = '\r'; InputBuffer[CurPos + 1] = '\n'; CurPos += 2;
					std::wcout << std::endl;
				}

				else if (ke.wVirtualKeyCode == VK_ESCAPE) {// ESC key
					//std::wcout << "\nESC key\n"; 
				}

				else if (ke.wVirtualKeyCode == VK_BACK && CurPos) {// Backspace. && CurPos is for making it not delete behind buffer.
					USHORT origPos = CurPos; CurPos--;
					for (int x = 0; x < ke.wRepeatCount; x++) {
						if (InputBuffer[CurPos] == '\n') {//Deleted char may be the new line delimiter, which is 2 characters (\r\n)
							InputBuffer[CurPos] = 0;
							if (InputBuffer[CurPos - 1] == '\r') {
								InputBuffer[CurPos - 1] = 0; CurPos -= 2; // ke.wRepeatCount++; x++;
							}
							else CurPos--;
						}
						else {
							InputBuffer[CurPos] = 0; CurPos--;
						}
					} InputSz -= ke.wRepeatCount; CurPos++;
					for (int x = 0; x < ke.wRepeatCount; x++) std::wcout << "\b \b";
					if (InputSz - 1 > CurPos) {// There's data ahead of cursor position, shift that data back as well.
						GetConsoleScreenBufferInfo(outHnd, &csbi);
						memcpy(&InputBuffer[CurPos], &InputBuffer[origPos], (InputSz - CurPos) * 2); InputBuffer[InputSz] = 0;
						printf("%ws", &InputBuffer[CurPos]); 
						for (int x = 0; x < ke.wRepeatCount; x++) std::wcout << " \b \b"; // Delete leftovers at end
						SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
					}
				}

				else if (ke.uChar.UnicodeChar>0) {// Any other standard character.
					if (ke.wRepeatCount > 2048 - InputSz - 1) ke.wRepeatCount = 2048 - InputSz - 1; 
					if (!ke.wRepeatCount) goto PollSocket; //Input buffer is full.
					if (CurPos < InputSz) {// Cursor is not at end, which means we are going to shift the buffer.
						memcpy(&InputBuffer[CurPos + ke.wRepeatCount], &InputBuffer[CurPos], (InputSz - CurPos) * 2);
						GetConsoleScreenBufferInfo(outHnd, &csbi);
					}
					for (int x = 0; x < ke.wRepeatCount; x++) {
						InputBuffer[CurPos] = ke.uChar.UnicodeChar; // We would use memset instead if it wasn't widechar.
					}
					printf("%ws", &InputBuffer[CurPos]); CurPos += ke.wRepeatCount; InputSz += ke.wRepeatCount;
					if (CurPos < InputSz) {// Set cursor position to where it was after printing.
						csbi.dwCursorPosition.X++; SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
					}
				}
			}

		}
		//this->ConMtx.unlock();
	PollSocket:
		if (WSAPoll(&pf, 1, 10)) {
			if (pf.revents & POLLIN) {
				if (!this->serv.ssl) received = recv(pf.fd, RecvBuffer, 2048, 0);
				else received = wolfSSL_recv(this->serv.ssl, RecvBuffer, 2048, 0);
				RecvBuffer[received] = '\0';
				printf("\r[%f][%s] sent data (%d bytes):\n%s\n%ws", time, (serv.isServer) ? "Client" : "Server", received, RecvBuffer, InputBuffer);
				if (out) {
					float amk = time;
					memcpy(fbuf, &amk, 4); memcpy(&fbuf[4], &serv.sAddr.sin_addr, 4); memcpy(&fbuf[8], &serv.sAddr.sin_port, 2);
					memcpy(&fbuf[10], &me.sin_addr, 4); memcpy(&fbuf[14], &me.sin_port, 2); memcpy(&fbuf[16], &received, 4);
					swab(&fbuf[8], &fbuf[8], 2); swab(&fbuf[14], &fbuf[14], 2); fwrite(fbuf, 20, 1, out); fwrite(RecvBuffer, received, 1, out); fflush(out);
				}
			}
			else if (pf.revents & POLLHUP) {
				printf("\r[%f][%s] disconnected. Ending session...\n", time, (serv.isServer) ? "Client" : "Server"); 
				return 0;
			}
		}
	}
}
