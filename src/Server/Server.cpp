#include "../../includes/Server.hpp"

Server::Server(): _port(0), _password(""), _servFd(-1) {}

Server::Server(int port, std::string password): _port(port), _password(password), _servFd(-1) {}

Server::~Server() {
	if (_servFd != -1)
		close(_servFd);
	std::cout << "Server Shutdown!" << std::endl;
}