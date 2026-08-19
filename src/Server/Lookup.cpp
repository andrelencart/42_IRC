#include "../../includes/Server.hpp"

std::string Server::_channelKey(const std::string &channelName) const {
	std::string key = channelName;

	for (std::string::size_type i = 0; i < key.size(); i++) {
		if (key[i] >= 'A' && key[i] <= 'Z')
			key[i] = static_cast<char>(key[i] - 'A' + 'a');
	}
	return key;
}

Channel *Server::_getChannel(std::string channelName) {
	std::map<std::string, Channel>::iterator it =
		_channels.find(_channelKey(channelName));

	if (it == _channels.end())
		return NULL;
	return &(it->second);
}

bool Server::_nickInUse(std::string nick, int currentFd) const {
	std::map<int, Client>::const_iterator it;

	for (it = _clients.begin(); it != _clients.end(); it++) {
		if (it->first != currentFd && it->second.getNickname() == nick)
			return true;
	}
	return false;
}

int Server::_findClientFdByNick(std::string nick) const {
	std::map<int, Client>::const_iterator client = _clients.begin();

	while (client != _clients.end()) {
		if (client->second.getNickname() == nick)
			return client->first;
		client++;
	}
	return -1;
}
