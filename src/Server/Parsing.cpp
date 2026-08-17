#include "../../includes/Server.hpp"
bool Server::_parseCommand(const std::string &line, Command &command) const {
	std::string::size_type pos = 0;
	std::string::size_type start;

	command.name.clear();
	command.params.clear();
	command.hasTrailing = false;
	command.trailing.clear();
	while (pos < line.size() && line[pos] == ' ')
		pos++;
	if (pos == line.size())
		return false;
	start = pos;
	while (pos < line.size() && line[pos] != ' ')
		pos++;
	command.name = line.substr(start, pos - start);
	for (std::string::size_type i = 0; i < command.name.size(); i++)
		command.name[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(command.name[i])));
	while (pos < line.size()) {
		while (pos < line.size() && line[pos] == ' ')
			pos++;
		if (pos == line.size())
			break;
		if (line[pos] == ':') {
			command.hasTrailing = true;
			command.trailing = line.substr(pos + 1);
			break;
		}
		start = pos;
		while (pos < line.size() && line[pos] != ' ')
			pos++;
		command.params.push_back(line.substr(start, pos - start));
	}
	return !command.name.empty();
}

bool Server::_parseModeChanges(int fd, const Command &command, std::vector<ModeChange> &changes) {
	const std::string &modeString = command.params[1];
	size_t parameterIndex = 2;
	char sign = '\0';
	bool errorSent = false;

	changes.clear();
	for (size_t i = 0; i < modeString.size(); i++) {
		if (modeString[i] == '+' || modeString[i] == '-') {
			sign = modeString[i];
			continue;
		}
		if (sign == '\0') {
			_sendMsg(fd, ERR_UMODEUNKNOWNFLAG());
			return false;
		}
		if (!_isValidChannelMode(modeString[i])) {
			_sendMsg(fd, ERR_UNKNOWNMODE(std::string(1, modeString[i])));
			errorSent = true;
			continue;
		}
		ModeChange change;
		change.sign = sign;
		change.mode = modeString[i];
		change.hasParameter = false;
		change.parameter.clear();
		bool needsParameter = (change.mode == 'o'
			|| (change.sign == '+' && change.mode == 'k')
			|| (change.sign == '+' && change.mode == 'l'));
		if (needsParameter) {
			if (parameterIndex >= command.params.size()) {
				_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
				errorSent = true;
				continue;
			}
			change.hasParameter = true;
			change.parameter = command.params[parameterIndex++];
		}
		changes.push_back(change);
	}
	if (changes.empty() && !errorSent)
		_sendMsg(fd, ERR_UMODEUNKNOWNFLAG());
	return !changes.empty();
}

std::map<std::string, std::string> Server::_buildChannelMap(std::string channel, std::string pass, int fd, int *check){
	size_t pos = 0;
	size_t pos2 = 0;
	std::string temp;
	std::map<std::string, std::string> channels;
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
		else if (!pass.empty()){
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
		if (!channel.empty() && channel[0] == ','){
			_sendMsg(fd, ERR_BADCHANMASK("JOIN"));
			*check = 1;
		}
		pos = 0;
		pos2 = 0;
		temp = "";
	}
	if (!pass.empty()){
		pos2 = pass.find(',');
		if(pos2 == std::string::npos){
			temp = pass.substr(0, pass.size());
			pass = "";
		}
		else{
			temp = pass.substr(0, pos2);
			pass = pass.substr(pos2 + 1, pass.size());
		}	
	}
	channels.insert(std::pair<std::string, std::string>(channel.substr(0, pos), temp));
	if (!pass.empty()){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("JOIN"));
		*check = 1;
	}
	return channels;
}

bool Server::_parseChannel(std::map<std::string, std::string>::const_iterator it, int fd){
	const std::string &name = it->first;

	if (name.empty() || (name[0] != '&' && name[0] != '#'))
	{
		_sendMsg(fd, ERR_BADCHANMASK(name));
		return false;
	}
	if (name.size() > 200)
	{
		_sendMsg(fd, ERR_BADCHANMASK(name));
		return false;
	}
	for (std::string::size_type i = 0; i < name.size(); i++)
	{
		unsigned char character = static_cast<unsigned char>(name[i]);

		if (name[i] == ',' || name[i] == ' ' || std::iscntrl(character))
		{
			_sendMsg(fd, ERR_BADCHANMASK(name));
			return false;
		}
	}
	return true;
}

std::string Server::_buildNamesList(const Channel &channel)
{
	std::stringstream ss;
	const std::set<int> &members = channel.getMembers();

	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); it++)
	{
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
	std::stringstream reply;

	if (!channel.getTopic().empty())
		_sendMsg(fd, RPL_TOPIC(_clients[fd].getNickname(), channel.getName(), channel.getTopic()));
	reply << ":" << _serverName
		<< " 353 " << _clients[fd].getNickname()
		<< " = " << channel.getName()
		<< " :" << _buildNamesList(channel)
		<< "\r\n";
	_sendMsg(fd, reply.str());
	reply.str("");
	reply.clear();
	reply << ":" << _serverName
		<< " 366 " << _clients[fd].getNickname()
		<< " " << channel.getName()
		<< " :End of /NAMES list.\r\n";
	_sendMsg(fd, reply.str());
}

bool Server::buildChan(std::map<std::string, std::string>::const_iterator channels, int fd){
	std::map<std::string, Channel>::iterator it = _channels.find(channels->first);
	if(it == _channels.end()){
		Channel newChan(channels->first);
		newChan.addMember(fd);
		newChan.addOperator(fd);
		_channels.insert(std::pair<std::string, Channel>(channels->first, newChan));
		it = _channels.find(channels->first);
		_broadcastChannelCommand(fd, it->second, "JOIN", "", "");
		_sendJoinReplies(fd, it->second);
		return true;
	}
	if(it->second.isMember(fd))
		return true;
	if(it->second.isFull()){
		_sendMsg(fd, ERR_CHANNELISFULL(it->first));
		return false;
	}
	if(it->second.isInviteOnly() && !it->second.isInvited(fd)){
		_sendMsg(fd, ERR_INVITEONLYCHAN(it->first));
		return false;
	}
	if(it->second.hasPass() && it->second.getPass() != channels->second){
		_sendMsg(fd, ERR_BADCHANNELKEY(it->first));
		return false;
	}
	it->second.addMember(fd);
	it->second.removeInvite(fd);
	_broadcastChannelCommand(fd, it->second, "JOIN", "", "");
	_sendJoinReplies(fd, it->second);
	return true;
}

bool Server::_handleJoin(int fd, const Command &command)
{
	std::string channel;
	std::string pass;
	std::map<std::string, std::string> channels;
	int check = 0;

	if (command.params.size() > 2)
	{
		_sendMsg(fd, ERR_TOOMANYTARGETS("JOIN"));
		return false;
	}
	if (command.params.empty())
	{
		_sendMsg(fd, ERR_NEEDMOREPARAMS("JOIN"));
		return false;
	}
	channel = command.params[0];
	if (command.params.size() > 1)
		pass = command.params[1];
	channels = _buildChannelMap(channel, pass, fd, &check);
	if(check)
		return false;
	std::map<std::string, std::string>::const_iterator it;
	for (it = channels.begin(); it != channels.end(); it++) {
		if(!_parseChannel(it, fd))
			return false;
	}
	bool ret = true;
	for (it = channels.begin(); it != channels.end(); it++) {
		if(!buildChan(it, fd))
			ret = false;
	}
	return ret;
}
