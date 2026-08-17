#include "../../includes/Server.hpp"	

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

void	Server::_broadcastChannelCommand(int fd, const Channel &channel, const std::string &command, const std::string &params, const std::string &trailing, int exceptFd){
	std::stringstream ss;

	ss << _clientPrefix(fd)
		<< " " << command
		<< " " << channel.getName();
	if (!params.empty())
		ss << " " << params;
	if (!trailing.empty())
		ss << " :" << trailing;
	ss << "\r\n";
	_broadcastToChannel(channel, ss.str(), exceptFd);
}
