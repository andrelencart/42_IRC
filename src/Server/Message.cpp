#include "../../includes/Server.hpp"

bool Server::_handleMsg(int fd, const Command &command) {
	std::string username;
	std::string msg;
	int user;

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
	user = _findClientFdByNick(username);
	if (user == -1) {
		_sendNumericReply(fd, ERR_NOSUCHNICK(username));
		return false;
	}
	std::stringstream ss;
	ss << _clientPrefix(fd) << " PRIVMSG " << username << " :" << msg << "\r\n";
	_sendMsg(user, ss.str());
	return true;
}
