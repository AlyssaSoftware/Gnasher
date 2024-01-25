#include "Gnasher.h"

char GPLLicense[] =
"Copyright (C) 2024 Alyssa Software\n\n"
"This program is free software : you can redistribute it and /or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.If not, see <https://www.gnu.org/licenses/>.\n";

char UsageString[] =
"Gnasher Network Development Utility basic usage\n"
"> gnasher [mode] [parameter] (-flags...)\n"
"List of avaliable modes and it's parameters:\n\n"

"Note: each of these commands can be prefixed with 's' for enabling SSL mode.\n"
"p(roxy)  [target]		: Launches a TCP transparent proxy to target.\n"
"c(lient) [target]		: TCP console client that connects to specified target.\n"
"e(choserver)			: Launches an echo server that echoes back payloads that clients send.\n"
"l(oopserver)			: Launches a server that sends a data every n seconds.\n"
"r(everseclient)		: similar to client but you're a server instead of a client.\n"
"g(n2txt) [src.] [dest.]: Converts a Gnasher capture to human-readable TXT file.\n"
"v(ersion)				: Prints version and license info.\n"
"h(elp)					: Prints this page.\n\n"

"Additional flags:\n\n"

"-f [path]		: File to save transferred packets.\n"
"-sslcert [path]: SSL certificate to use.\n"
"-sslkey  [path]: SSL private key to use."
"-p [port]		: Port to listen on, default is 22222.\n\n"

"Examples:\n\n"

"> gnasher.exe client 172.217.169.174:80\n"
"> gnasher.exe p 12.34.56.78:443 -f asd.gncap\n"
"> gnasher.exe src -p 27031 -sslcert crt.pem -sslkey key.key -f patest.gncap\n"
"> gnasher.exe e\n"
"> gnasher.exe echoserver -p 25565\n"
"> gnasher.exe gn2txt asd.gncap asd.txt\n";
