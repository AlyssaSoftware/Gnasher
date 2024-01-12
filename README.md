# Gnasher Network Development Utility

Gnasher is network development utility that I plan to use myself to aid me develop my networking applications. 
Currently implements a transparent TCP proxy and a TCP client.

## Wanted for release (will be populated more, order may differ)
- [ ] Linux support
- [x] SSL support
	- [x] SSL client
	- [x] SSL transparent proxy (with self-trust probably)
	- [ ] SSL on built-in servers.
- [x] Add some basic server implementations
	- [x] Echo server
	- [x] Loop server (sends something every n seconds)
	- [ ] Do polishing on server implementations.
- [x] Improved console support
	- [ ] *More improved* console support
	- [ ] History support on client.
	- [ ] Port client console to \*nix.
- [ ] Concurrent proxy (multiple clients to single server)
- [ ] UDP support (maybe not, I never used UDP)
- [ ] Replace placeholders with actual implementations
- [ ] Abiity to save packets to files
- [ ] Stress testing
- [ ] Last step, make sure it's ready to production.