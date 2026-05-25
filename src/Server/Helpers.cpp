#include "../../includes/Server.hpp"

void Server::_removeClient(int fd) {
	_clientBuffers.erase(fd);
	_authenticated.erase(fd);
	_nicknames.erase(fd);
	_usernames.erase(fd);
	std::cout << "Client disconnected: fd " << fd << std::endl;

}

void Server::_sendMsg(int fd, std::string msg) {
	send(fd, msg.c_str(), msg.size(), 0);
}

