#include "../../includes/Server.hpp"

void Server::_sendMsg(int fd, std::string msg) {
	send(fd, msg.c_str(), msg.size(), 0);
}

void	Server::broadcastToChannel(std::string chanName, std::string msg, int fd){
	std::map<std::string, Channel>::iterator finder;
	std::stringstream ss;
	finder = _channels.find(chanName);
	if(finder == _channels.end())
		return ;
	const std::set<int> &members = finder->second.getMembers();
	std::set<int>::const_iterator it;
	ss << finder->second.getName() << ", "<<_clients[fd].getNickname() << ": " << msg << std::endl;
	for (it = members.begin(); it != members.end(); it++){
		_sendMsg(*it, ss.str());
	}
}
