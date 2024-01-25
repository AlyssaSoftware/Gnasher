#include "Gnasher.h"
#define MODE_NORMAL 0
#define MODE_COMMAND 1
#define MODE_HISTORY 2
#define MODE_FILE 3
ClientSession::ClientSession(Params* p, bool amIServer) {
	// 0.5: Now we can run as ReverseClient, which means clients get connects to us and
	// we send them data from console just like as we were a client.
#define ServerTarget p->argv[2]
	if (p->argc < 3) {
		std::cout << "E: Parameter is missing. See \"" << p->argv[0] << " help\" for usage.\n"; exit(-1);
	}
	memset(&this->serv, 0, sizeof(this->serv)); int _ret;
	// Init sockets.
	this->serv.s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin; sin.sin_family = AF_INET;  socklen_t sz = sizeof(sin);
	if (amIServer) {//ReverseClient
		sin.sin_port = htons(atol(ServerTarget));
		_ret = inet_pton(AF_INET, "0.0.0.0", &sin.sin_addr);
		bind(serv.s, (sockaddr*)&sin, sz);
		listen(serv.s, SOMAXCONN);
		if (p->hasSSL) {
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
		if (p->hasSSL) {
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
	CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 }, csbi0 = { 0 }; INPUT_RECORD ir[5]; pollfd pf = { serv.s,POLLIN,0 };
	unsigned short curPos = 0, inputSz = 0;
	//0.6: Support for different input modes added. Such as normal input, command input, file transmission, and history feature etc.
	uint8_t inputMode = 0; wchar_t varBuffer[512] = { 0 }; unsigned short varPos = 0, varSz = 0;
	std::deque<wchar_t*> history; unsigned short histOffset = 0, histSz = 0, inputSz0 = 0; FILE* in = NULL;
	// end of 0.6
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
				if (!ke.bKeyDown) continue;// Ignore key release events.
				if (inputMode == MODE_FILE && ke.dwControlKeyState && ke.uChar.AsciiChar == 3) {
					// In case of an active file stream, only valid input is CTRL+C which aborts it.
					// Only handle it and ignore any other key stroke.
					goto FileAbort;
				}// Transmisson of file is done at the very bottom, out of the scope of this for.
				if (ke.dwControlKeyState & ENHANCED_KEY) {
					switch (ke.wVirtualKeyCode) {
						case VK_LEFT: //std::wcout << "Left arrow\n"; 
							if (!inputMode) {// Normal input
								if (!curPos) break;// Don't shift beyond beginning.
								curPos--;
							}
							else {// Variable input (command, etc.)
								if (!varPos) break;
								varPos--;
							}
							GetConsoleScreenBufferInfo(outHnd, &csbi);
							if (!csbi.dwCursorPosition.X) {// if at the beginning of line.
								csbi.dwCursorPosition.X = csbi.dwSize.X; csbi.dwCursorPosition.Y--;
							}
							else csbi.dwCursorPosition.X--;
							SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
							break;
						case VK_RIGHT: //std::wcout << "Right arrow\n"; 
							if (!inputMode) {// Same as above.
								if (curPos == inputSz) break;// Don't shift beyond end.
								curPos++;
							}
							else {
								if (varPos == varSz) break;
								varPos++;
							}
							GetConsoleScreenBufferInfo(outHnd, &csbi);
							if (csbi.dwCursorPosition.X==csbi.dwSize.X) {// if at the end of line.
								csbi.dwCursorPosition.X = 0; csbi.dwCursorPosition.Y++;
							}
							else csbi.dwCursorPosition.X++;
							SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
							break;
						case VK_UP:
							if (!histOffset) break;
							if (histOffset == histSz) inputSz0 = inputSz; // Save length of current input
							SetConsoleCursorPosition(outHnd, csbi0.dwCursorPosition); histOffset--; inputSz = wcslen(history[histOffset]);
							std::wcout << history[histOffset]; inputMode = MODE_HISTORY;
							curPos = inputSz; break;
						case VK_DOWN: 
							if (histOffset == histSz) break;// don't go beyond end of array.
							SetConsoleCursorPosition(outHnd, csbi0.dwCursorPosition); histOffset++;
							if (histOffset<histSz) { std::wcout << history[histOffset]; inputMode = MODE_HISTORY; }
							else { std::wcout << InputBuffer; inputMode = MODE_NORMAL; inputSz = inputSz0; }// We came to end of array, which is the current input.
							curPos = inputSz; break;
						case VK_RETURN: goto EnterKey; break;
						default: break;
					}
				}
				else if (ke.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
					switch (ke.uChar.AsciiChar) {
						case '\r': goto EnterKey; break;
						case 19 /*CTRL+S*/: 
						{
							if (inputMode==MODE_COMMAND) {// Handle a command.
HandleCommand:
								if (!wcsncmp(varBuffer, L"disconnect", 10)) {
									if (serv.ssl) wolfSSL_shutdown(serv.ssl);
									shutdown(serv.s, 2);
									if (out) fclose(out);
									return 0;
								}
								else if (!wcsncmp(varBuffer, L"file", 4)) {
									in = _wfopen(&varBuffer[5], L"rb");
									if (!in) std::cout << "\rFile opening failed." << std::endl;
									else std::cout << "\rFile transfer have started. Press CTRL+C for aborting it." << std::endl;
								}
								else {
									std::cout << "\rUnknown command." << std::endl;
								}
								memset(varBuffer, 0, varSz * 2); varSz = 0, varPos = 0;
								break;
							}
							else if (inputMode == MODE_HISTORY) {// We are sending an item in the history.
								if (history[histOffset] != history[histSz - 1]) {// Current item is different than last item, add it to the end.
									history.emplace_back(history[histOffset]); histSz++;
								}
							}
							else if (inputMode == MODE_NORMAL) {// Add the payload we sent to history.
								if (!inputSz) break;
								wchar_t* ptr = new wchar_t[inputSz + 1]; memcpy(ptr, InputBuffer, inputSz * 2); ptr[inputSz] = '\0';
								history.emplace_back(ptr); histSz++;
							}
							// Convert wchar to UTF-8 and send the payload.
							// (!inputMode) == (inputMode==MODE_NORMAL)
							int conv=WideCharToMultiByte(CP_UTF8, NULL, 
								(!inputMode) ? InputBuffer : history[histOffset],
								(!inputMode) ? wcslen(InputBuffer) : wcslen(history[histOffset]),
								SendBufConv, 4096, NULL, NULL);

							if (!conv) std::wcout << GetLastError();
							if (!this->serv.ssl) send(serv.s, SendBufConv, conv, 0);
							else wolfSSL_send(this->serv.ssl, SendBufConv, conv, 0);
							printf("\r[%f][%s] sent data (%d bytes):\n%ws\n", std::chrono::duration<float>(std::chrono::system_clock::now() - SessionBegin).count(),
								(serv.isServer) ? "Client" : "Server", conv,
								(!inputMode) ? InputBuffer : history[histOffset]);
							if (out) {
								float amk = time;
								memcpy(fbuf, &amk, 4); memcpy(&fbuf[4], &me.sin_addr, 4); memcpy(&fbuf[8], &me.sin_port, 2);
								memcpy(&fbuf[10], &serv.sAddr.sin_addr, 4); memcpy(&fbuf[14], &serv.sAddr.sin_port, 2); memcpy(&fbuf[16], &conv, 4);
								swab(&fbuf[8], &fbuf[8], 2); swab(&fbuf[14], &fbuf[14], 2); fwrite(fbuf, 20, 1, out); fwrite(SendBufConv, conv, 1, out); fflush(out);
							}
							memset(InputBuffer, 0, wcslen(InputBuffer) * 2); curPos = 0; inputSz = 0;
							GetConsoleScreenBufferInfo(outHnd, &csbi0);
							histOffset = histSz; inputMode = MODE_NORMAL; break;
						}
						case 3 /*CTRL+C*/: 
FileAbort:
							if (inputMode == MODE_FILE) {
								std::cout << "\rFile transfer aborted." << std::endl;
								inputMode = MODE_NORMAL;
								fclose(in); in = NULL;
							}
							break;
						case 1 /*CTRL+A*/: break;
						default /*Some other control key*/: break;
					}
				}
				else if(ke.wVirtualKeyCode==VK_RETURN) {// ENTER key
				EnterKey:
					if (inputMode) goto HandleCommand;
					//std::wcout << "\nENTER key\n"; 
					/*memcpy(&SendBuffer[BufPos], InputBuffer, curPos * 2); BufPos += curPos; BufSz += curPos;
					memset(InputBuffer, 0, curPos * 2); curPos = 0;*/
					InputBuffer[curPos] = '\r'; InputBuffer[curPos + 1] = '\n'; curPos += 2;
					std::wcout << std::endl;
				}

				else if (ke.wVirtualKeyCode == VK_ESCAPE) {// ESC key, enable/disable command mode.
					if (!inputMode) {//Switch from normal mode to command mode.
						std::cout << "\r\nCommand mode enabled." << std::endl; 
						GetConsoleScreenBufferInfo(outHnd, &csbi0); inputMode = 1;
					}
					else {
						// Clear buffer if there's something on it.
						if (varSz) { 
							memset(varBuffer, 0, varSz * 2); varSz = 0, varPos = 0; std::cout << '\r';
							for (int i = 0; i < varSz;i++) std::cout << ' ';
							std::cout << '\r';
						}
						// Get out of command mode if empty.
						else { 
							inputMode = 0; std::cout << "Command mode disabled." << std::endl;
							GetConsoleScreenBufferInfo(outHnd, &csbi0); std::wcout << InputBuffer; curPos = inputSz;
						}
					}
					//std::wcout << "\nESC key\n"; 
				}

				else if (ke.wVirtualKeyCode == VK_BACK) {// Backspace. && curPos is for making it not delete behind buffer.
					//0.6: Adding new pointer and sz for making same code work on either types of input.
					wchar_t* _buf = NULL; USHORT _pos = 0, _sz = 0;
					if (!inputMode) { if (!inputSz) continue; _buf = InputBuffer; _pos = curPos; _sz = inputSz; }
					else if (inputMode == MODE_COMMAND) { if (!varSz) continue; _buf = varBuffer; _pos = varPos; _sz = varSz; }
					else if (inputMode == MODE_HISTORY) {
						// Any keystroke while on history will be treated as item is edited.
						// Edited history items will be a separate item and will be appended at end of history.
						// And also will overwrite the input buffer.
						inputMode = MODE_NORMAL; inputSz = wcslen(history[histOffset]);
						memcpy(InputBuffer, history[histOffset], inputSz * 2); curPos = inputSz; histOffset = histSz;
					}
					USHORT origPos = _pos; _pos--;
					for (int x = 0; x < ke.wRepeatCount; x++) {
						if (_buf[_pos] == '\n') {//Deleted char may be the new line delimiter, which is 2 characters (\r\n)
							_buf[_pos] = 0;
							if (_buf[_pos - 1] == '\r') {
								_buf[_pos - 1] = 0; _pos -= 2; // ke.wRepeatCount++; x++;
							}
							else _pos--;
						}
						else {
							_buf[_pos] = 0; _pos--;
						}
					} _sz -= ke.wRepeatCount; _pos++;
					for (int x = 0; x < ke.wRepeatCount; x++) std::cout << "\b \b";
					if (_sz - 1 > _pos) {// There's data ahead of cursor position, shift that data back as well.
						GetConsoleScreenBufferInfo(outHnd, &csbi);
						memcpy(&_buf[_pos], &_buf[origPos], (_sz - _pos) * 2); _buf[_sz] = 0;
						printf("%ws", &_buf[_pos]); 
						for (int x = 0; x < ke.wRepeatCount; x++) std::cout << " \b \b"; // Delete leftovers at end
						SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
					}
					//0.6 again: set these shit back.
					if (!inputMode) { curPos = _pos; inputSz = _sz; }
					else { varPos = _pos; varSz = _sz; }
				}

				else if (ke.uChar.UnicodeChar>0) {// Any other standard character.
					//0.6: Exactly same thing on VK_BACK case.
					wchar_t* _buf = NULL; USHORT _pos = 0, _sz = 0;
					if (!inputMode) { _buf = InputBuffer; _pos = curPos; _sz = inputSz; }
					else if(inputMode==MODE_COMMAND) { _buf = varBuffer; _pos = varPos; _sz = varSz; }
					else if (inputMode == MODE_HISTORY) {
						// Any keystroke while on history will be treated as item is edited.
						// Edited history items will be a separate item and will be appended at end of history.
						// And also will overwrite the input buffer.
						inputMode = MODE_NORMAL; inputSz = wcslen(history[histOffset]);
						memcpy(InputBuffer, history[histOffset], inputSz * 2); curPos = inputSz; histOffset = histSz;
					}
					if (ke.wRepeatCount > 2048 - _sz - 1) ke.wRepeatCount = 2048 - _sz - 1; 
					if (!ke.wRepeatCount) goto PollSocket; //Input buffer is full.
					if (_pos < _sz) {// Cursor is not at end, which means we are going to shift the buffer.
						memcpy(&_buf[_pos + ke.wRepeatCount], &_buf[_pos], (_sz - _pos) * 2);
						GetConsoleScreenBufferInfo(outHnd, &csbi);
					}
					for (int x = 0; x < ke.wRepeatCount; x++) {
						_buf[_pos] = ke.uChar.UnicodeChar; // We would use memset instead if it wasn't widechar.
					}
					printf("%ws", &_buf[_pos]); _pos += ke.wRepeatCount; _sz += ke.wRepeatCount;
					if (_pos < _sz) {// Set cursor position to where it was after printing.
						csbi.dwCursorPosition.X++; SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
					}
					if (!inputMode) { curPos = _pos; inputSz = _sz; }
					else { varPos = _pos; varSz = _sz; }
				}
			}

		}
	// Read and send the file if there is.
		if (inputMode == MODE_FILE) {
			int sz = fread(InputBuffer, 4096, 1, in);
			if (serv.ssl) wolfSSL_send(serv.ssl, InputBuffer, sz, 0);
			else send(serv.s, (char*)InputBuffer, sz, 0);
			if (out) {
				float amk = time;
				memcpy(fbuf, &amk, 4); memcpy(&fbuf[4], &serv.sAddr.sin_addr, 4); memcpy(&fbuf[8], &serv.sAddr.sin_port, 2);
				memcpy(&fbuf[10], &me.sin_addr, 4); memcpy(&fbuf[14], &me.sin_port, 2); memcpy(&fbuf[16], &sz, 4);
				swab(&fbuf[8], &fbuf[8], 2); swab(&fbuf[14], &fbuf[14], 2); fwrite(fbuf, 20, 1, out); fwrite(RecvBuffer, sz, 1, out); fflush(out);
			}
			if (feof(in)) {
				std::cout << "\rFile transfer completed." << std::endl;
				fclose(in); in = NULL; inputMode = MODE_NORMAL; memset(InputBuffer, 0, sz);
			}
		}
	PollSocket:
		if (WSAPoll(&pf, 1, 10)) {
			if (pf.revents & POLLIN) {
				if (!this->serv.ssl) received = recv(pf.fd, RecvBuffer, 2048, 0);
				else received = wolfSSL_recv(this->serv.ssl, RecvBuffer, 2048, 0);
				RecvBuffer[received] = '\0';
				printf("\r[%f][%s] sent data (%d bytes):\n%s(end of data)\n", time, (!serv.isServer) ? "Client" : "Server", received, RecvBuffer);
				GetConsoleScreenBufferInfo(outHnd, &csbi0); std::wcout << InputBuffer;
				// For now reset cursor position to end.
				curPos = inputSz;
				if (out) {
					float amk = time;
					memcpy(fbuf, &amk, 4); memcpy(&fbuf[4], &serv.sAddr.sin_addr, 4); memcpy(&fbuf[8], &serv.sAddr.sin_port, 2);
					memcpy(&fbuf[10], &me.sin_addr, 4); memcpy(&fbuf[14], &me.sin_port, 2); memcpy(&fbuf[16], &received, 4);
					swab(&fbuf[8], &fbuf[8], 2); swab(&fbuf[14], &fbuf[14], 2); fwrite(fbuf, 20, 1, out); fwrite(RecvBuffer, received, 1, out); fflush(out);
				}
			}
			else if (pf.revents & POLLHUP) {
				printf("\r[%f][%s] disconnected. Ending session...\n", time, (!serv.isServer) ? "Client" : "Server"); 
				return 0;
			}
		}
	}
}
