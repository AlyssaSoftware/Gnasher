# Gnasher Network Development Utility

## STILL WORK IN PROGRESS - EARLY RELEASE
Work is still not done, though it's pretty much working and usable. As it's not finished some things are missing and/or not implemented and can have bugs.

Gnasher is free and open source network development utility that I plan to use myself to aid me develop my networking applications. 
Currently implements a transparent TCP proxy and a TCP client.

Gnasher is free and open source software and is licensed with GPLv3.

## Wanted for release (will be populated more, order may differ)
- [ ] Linux support
- [x] SSL support
	- [x] SSL client
	- [x] SSL transparent proxy (with self-trust probably)
	- [x] SSL on built-in servers.
- [x] Add some basic server implementations
	- [x] Echo server
	- [x] Loop server (sends something every n seconds)
	- [x] Client code can run as a server now.
	- [x] ~~Do polishing on server implementations.~~ Likely not needed.
- [x] Improved console support
	- [x] *More improved* console support
	- [x] History support on client.
	- [ ] Port client console to \*nix.
	- [ ] Prefixes (maybe not)
	- [x] Sending a whole file.
	- [ ] Add abiity to clear the text and retaining the cursor position after print.
- [x] Concurrent proxy (multiple clients to single server)
- [ ] UDP support (maybe not, I never used UDP)
- [x] ~~Replace placeholders with actual implementations~~ (is there any?)
- [x] Abiity to save packets to files
	- [x] Converting to human readable TXT
	- [ ] Improvements for packets and convertions
	- [ ] Converting to pcap (probably not)
- [ ] Stress testing
- [x] Implement actual CLI 
- [ ] Last step, make sure it's ready to production.