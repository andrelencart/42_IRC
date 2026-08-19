#include "../../includes/Server.hpp"

void Server::_removeClientFromChannels(int fd, const std::string &reason,
	bool notifyPeers) {
	std::map<int, Client>::iterator client = _clients.find(fd);
	std::map<std::string, Channel>::iterator channel;
	std::set<int> recipients;

	if (client == _clients.end())
		return;
	channel = _channels.begin();
	while (channel != _channels.end()) {
		if (channel->second.isMember(fd)) {
			if (notifyPeers && client->second.getAuth()) {
				const std::set<int> &members = channel->second.getMembers();

				for (std::set<int>::const_iterator member = members.begin();
					member != members.end(); member++) {
					if (*member != fd)
						recipients.insert(*member);
				}
			}
			channel->second.removeClient(fd);
		}
		if (channel->second.getMemberCount() == 0)
			_channels.erase(channel++);
		else
			channel++;
	}
	if (!notifyPeers || !client->second.getAuth())
		return;
	const std::string quitMessage = _clientPrefix(fd)
		+ " QUIT :" + reason + "\r\n";
	for (std::set<int>::const_iterator recipient = recipients.begin();
		recipient != recipients.end(); recipient++)
		_sendMsg(*recipient, quitMessage);
}

bool Server::_handleQuit(int fd, const Command &command) {
	std::string reason = "Client Quit";
	std::string closingReason;

	if (command.hasTrailing)
		reason = command.trailing;
	else if (!command.params.empty())
		reason = command.params[0];
	_removeClientFromChannels(fd, reason, true);
	closingReason = reason;
	if (closingReason.empty())
		closingReason = "Client Quit";
	_sendMsg(fd, "ERROR :Closing Link: " + closingReason + "\r\n");
	_clients[fd].setCloseAfterWrite(true);
	return true;
}

void Server::_removeClient(int fd) {
	std::map<int, Client>::iterator client = _clients.find(fd);

	if (client == _clients.end())
		return;
	_removeClientFromChannels(fd, "Client disconnected", true);
	_clients.erase(client);
	std::cout << "Client disconnected: fd " << fd << std::endl;
}
