/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmota-ma <rmota-ma@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 16:49:29 by dicosta-          #+#    #+#             */
/*   Updated: 2026/06/26 20:55:50 by rmota-ma         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"


volatile sig_atomic_t g_stop = 0;

void signalHandler(int sig){
	(void)sig;
	g_stop = 1;
}

Server::Server(): _port(0), _password(""), _servFd(-1) {
	_initCommandHandlers();
}

Server::Server(int port, std::string password, std::string serverName): _port(port), _password(password), _serverName(serverName), _servFd(-1) {
	_initCommandHandlers();
}

Server::~Server() {
	for (size_t i = 1; i < _fds.size(); i++)
		close(_fds[i].fd);
	if (_servFd != -1)
		close(_servFd);
	std::cout << "Server Shutdown!" << std::endl;
}

Channel* Server::_getChannel(std::string channelName) {
	std::map<std::string, Channel>::iterator it = _channels.find(channelName);

	if (it == _channels.end())
		return NULL;

	return &(it->second);
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
	Client newClient(clientFd);
	_clients[clientFd] = newClient;
	std::cout << "New client connected: fd " << newClient.getClientFD() << std::endl;
  
	struct pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN; // This Flag means this "wake me up when this fd has data ready to read"
	clientPollFd.revents = 0;
	_fds.push_back(clientPollFd);
}

bool Server::_handleClient(int fd) {
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes == 0) {
		_removeClient(fd);
		return true;
	}
	else if (bytes < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return false;
		std::cerr << "recv() error on fd " << fd << ": "
			<< std::strerror(errno) << std::endl;
		_removeClient(fd);
		return true;
	}
	else {
		_clients[fd].appendReadBuffer(std::string(buffer, bytes));
	//	std::cout << _clients[fd].getReadBuffer(); // ADICIONADA PARA TESTE
		if (!_processBuffer(fd))
			return true;
		return false;
	}
}

bool Server::_handleClientEvents(int fd, short revents) {
	bool shouldRemove = false;
	std::map<int, Client>::iterator client = _clients.find(fd);

	if (client == _clients.end())
		return true;
	if ((revents & POLLIN) && !client->second.getCloseAfterWrite())
		shouldRemove = _handleClient(fd);
	if (!shouldRemove && (revents & POLLOUT))
		shouldRemove = _flushClientOutput(fd);
	if (!shouldRemove && (revents & (POLLERR | POLLHUP | POLLNVAL))) {
		_removeClient(fd);
		shouldRemove = true;
	}
	return shouldRemove;
}

void Server::_setWritePolling(int fd, bool enabled) {
	for (size_t i = 0; i < _fds.size(); i++) {
		if (_fds[i].fd != fd)
			continue;
		if (enabled)
			_fds[i].events |= POLLOUT;
		else
			_fds[i].events &= ~POLLOUT;
		return;
	}
}

void Server::_sendMsg(int fd, const std::string &message) {
	std::map<int, Client>::iterator client = _clients.find(fd);

	if (client == _clients.end())
		return;
	client->second.appendWriteBuffer(message);
	_setWritePolling(fd, true);
}

bool Server::_flushClientOutput(int fd) {
	std::map<int, Client>::iterator client = _clients.find(fd);

	if (client == _clients.end())
		return true;
	const std::string &pending = client->second.getWriteBuffer();
	if (pending.empty()) {
		_setWritePolling(fd, false);
		if (client->second.getCloseAfterWrite()) {
			_removeClient(fd);
			return true;
		}
		return false;
	}
	ssize_t sent = send(fd, pending.c_str(), pending.size(), 0);
	if (sent > 0) {
		client->second.eraseWriteBuffer(static_cast<size_t>(sent));
		if (client->second.getWriteBuffer().empty()) {
			_setWritePolling(fd, false);
			if (client->second.getCloseAfterWrite()) {
				_removeClient(fd);
				return true;
			}
		}
		return false;
	}
	if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		return false;
	if (sent < 0)
		std::cerr << "send() error on fd " << fd << ": "
			<< std::strerror(errno) << std::endl;
	_removeClient(fd);
	return true;
}

bool Server::_processBuffer(int fd) {
	size_t pos;
	std::string buffer;

	while (true) {
		buffer = _clients[fd].getReadBuffer();
		pos = buffer.find("\r\n");
		if (pos == std::string::npos)
			break;
		if (pos > 510) {
			_removeClient(fd);
			return false;
		}
		std::string line = buffer.substr(0, pos);
		if (_clients[fd].getAuth())
			std::cout << _clients[fd].getNickname() << ": " << line << std::endl;
		_clients[fd].eraseBuffer(pos);
		if (!_processCommand(fd, line))
			return false;
		if (_clients[fd].getCloseAfterWrite())
			return true;
	}
	buffer = _clients[fd].getReadBuffer();
	if (buffer.size() > 511
		|| (buffer.size() == 511 && buffer[buffer.size() - 1] != '\r')) {
		_removeClient(fd);
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
			if (i == 0) {
				if (_fds[i].revents & POLLIN)
					_acceptNewClient();
				continue;
			}
			int clientFd = _fds[i].fd;
			short revents = _fds[i].revents;
			if (_handleClientEvents(clientFd, revents)) {
				close(clientFd);
				_fds.erase(_fds.begin() + i);
				i--;
			}
		}
	}
}

void Server::start() {
	signal(SIGPIPE, SIG_IGN);
	_setupSocket();
	std::cout << "Server is up on port " << _port << std::endl;
	_loopServer();
}
