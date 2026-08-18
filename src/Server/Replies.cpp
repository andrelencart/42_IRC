#include "../../includes/Server.hpp"	

void Server::_sendNumericReply(int fd, const std::string &numeric,
	const std::string &parameters, const std::string &description,
	bool hasDescription) {
	std::string nickname = _clients[fd].getNickname();
	std::stringstream reply;

	if (nickname.empty())
		nickname = "*";
	reply << ":" << _serverName << " " << numeric << " " << nickname;
	if (!parameters.empty())
		reply << " " << parameters;
	if (hasDescription)
		reply << " :" << description;
	reply << "\r\n";
	_sendMsg(fd, reply.str());
}

std::string	Server::_clientPrefix(int fd){
	std::stringstream ss;

	ss << ":" << _clients[fd].getNickname()
		<< "!" << _clients[fd].getUsername()
		<< "@localhost";
	return ss.str();
}

void	Server::_broadcastToChannel(const Channel &channel, const std::string &msg, int exceptFd){
	const std::set<int> &members = channel.getMembers();
	std::set<int>::const_iterator it;

	for (it = members.begin(); it != members.end(); it++){
		if (*it != exceptFd)
			_sendMsg(*it, msg);
	}
}

void Server::_broadcastChannelCommand(int fd, const Channel &channel,
	const std::string &command, const std::string &params,
	const std::string &trailing, bool hasTrailing, int exceptFd) {
	std::stringstream ss;

	ss << _clientPrefix(fd)
		<< " " << command
		<< " " << channel.getName();
	if (!params.empty())
		ss << " " << params;
	if (hasTrailing)
		ss << " :" << trailing;
	ss << "\r\n";
	_broadcastToChannel(channel, ss.str(), exceptFd);
}
