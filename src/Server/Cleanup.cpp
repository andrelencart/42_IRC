#include "../../includes/Server.hpp"

void Server::_removeClient(int fd) {
	_clients.erase(fd);
	std::cout << "Client disconnected: fd " << fd << std::endl;

}
