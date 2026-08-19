#include "../../includes/Server.hpp"

std::set<std::string> Server::_buildUserMap(std::string username, int fd, int *check) {
	size_t pos;
	std::set<std::string> users;

	while (true) {
		pos = username.find(',');
		std::string target = username.substr(0, pos);
		if (target.empty()) {
			if (*check == 0)
				_sendNumericReply(fd, ERR_NORECIPIENT("PRIVMSG"));
			*check = 1;
		}
		else
			users.insert(target);
		if (pos == std::string::npos)
			break;
		username = username.substr(pos + 1);
	}
	return users;
}

bool Server::_parseMsgRequest(int fd, const Command &command,
	std::set<std::string> &targets, std::string &message, int &check) {
	if (command.params.empty()) {
		_sendNumericReply(fd, ERR_NORECIPIENT("PRIVMSG"));
		return false;
	}
	if (command.hasTrailing) {
		if (command.params.size() != 1) {
			_sendNumericReply(fd, ERR_NEEDMOREPARAMS("PRIVMSG"));
			return false;
		}
		message = command.trailing;
	}
	else if (command.params.size() == 1) {
		_sendNumericReply(fd, ERR_NOTEXTTOSEND());
		return false;
	}
	else if (command.params.size() != 2) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("PRIVMSG"));
		return false;
	}
	else
		message = command.params[1];
	if (message.empty()) {
		_sendNumericReply(fd, ERR_NOTEXTTOSEND());
		return false;
	}
	targets = _buildUserMap(command.params[0], fd, &check);
	return true;
}

bool Server::_executeMsgRequest(int fd,
	const std::set<std::string> &targets, const std::string &message,
	int check) {
	std::set<std::string>::const_iterator it;
	int user;
	bool delivered = false;

	for (it = targets.begin(); it != targets.end(); it++) {
		if ((*it)[0] == '#' || (*it)[0] == '&') {
			Channel *channel = _getChannel(*it);

			if (channel == NULL) {
				_sendNumericReply(fd, ERR_NOSUCHCHANNEL(*it));
				check = 1;
				continue;
			}
			if (!channel->isMember(fd)) {
				_sendNumericReply(fd, ERR_CANNOTSENDTOCHAN(*it));
				check = 1;
				continue;
			}
			_broadcastChannelCommand(fd, *channel, "PRIVMSG", "", message,
				true, fd);
			delivered = true;
			continue;
		}
		user = _findClientFdByNick(*it);
		if (user == -1) {
			_sendNumericReply(fd, ERR_NOSUCHNICK(*it));
			check = 1;
			continue;
		}
		std::stringstream ss;
		ss << _clientPrefix(fd) << " PRIVMSG " << *it
			<< " :" << message << "\r\n";
		_sendMsg(user, ss.str());
		delivered = true;
	}
	return check == 0 && delivered;
}

bool Server::_handleMsg(int fd, const Command &command) {
	std::set<std::string> targets;
	std::string message;
	int check = 0;

	if (!_parseMsgRequest(fd, command, targets, message, check))
		return false;
	return _executeMsgRequest(fd, targets, message, check);
}
