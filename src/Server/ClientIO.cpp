#include "../../includes/Server.hpp"

void Server::_acceptNewClient() {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(_servFd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
	if (clientFd == -1) {
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK
			|| errno == ECONNABORTED)
			return;
		throw std::runtime_error("accept() failed!");
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
		close(clientFd);
		return;
	}
	Client newClient(clientFd);
	_clients[clientFd] = newClient;
	std::cout << "New client connected: fd " << newClient.getClientFD() << std::endl;

	struct pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
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
	else if (bytes < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return false;
		std::cerr << "recv() error on fd " << fd << ": "
			<< std::strerror(errno) << std::endl;
		_removeClient(fd);
		return true;
	}
	_clients[fd].appendReadBuffer(std::string(buffer, bytes));
	if (!_processBuffer(fd))
		return true;
	return false;
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
		// if (_clients[fd].getAuth())
		// 	std::cout << _clients[fd].getNickname() << ": " << line << std::endl;
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
