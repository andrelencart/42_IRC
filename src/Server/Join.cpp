#include "../../includes/Server.hpp"

std::map<std::string, std::string> Server::_buildChannelMap(std::string channel,
	std::string pass, int fd, int *check) {
	size_t pos = 0;
	size_t pos2 = 0;
	std::string temp;
	std::map<std::string, std::string> channels;

	while ((pos = channel.find(',')) != std::string::npos) {
		if ((pos2 = pass.find(',')) != std::string::npos) {
			if (pos2 == 0 && pass[0] == ',')
				temp = "";
			else
				temp = pass.substr(0, pos2);
			pass = pass.substr(pos2 + 1, pass.size());
		}
		else if (!pass.empty()) {
			if (pos2 == std::string::npos)
				temp = pass.substr(0, pass.size());
			else
				temp = pass.substr(0, pos2);
			pass = "";
		}
		else
			temp = "";
		std::cout << "pass    " << pass << std::endl;
		std::cout << "channel    " << channel << std::endl;
		channels.insert(std::pair<std::string, std::string>(channel.substr(0, pos), temp));
		channel = channel.substr(pos + 1, channel.size());
		if (!channel.empty() && channel[0] == ',') {
			_sendNumericReply(fd, ERR_BADCHANMASK(channel));
			*check = 1;
		}
		pos = 0;
		pos2 = 0;
		temp = "";
	}
	if (!pass.empty()) {
		pos2 = pass.find(',');
		if (pos2 == std::string::npos) {
			temp = pass.substr(0, pass.size());
			pass = "";
		}
		else {
			temp = pass.substr(0, pos2);
			pass = pass.substr(pos2 + 1, pass.size());
		}
	}
	channels.insert(std::pair<std::string, std::string>(channel.substr(0, pos), temp));
	return channels;
}

bool Server::_parseChannel(std::map<std::string, std::string>::const_iterator it,
	int fd) {
	const std::string &name = it->first;

	if (name.empty() || (name[0] != '&' && name[0] != '#')) {
		_sendNumericReply(fd, ERR_BADCHANMASK(name));
		return false;
	}
	if (name.size() > 200) {
		_sendNumericReply(fd, ERR_BADCHANMASK(name));
		return false;
	}
	for (std::string::size_type i = 0; i < name.size(); i++) {
		unsigned char character = static_cast<unsigned char>(name[i]);

		if (name[i] == ',' || name[i] == ' ' || std::iscntrl(character)) {
			_sendNumericReply(fd, ERR_BADCHANMASK(name));
			return false;
		}
	}
	return true;
}

std::string Server::_buildNamesList(const Channel &channel)
{
	std::stringstream ss;
	const std::set<int> &members = channel.getMembers();

	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); it++) {
		if (it != members.begin() && _clients[*it].getNickname() != "")
			ss << " ";
		if (channel.isOperator(*it))
			ss << "@";
		ss << _clients[*it].getNickname();
	}
	return ss.str();
}

void Server::_sendJoinReplies(int fd, Channel &channel)
{
	if (!channel.getTopic().empty())
		_sendNumericReply(fd, RPL_TOPIC(channel.getName(), channel.getTopic()));
	_sendNumericReply(fd, RPL_NAMREPLY("=", channel.getName(),
		_buildNamesList(channel)));
	_sendNumericReply(fd, RPL_ENDOFNAMES(channel.getName()));
}

bool Server::buildChan(std::map<std::string, std::string>::const_iterator channels,
	int fd) {
	std::string channelKey = _channelKey(channels->first);
	std::map<std::string, Channel>::iterator it = _channels.find(channelKey);

	if (it == _channels.end()) {
		Channel newChan(channels->first);
		newChan.addMember(fd);
		newChan.addOperator(fd);
		_channels.insert(std::pair<std::string, Channel>(channelKey, newChan));
		it = _channels.find(channelKey);
		_broadcastChannelCommand(fd, it->second, "JOIN", "", "", false);
		_sendJoinReplies(fd, it->second);
		return true;
	}
	if (it->second.isMember(fd))
		return true;
	if (it->second.isFull()) {
		_sendNumericReply(fd, ERR_CHANNELISFULL(it->second.getName()));
		return false;
	}
	if (it->second.isInviteOnly() && !it->second.isInvited(fd)) {
		_sendNumericReply(fd, ERR_INVITEONLYCHAN(it->second.getName()));
		return false;
	}
	if (it->second.hasPass() && it->second.getPass() != channels->second) {
		_sendNumericReply(fd, ERR_BADCHANNELKEY(it->second.getName()));
		return false;
	}
	it->second.addMember(fd);
	it->second.removeInvite(fd);
	_broadcastChannelCommand(fd, it->second, "JOIN", "", "", false);
	_sendJoinReplies(fd, it->second);
	return true;
}

bool Server::_handleJoin(int fd, const Command &command)
{
	std::string channel;
	std::string pass;
	std::map<std::string, std::string> channels;
	int check = 0;

	if (command.params.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("JOIN"));
		return false;
	}
	channel = command.params[0];
	if (command.params.size() > 1)
		pass = command.params[1];
	channels = _buildChannelMap(channel, pass, fd, &check);
	if (check)
		return false;
	std::map<std::string, std::string>::const_iterator it;
	for (it = channels.begin(); it != channels.end(); it++) {
		if (!_parseChannel(it, fd))
			return false;
	}
	bool ret = true;
	for (it = channels.begin(); it != channels.end(); it++) {
		if (!buildChan(it, fd))
			ret = false;
	}
	return ret;
}
