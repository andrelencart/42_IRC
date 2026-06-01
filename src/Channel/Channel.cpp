#include "../../includes/Channel.hpp"

Channel::Channel() : _name("#default") {};

Channel::Channel(std::string channel) : _name(channel) {};

Channel::Channel(const Channel &other)
{
	*this = other;
};

Channel &Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		return (*this);
	}
	return (*this);
};

void _sendMsg2(int fd, std::string msg)
{
	send(fd, msg.c_str(), msg.size(), 0);
}

bool _handleJoin(int fd, std::string line)
{
	std::istringstream iss(line);
	std::string channel;
	std::string pass;
	std::string check_no;
	std::map<std::string, std::string> channels;

	iss >> check_no;
	iss >> channel;
	iss >> pass;
	iss >> check_no;
	if (check_no != "JOIN")
	{
		_sendMsg2(fd, ERR_TOOMANYTARGETS("JOIN"));
		return false;
	}
	if (channel.empty())
	{
		_sendMsg2(fd, ERR_NEEDMOREPARAMS("JOIN"));
		return false;
	}
	size_t pos = 0;
	size_t pos2 = 0;
	std::string temp;
	while ((pos = channel.find(',')) != std::string::npos)
	{
		if ((pos2 = pass.find(',')) != std::string::npos)
		{
			if(pos2 == 0 && pass[0] == ',')
				temp = "";
			else
				temp = pass.substr(0, pos2);
			pass = pass.substr(pos2 + 1, pass.size());
		}
		else if (pass[0]){
			if(pos2 == std::string::npos)
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
		if (channel[0] == ',')
			return _sendMsg2(fd, ERR_BADCHANMASK("JOIN")), false;
		pos = 0;
		pos2 = 0;
		temp = "";
	}
	if (pass[0]){
		pos2 = pass.find(',');
		if(pos2 == std::string::npos)
			temp = pass.substr(0, pass.size());
		else
			temp = pass.substr(0, pos2);
		pass = "";
	}
	channels.insert(std::pair<std::string, std::string>(channel.substr(0, pos), temp));
	std::map<std::string, std::string>::const_iterator it;
    for (it = channels.begin(); it != channels.end(); it++) {
        std::cout << "Chave: " << it->first 
                  << " | Valor: " << it->second 
                  << std::endl;
    }
	std::cout << "check_no: " << check_no << std::endl;
	std::cout << "channel: " << channel << std::endl;
	std::cout << "pass: " << pass << std::endl;
	if (channel[0] != '&' && channel[0] != '#')
	{
		_sendMsg2(fd, ERR_BADCHANMASK("JOIN"));
		return false;
	}
	if (channel.find(7) != std::string::npos)
	{
		_sendMsg2(fd, ERR_NOSUCHCHANNEL("JOIN"));
		return false;
	}
	return true;
}
