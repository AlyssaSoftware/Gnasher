#include "Gnasher.h"

ClientSession::ClientSession(char* ServerTarget) {
	memset(&this->serv, 0, sizeof(this->serv)); int _ret;
	this->serv.ipAddr = ServerTarget; this->serv.isServer = 1;
	// Init sockets.
	this->serv.s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	this->serv.s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in sin; sin.sin_family = AF_INET;
	// Set server socket which we will connect to.
	char* pos;
	if (pos = std::find(ServerTarget, ServerTarget + strlen(ServerTarget), ':')) {
		sin.sin_port = htons(std::atoi(pos + 1)); pos[0] = '\0';
	}
	_ret = inet_pton(AF_INET, ServerTarget, &sin.sin_addr);
	socklen_t sz = sizeof(sin);
	this->serv.sAddr = sin;
	this->InputBuffer = new wchar_t[2048]; memset(InputBuffer, 0, 4096);
	this->RecvBuffer = new char[2048]; memset(RecvBuffer, 0, 2048);
	this->SendBufConv = new char[4096]; memset(SendBufConv, 0, 4096);
	return;
}

ClientSession::~ClientSession() {
	shutdown(this->serv.s, 2); closesocket(this->serv.s);
	delete[] this->InputBuffer; delete[] this->RecvBuffer; delete[] this->SendBufConv;
}

int ClientSession::InitSession() {
	if (connect(this->serv.s, (sockaddr*)&this->serv.sAddr, sizeof sockaddr_in)) {
		std::cout << "[System] E: Connection to server failed: " << WSAGetLastError() << std::endl; return -1;
	}
	return 0;
}

int ClientSession::SessionLoop() {// This function handles the whole console interface and reading data from client.
	HANDLE inHnd = GetStdHandle(STD_INPUT_HANDLE); DWORD readnum;
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
							send(serv.s, SendBufConv, conv, 0); printf("[Client] sent data (%d bytes):\n %ws\n", conv, InputBuffer);
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
					} InputSz -= ke.wRepeatCount;
					for (int x = 0; x < ke.wRepeatCount; x++) std::wcout << "\b \b";
					if (InputSz - 1 > CurPos) {// There's data ahead of cursor position, shift that data back as well.
						GetConsoleScreenBufferInfo(outHnd, &csbi);
						/*if (ke.wRepeatCount > csbi.dwCursorPosition.X) { csbi.dwCursorPosition.Y--; csbi.dwCursorPosition.X += csbi.dwSize.X; }
						csbi.dwCursorPosition.X -= ke.wRepeatCount;
						SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);*/
						CurPos++; memcpy(&InputBuffer[CurPos], &InputBuffer[origPos], (InputSz - CurPos) * 2); InputBuffer[InputSz] = 0;
						printf("%ws", &InputBuffer[CurPos]); 
						for (int x = 0; x < ke.wRepeatCount; x++) std::wcout << " \b \b"; // Delete leftovers at end
						SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
					}
				}

				else if (ke.uChar.UnicodeChar>0) {// Any other standard character.
					if (ke.wRepeatCount > 2048 - InputSz - 1) ke.wRepeatCount = 2048 - InputSz; 
					if (!ke.wRepeatCount) goto PollSocket; //Input buffer is full.
					if (CurPos < InputSz - 1) {// Cursor is not at end, which means we are going to shift the buffer.
						memcpy(&InputBuffer[CurPos + ke.wRepeatCount], &InputBuffer[CurPos], (InputSz - CurPos) * 2);
						GetConsoleScreenBufferInfo(outHnd, &csbi);
					}
					for (int x = 0; x < ke.wRepeatCount; x++) {
						InputBuffer[CurPos] = ke.uChar.UnicodeChar; // We would use memset instead if it wasn't widechar.
					}
					printf("%ws", &InputBuffer[CurPos]); CurPos += ke.wRepeatCount; InputSz += ke.wRepeatCount;
					if (CurPos < InputSz - 1) {// Set cursor position to where it was after printing.
						csbi.dwCursorPosition.X++; SetConsoleCursorPosition(outHnd, csbi.dwCursorPosition);
					}
				}
			}

		}
		//this->ConMtx.unlock();
	PollSocket:
		if (WSAPoll(&pf, 1, 10)) {
			if (pf.revents & POLLIN) {
				int received = recv(pf.fd, RecvBuffer, 2048, 0); RecvBuffer[received] = '\0';
				printf("\r[Server] sent data (%d bytes):\n%s\n%ws", received, RecvBuffer, InputBuffer); 
			}
			else if (pf.revents & POLLHUP) {
				std::wcout << "[Server] disconnected. Ending session...\n"; return 0;
			}
		}
	}
}
