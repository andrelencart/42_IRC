#include "../../includes/Channel.hpp"

Channel::Channel() : _name("#default"){};

Channel::Channel(std::string channel) : _name(channel){};

Channel::Channel(const Channel &other)
{
	*this = other;
};

Channel& Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
	    return (*this);
	}
	return (*this);
};

void _sendMsg2(int fd, std::string msg) {
	send(fd, msg.c_str(), msg.size(), 0);
}

bool _handleJoin(int fd, std::string line){
	std::istringstream iss(line);
	std::string channel;
	std::string pass;
	std::string check_no;
	std::map<std::string, std::string> channels;

	iss >> check_no;
	iss >> channel;
	iss >> pass;
	iss >> check_no;
	if(check_no != "JOIN"){
		 _sendMsg2(fd, ERR_TOOMANYTARGETS("JOIN"));
		 return false;
	}
	std::cout << "check_no: " << check_no << std::endl;	
	std::cout << "channel: " << channel << std::endl;
	std::cout << "pass: " << pass << std::endl;
    if (channel.empty()){
		 _sendMsg2(fd, ERR_NEEDMOREPARAMS("JOIN"));
		 return false;
	}
    if (channel[0] != '&' && channel[0] != '#'){
		 _sendMsg2(fd, ERR_BADCHANMASK("JOIN"));
		 return false;
	}
    if(channel.find(7) != std::string::npos){
		 _sendMsg2(fd, ERR_NOSUCHCHANNEL("JOIN"));
		 return false;
	}
	return true;
}
