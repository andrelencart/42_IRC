#include "../../includes/Server.hpp"

Server::Server(): _port(0), _password(""), _servFd(-1) {}

Server::Server(int port, std::string password): _port(port), _password(password), _servFd(-1) {}

Server::~Server() {
	if (_servFd != -1)
		close(_servFd);
	std::cout << "Server Shutdown!" << std::endl;
}

void Server::_setupSocket() {
	_servFd = socket(AF_INET, SOCK_STREAM, 0); //Creates a TCP socket. AF_INET = IPv4, SOCK_STREAM = TCP (reliable, ordered). Returns a file descriptor (_servFd)
	if (_servFd == -1)
		throw std::runtime_error("socket() failed!");
	
	int opt = 1;
	if (setsockopt(_servFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //Tells the OS to allow reusing the port immediately after the server stops. Without this, if you restart the server quickly you get "address already in use" for ~60 seconds
		throw std::runtime_error("setsockopt() failed!");
	fcntl(_servFd, F_SETFL, O_NONBLOCK); //Sets the listening socket to non-blocking mode. This means accept() won't freeze the server if called when no client is waiting — it just returns -1 with EWOULDBLOCK instead

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET; // IPv4
	addr.sin_addr.s_addr = INADDR_ANY; // accept connections on any network interface
	addr.sin_port = htons(_port); // the port in network byte order (big-endian)

	if (bind(_servFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) //Attaches the socket to that address/port. After this, the OS knows "this socket owns port X".
		throw std::runtime_error("bind() failed!");
	
	if (listen(_servFd, SOMAXCONN) == -1) // Tells the OS to start accepting incoming connection requests on that socket. SOMAXCONN is the max queue of pending connections waiting to be accept()ed
		throw std::runtime_error("listen() failed!");
}

void Server::_loopServer() {
	struct pollfd servPollFd;
	servPollFd.fd = _servFd;
	servPollFd.events = POLLIN; // This Flag means this "wake me up when this fd has data ready to read"
	servPollFd.revents = 0;
	_fds.push_back(servPollFd);

}