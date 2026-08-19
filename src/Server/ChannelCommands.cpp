#include "../../includes/Server.hpp"

bool Server::_kickFromChannel(int fd, Channel &channel,
	const std::string &targetNickname, const std::string &comment) {
	int targetFd;
	std::stringstream reply;

	if (!channel.isMember(fd)) {
		_sendNumericReply(fd, ERR_NOTONCHANNEL(channel.getName()));
		return false;
	}
	if (!channel.isOperator(fd)) {
		_sendNumericReply(fd, ERR_CHANOPRIVSNEEDED(channel.getName()));
		return false;
	}
	targetFd = _findClientFdByNick(targetNickname);
	if (targetFd == -1) {
		_sendNumericReply(fd, ERR_NOSUCHNICK(targetNickname));
		return false;
	}
	if (!channel.isMember(targetFd)) {
		_sendNumericReply(fd,
			ERR_USERNOTINCHANNEL(targetNickname, channel.getName()));
		return false;
	}
	reply << _clientPrefix(fd) << " KICK " << channel.getName()
		<< " " << targetNickname << " :" << comment << "\r\n";
	_broadcastToChannel(channel, reply.str());
	channel.removeClient(targetFd);
	return true;
}

bool Server::_handleKick(int fd, const Command &command) {
	std::string channelName;
	std::string targetNickname;
	std::string comment;
	std::map<std::string, Channel>::iterator channel;

	if (command.params.size() < 2) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("KICK"));
		return false;
	}
	channelName = command.params[0];
	targetNickname = command.params[1];
	comment = _clients[fd].getNickname();
	if (command.hasTrailing && command.params.size() != 2) {
		_sendNumericReply(fd, ERR_MALFORMEDTEXT("KICK"));
		return false;
	}
	if (!command.hasTrailing && command.params.size() > 3) {
		_sendNumericReply(fd, ERR_MALFORMEDTEXT("KICK"));
		return false;
	}
	if (command.hasTrailing)
		comment = command.trailing;
	else if (command.params.size() > 2)
		comment = command.params[2];
	channel = _channels.find(_channelKey(channelName));
	if (channel == _channels.end()) {
		_sendNumericReply(fd, ERR_NOSUCHCHANNEL(channelName));
		return false;
	}
	if (!_kickFromChannel(fd, channel->second, targetNickname, comment))
		return false;
	if (channel->second.getMemberCount() == 0)
		_channels.erase(channel);
	return true;
}

bool Server::_partChannel(int fd, const std::string &channelName,
	const std::string &partMessage) {
	std::map<std::string, Channel>::iterator channel;
	std::stringstream reply;

	channel = _channels.find(_channelKey(channelName));
	if (channel == _channels.end()) {
		_sendNumericReply(fd, ERR_NOSUCHCHANNEL(channelName));
		return false;
	}
	if (!channel->second.isMember(fd)) {
		_sendNumericReply(fd, ERR_NOTONCHANNEL(channel->second.getName()));
		return false;
	}
	reply << _clientPrefix(fd) << " PART " << channel->second.getName()
		<< " :" << partMessage << "\r\n";
	_broadcastToChannel(channel->second, reply.str());
	channel->second.removeClient(fd);
	if (channel->second.getMemberCount() == 0)
		_channels.erase(channel);
	return true;
}

bool Server::_handlePart(int fd, const Command &command) {
	std::string channelList;
	std::string partMessage;
	std::string::size_type start;
	std::string::size_type end;

	if (command.params.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("PART"));
		return false;
	}
	channelList = command.params[0];
	partMessage = _clients[fd].getNickname();
	if (command.hasTrailing && command.params.size() != 1) {
		_sendNumericReply(fd, ERR_MALFORMEDTEXT("PART"));
		return false;
	}
	if (!command.hasTrailing && command.params.size() > 2) {
		_sendNumericReply(fd, ERR_MALFORMEDTEXT("PART"));
		return false;
	}
	if (command.hasTrailing)
		partMessage = command.trailing;
	else if (command.params.size() > 1)
		partMessage = command.params[1];
	start = 0;
	while (start <= channelList.size()) {
		end = channelList.find(',', start);
		_partChannel(fd, channelList.substr(start, end - start), partMessage);
		if (end == std::string::npos)
			break;
		start = end + 1;
	}
	return true;
}

bool Server::_handleInvite(int fd, const Command &command) {
	std::string targetNickname;
	std::string channelName;
	std::map<std::string, Channel>::iterator channel;
	std::stringstream notification;
	int targetFd;

	if (command.params.size() < 2) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("INVITE"));
		return false;
	}
	if (command.hasTrailing || command.params.size() > 2) {
		_sendNumericReply(fd, ERR_TOOMANYPARAMS("INVITE"));
		return false;
	}
	targetNickname = command.params[0];
	channelName = command.params[1];
	channel = _channels.find(_channelKey(channelName));
	if (channel == _channels.end()) {
		_sendNumericReply(fd, ERR_NOSUCHCHANNEL(channelName));
		return false;
	}
	targetFd = _findClientFdByNick(targetNickname);
	if (targetFd == -1) {
		_sendNumericReply(fd, ERR_NOSUCHNICK(targetNickname));
		return false;
	}
	if (!channel->second.isMember(fd)) {
		_sendNumericReply(fd, ERR_NOTONCHANNEL(channelName));
		return false;
	}
	if (channel->second.isInviteOnly() && !channel->second.isOperator(fd)) {
		_sendNumericReply(fd, ERR_CHANOPRIVSNEEDED(channelName));
		return false;
	}
	if (channel->second.isMember(targetFd)) {
		_sendNumericReply(fd, ERR_USERONCHANNEL(targetNickname, channelName));
		return false;
	}
	channel->second.invite(targetFd);
	_sendNumericReply(fd,
		RPL_INVITING(targetNickname, channel->second.getName()));
	notification << _clientPrefix(fd) << " INVITE " << targetNickname
		<< " :" << channel->second.getName() << "\r\n";
	_sendMsg(targetFd, notification.str());
	return true;
}

bool Server::_handleTopic(int fd, const Command &command) {
	if (command.params.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("TOPIC"));
		return false;
	}

	Channel *channel = _getChannel(command.params[0]);
	if (channel == NULL) {
		_sendNumericReply(fd, ERR_NOSUCHCHANNEL(command.params[0]));
		return false;
	}
	if (!channel->isMember(fd)) {
		_sendNumericReply(fd, ERR_NOTONCHANNEL(command.params[0]));
		return false;
	}
	if (command.hasTrailing && command.params.size() != 1) {
		_sendNumericReply(fd, ERR_MALFORMEDTEXT("TOPIC"));
		return false;
	}
	if (!command.hasTrailing && command.params.size() > 2) {
		_sendNumericReply(fd, ERR_MALFORMEDTEXT("TOPIC"));
		return false;
	}

	bool hasTopic = command.hasTrailing || command.params.size() == 2;

	if (!hasTopic) {
		if (channel->getTopic().empty())
			_sendNumericReply(fd, RPL_NOTOPIC(channel->getName()));
		else
			_sendNumericReply(fd,
				RPL_TOPIC(channel->getName(), channel->getTopic()));
		return true;
	}
	if (channel->isTopicRestricted() && !channel->isOperator(fd)) {
		_sendNumericReply(fd, ERR_CHANOPRIVSNEEDED(channel->getName()));
		return false;
	}

	std::string topic;
	if (command.hasTrailing)
		topic = command.trailing;
	else
		topic = command.params[1];
	channel->setTopic(topic);
	_broadcastChannelCommand(fd, *channel, "TOPIC", "", topic, true);

	return true;
}
