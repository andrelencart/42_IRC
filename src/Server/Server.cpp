#include "../../includes/Server.hpp"

volatile sig_atomic_t g_stop = 0;

void signalHandler(int sig){
	(void)sig;
	g_stop = 1;
}

Server::Server(): _port(0), _password(""), _servFd(-1) {}

Server::Server(int port, std::string password): _port(port), _password(password), _servFd(-1) {}

Server::~Server() {
	for (size_t i = 1; i < _fds.size(); i++)
		close(_fds[i].fd);
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

void Server::_acceptNewClient() {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(_servFd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
	if (clientFd == -1)
		throw std::runtime_error("accept() failed!");
	
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	std::cout << "New client connected: fd " << clientFd << std::endl;

	struct pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN; // This Flag means this "wake me up when this fd has data ready to read"
	clientPollFd.revents = 0;
	_fds.push_back(clientPollFd);
}

bool Server::_processCommand(int fd, std::string line) {
	std::istringstream iss(line);
	std::string command;
	iss >> command;

	if (command != "PASS" && !_passverified.count(fd)){
		_sendMsg(fd, ":server 451 * :You have not registered\r\n");
		return true;
	}

	if (command == "PASS"){
		if (!_handlePass(fd, iss))
			return false;
	}
	else if (command == "NICK"){
		_handleNick(fd, iss);
		
	}
	else if (command == "USER"){
		_handleUser(fd, iss);
		
	}
	if (_passverified.count(fd) && _nicknames.count(fd) && _usernames.count(fd))
		_authenticated[fd] = true;
	return true;
}

bool Server::_processBuffer(int fd) {
	size_t pos;

	while ((pos = _clientBuffers[fd].find("\r\n")) != std::string::npos) {
		std::string line = _clientBuffers[fd].substr(0, pos);
		if (_authenticated[fd])
			std::cout << "Line: " << line << std::endl;
		_clientBuffers[fd].erase(0, pos + 2);
		if (!_processCommand(fd, line))
			return false;
	}
	return true;
}

void Server::_loopServer() {
	struct pollfd servPollFd;
	servPollFd.fd = _servFd;
	servPollFd.events = POLLIN; // This Flag means this "wake me up when this fd has data ready to read"
	servPollFd.revents = 0;
	_fds.push_back(servPollFd);

	signal(SIGINT, signalHandler);
	while (!g_stop) {
		int connected = poll(_fds.data(), _fds.size(), -1);
		if(connected == -1){
			if (errno == EINTR)
				break;
			throw std::runtime_error("poll() failed!");
		}
		for(size_t i = 0; i < _fds.size(); i++){
			if (_fds[i].revents & POLLIN) { // if there is data to read in that fd
				if (i == 0){
					_acceptNewClient();
				}
				else {
					if (_handleClient(_fds[i].fd)){
						close(_fds[i].fd);
						_fds.erase(_fds.begin() + i);
						i--;
					}
				}
			}
		}
	}
}

void Server::start() {
	//signal(SIGPIPE, SIG_IGN); //registers a handler for the SIGPIPE signal and sets it to SIG_IGN (ignore).
	_setupSocket();
	std::cout << "Server is up on port " << _port << std::endl;
	_loopServer();
}