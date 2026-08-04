#include "../../includes/Server.hpp"

void Server::_handleHelp(int fd)
{
	_sendMsg(fd,"IRC Connection Manual\r\n\n");
	_sendMsg(fd,"1. PASS <server password>\r\n");
	_sendMsg(fd,"2. NICK <user nickname>\r\n");
	_sendMsg(fd,"3. USER <user username>\r\n");
	_sendMsg(fd,"4. JOIN #<channel name> (optional)<password>\r\n");
};

bool Server::_handlePass(int fd, std::string password){
	if ( _clients[fd].getPassword() == true){
		_sendMsg(fd, ERR_ALREADYREGISTED());
		return true;
	}
	if (password.empty())
	{
		_sendMsg(fd, ERR_NEEDMOREPARAMS("PASS"));
		_removeClient(fd);
		return false; // disconnect fd,
	}
	if (password != _password)
	{
		_sendMsg(fd, ERR_PASSWDMISMATCH());
		_removeClient(fd);
		return false;
	}
	_clients[fd].setPassword(true);
	return true;
}

bool Server::_handleNick(int fd, std::string nick)
{
	if (nick.empty()){
		_sendMsg(fd, ERR_NONICKNAMEGIVEN());
		return (false);
	} 
	if (nick.size() > 9 || isdigit(nick[0]) || nick[0] == '-') // nicknames cant be longer than 9 chars && Cant start with number or hyphen
	{
		_sendMsg(fd, ERR_ERRONEUSNICKNAME(nick));
		return (false);
	}
	for (size_t i = 0; i < nick.size(); i++)
	{
		if (isspace(nick[i]) || !isascii(nick[i]) || nick[i] == '@' || nick[i] == '!' || nick[i] == '.' || nick[i] == ':' || nick[i] == ',') // cant have any of the following chars
		{
			_sendMsg(fd, ERR_ERRONEUSNICKNAME(nick));
			return (false);
		}
	}
	if (_nickInUse(nick, fd))
	{
		_sendMsg(fd, ERR_NICKNAMEINUSE(nick));
		return false;
	}
	_clients[fd].setNickname(nick);
	return (true);
}

bool Server::_handleUser(int fd, std::string line)
{
	std::istringstream iss(line);
	std::vector<std::string> params;
	std::string token;

	while (iss >> token)
		params.push_back(token);

	if (params.size() < 5){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (params[2] != "0" || params[3] != "*"){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (!_clients[fd].getUsername().empty()){
		_sendMsg(fd, ERR_ALREADYREGISTED() );
		return false;
	}
	_clients[fd].setUsername(params[1]);
	return true;
}

bool Server::_checkDupes(std::string type, std::string toCheck) const
{
	std::map<int, Client>::const_iterator i;
	for (i = _clients.begin(); i != _clients.end(); i++)
	{
		if (type == "username" && i->second.getUsername() == toCheck)
			return (true);
		else if (type == "nickname" && i->second.getNickname() == toCheck)
			return (true);
	}
	return (false);
};

bool Server::_nickInUse(std::string nick, int currentFd) const
{
	std::map<int, Client>::const_iterator it;

	for (it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->first != currentFd && it->second.getNickname() == nick)
		return true;
	}
	return false;
}

int Server::_userToFd(std::string username, int fd, std::string cmdErr){
	std::map<int, Client>::iterator uname;
	for(uname = _clients.begin(); uname != _clients.end(); uname++){
		if(uname->second.getNickname() == username)
			break;
	}
	if(uname == _clients.end()){
		_sendMsg(fd, ERR_BADCHANMASK(cmdErr));
		return false;
	}
	return uname->second.getClientFD();
}

bool Server::_handleKick(int fd, std::string line)
{
	std::istringstream iss(line);
	std::string check_no;
	std::string channel;
	std::string username;
	std::string comment;
	int user;

	iss >> check_no;
	iss >> channel;
	iss >> username;
	iss >> comment;
	iss >> check_no;
	if (check_no != "KICK")
	{
		_sendMsg(fd, ERR_TOOMANYTARGETS("KICK"));
		return false;
	}
	if(channel == "" || username == ""){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("KICK"));
		return false;
	}
	std::map<std::string, Channel>::iterator it;
	it = _channels.find(channel);
	if(it == _channels.end()){
		_sendMsg(fd, ERR_BADCHANMASK("KICK"));
		return false;
	}
	user = _userToFd(username, fd, "KICK");
	if(!it->second.isMember(fd) || !it->second.isOperator(fd) || !it->second.isMember(user) || it->second.isOperator(user))
	{
		_sendMsg(fd, ERR_NOSUCHNICK("KICK"));
		return false;
	}
	if (comment.size() > 0 && comment[0] == ':')
		comment = comment.substr(1);
	_broadcastChannelCommand(fd, it->second, "KICK", username, comment);
	it->second.removeMember(user);
	return true;
}

bool Server::_handleInvite(int fd, std::string line)
{
	std::istringstream iss(line);
	std::string check_no;
	std::string username;
	std::string channel;
	int user;

	iss >> check_no;
	iss >> username;
	iss >> channel;
	iss >> check_no;
	if (check_no != "INVITE")
	{
		_sendMsg(fd, ERR_TOOMANYTARGETS("INVITE"));
		return false;
	}
	if(channel == "" || username == ""){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("INVITE"));
		return false;
	}
	std::map<std::string, Channel>::iterator it;
	it = _channels.find(channel);
	if(it == _channels.end()){
		_sendMsg(fd, ERR_BADCHANMASK("INVITE"));
		return false;
	}
	if(!it->second.isInviteOnly() && !it->second.hasPass()){
		return false;
	}
	user = _userToFd(username, fd, "INVITE");
	it->second.invite(user);
	return true;
}

bool Server::_handleTopic(int fd, std::string line) {
	std::istringstream iss(line);
	std::vector<std::string>params;
	std::string token;
	std::string topic;
	
	while (iss >> token)
		params.push_back(token);
	size_t pos = line.find(':');
	if (pos != std::string::npos)
		topic = line.substr(pos + 1);

	if (params.size() < 2){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("TOPIC"));
		return false;
	}

	Channel *channel = _getChannel(params[1]);
	if (channel == NULL){
		_sendMsg(fd, ERR_NOSUCHCHANNEL(params[1]));
		return false;
	}
	if (!channel->isMember(fd)){
		_sendMsg(fd, ERR_NOTONCHANNEL(params[1]));
		return false;
	}
	if (topic.empty()){
		if (channel->getTopic().empty())
			_sendMsg(fd, RPL_NOTOPIC(_clients[fd].getNickname(), channel->getName()));
		else
			_sendMsg(fd, RPL_TOPIC(_clients[fd].getNickname(), channel->getName(), channel->getTopic()));
		return true;
	}
	channel->setTopic(topic);
	_broadcastChannelCommand(fd, *channel, "TOPIC", "", topic);
	return true;
}

bool Server::_processCommand(int fd, std::string line) {
	std::istringstream iss(line);
	std::string command;
	std::string param;

	// Handle functions recebiam o "iss" e eu mudei para "param" para receber o valor diretamente
	iss >> command;
	iss >> param;
	if (command != "PASS" && !_clients[fd].getPassword()){
		_sendMsg(fd, ":server 451 * :You have not registered\r\n");
		return true;
	}
	if (command == "PASS"){
		if (!_handlePass(fd, param))
			return false;
	}
	else if (command == "NICK"){
		_handleNick(fd, param);
			
	}
	else if (command == "USER"){
		_handleUser(fd, line);
		
	}
	else if (command == "HELP")
	{
		_handleHelp(fd);
	}
	
	
	else if (command == "TOPIC")
	{
		_handleTopic(fd, line);
	}
	//else if (command == "MODE")
	//{
	//	_handleMode(fd);
	//}
	if (_clients[fd].getAuth() == false && _clients[fd].getPassword() && !_clients[fd].getNickname().empty() && !_clients[fd].getUsername().empty())
	{
		_clients[fd].setAuth(true);
		// Created a welcome message according to IRC standards, Need to change servername.
		std::stringstream ss;
		ss << ":" << _serverName << " 001 " << _clients[fd].getNickname() << " :Welcome to the Internet Relay Network " << _clients[fd].getNickname() << "!" << _clients[fd].getUsername() << "@" << "localhost\r\n"; 
		_sendMsg(fd, ss.str());
	}
	if (_clients[fd].getAuth() == true){
		if (command == "JOIN")
		{
			_handleJoin(fd, line);
		}
		else if (command == "KICK")
		{
			_handleKick(fd, line);
		}
		else if (command == "INVITE")
		{
			_handleInvite(fd, line);
		}
		else if (command == "PRIVMSG")
		{
			_handleMsg(fd, line);
		}
	}
	if(command[0] == '#')
		broadcastToChannel(command, param, fd); //temporary for testing broadcast to channel function; usage: 'channel' 'msg'.
	return true;
}

void	Server::_handleMsg(int fd, std::string line){
	std::istringstream iss(line);
	std::string check_no;
	std::string username;
	std::string msg;
	int user;
	size_t pos;

	iss >> check_no;
	iss >> username;
	pos = line.find(':');
	if (pos != std::string::npos)
		msg = line.substr(pos + 1);
	else
		iss >> msg;
	if (username.empty()){
		_sendMsg(fd, ERR_NORECIPIENT("PRIVMSG"));
		return ;
	}
	if (msg.empty()){
		_sendMsg(fd, ERR_NOTEXTTOSEND());
		return ;
	}
	if (username[0] == '#' || username[0] == '&'){
		Channel *channel = _getChannel(username);
		if (channel == NULL){
			_sendMsg(fd, ERR_NOSUCHCHANNEL(username));
			return ;
		}
		if (!channel->isMember(fd)){
			_sendMsg(fd, ERR_CANNOTSENDTOCHAN(username));
			return ;
		}
		_broadcastChannelCommand(fd, *channel, "PRIVMSG", "", msg, fd);
		return ;
	}
	user = _userToFd(username, fd, "PRIVMSG");
	std::stringstream ss;
	ss << _clientPrefix(fd) << " PRIVMSG " << username << " :" << msg << "\r\n";
	_sendMsg(user, ss.str());
}
