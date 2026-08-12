#include "../../includes/Server.hpp"

void Server::_removeClientFromChannel(Channel &channel, int fd, const std::string &quitMessage) {
	if (channel.isMember(fd) && !quitMessage.empty())
		_broadcastToChannel(channel, quitMessage, fd);
	channel.removeClient(fd);
}

void Server::_removeClient(int fd) {
	std::map<int, Client>::iterator client = _clients.find(fd);
	std::map<std::string, Channel>::iterator channel;
	std::string quitMessage;

	if (client == _clients.end())
		return;
	if (client->second.getAuth())
		quitMessage = _clientPrefix(fd) + " QUIT :Client disconnected\r\n";
	channel = _channels.begin();
	while (channel != _channels.end()) {
		_removeClientFromChannel(channel->second, fd, quitMessage);
		if (channel->second.getMemberCount() == 0)
			_channels.erase(channel++);
		else
			++channel;
	}
	_clients.erase(client);
	std::cout << "Client disconnected: fd " << fd << std::endl;
}
