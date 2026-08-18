#include "../../includes/Server.hpp"

std::set<std::string> Server::_buildUserMap(std::string username, int fd, int *check){
	size_t pos = 0;
	std::set<std::string> users;

	while ((pos = username.find(',')) != std::string::npos) {
		std::cout << "username    " << username << std::endl;
		users.insert(username.substr(0, pos));
		username = username.substr(pos + 1, username.size());
		if (!username.empty() && username[0] == ',') {
			_sendNumericReply(fd, ERR_NOSUCHNICK(username));
			*check = 1;
		}
		pos = 0;
	}
	users.insert(username.substr(0, pos));
	return users;
}

bool Server::_handleMsg(int fd, const Command &command) {
	std::string username;
	std::string msg;
	int user;
	int check = 0;

	if (command.params.empty()) {
		_sendNumericReply(fd, ERR_NORECIPIENT("PRIVMSG"));
		return false;
	}
	username = command.params[0];
	if (command.hasTrailing)
		msg = command.trailing;
	else if (command.params.size() > 1)
		msg = command.params[1];
	if (msg.empty()) {
		_sendNumericReply(fd, ERR_NOTEXTTOSEND());
		return false;
	}
	if (username[0] == '#' || username[0] == '&') {
		Channel *channel = _getChannel(username);
		if (channel == NULL) {
			_sendNumericReply(fd, ERR_NOSUCHCHANNEL(username));
			return false;
		}
		if (!channel->isMember(fd)) {
			_sendNumericReply(fd, ERR_CANNOTSENDTOCHAN(username));
			return false;
		}
		_broadcastChannelCommand(fd, *channel, "PRIVMSG", "", msg, true,
			fd);
		return true;
	}
	std::set<std::string> users = _buildUserMap(username, fd, &check);
	if (check)
		return false;
	std::set<std::string>::iterator it;
	for (it = users.begin(); it != users.end(); it++) {
		user = _findClientFdByNick(*it);
		if (user == -1) {
			_sendNumericReply(fd, ERR_NOSUCHNICK(username));
			return false;
		}
		std::stringstream ss;
		ss << _clientPrefix(fd) << " PRIVMSG " << username << " :" << msg << "\r\n";
		_sendMsg(user, ss.str());
	}
	return true;
}
