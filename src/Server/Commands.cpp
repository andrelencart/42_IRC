#include "../../includes/Server.hpp"
#include <cstdlib>

void Server::_handleHelp(int fd)
{
	_sendMsg(fd,"IRC Connection Manual\r\n\n");
	_sendMsg(fd,"1. PASS <server password>\r\n");
	_sendMsg(fd,"2. NICK <user nickname>\r\n");
	_sendMsg(fd,"3. USER <user username>\r\n");
	_sendMsg(fd,"4. JOIN #<channel name> (optional)<password>\r\n");
};

bool Server::_handlePass(int fd, const Command &command){
	std::string password;

	if (!command.params.empty())
		password = command.params[0];
	else if (command.hasTrailing)
		password = command.trailing;
	if ( _clients[fd].getPassword() == true){
		_sendMsg(fd, ERR_ALREADYREGISTED());
		return true;
	}
	if (password.empty())
	{
		_sendMsg(fd, ERR_NEEDMOREPARAMS("PASS"));
		_clients[fd].setCloseAfterWrite(true);
		return true;
	}
	if (password != _password)
	{
		_sendMsg(fd, ERR_PASSWDMISMATCH());
		_clients[fd].setCloseAfterWrite(true);
		return true;
	}
	_clients[fd].setPassword(true);
	return true;
}

bool Server::_handleNick(int fd, const Command &command)
{
	std::string nick;

	if (!command.params.empty())
		nick = command.params[0];
	else if (command.hasTrailing)
		nick = command.trailing;
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

bool Server::_handleUser(int fd, const Command &command)
{
	if (command.params.size() < 3
		|| (command.params.size() < 4 && !command.hasTrailing)){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (command.params[1] != "0" || command.params[2] != "*"){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (!_clients[fd].getUsername().empty()){
		_sendMsg(fd, ERR_ALREADYREGISTED() );
		return false;
	}
	_clients[fd].setUsername(command.params[0]);
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

bool Server::_handleKick(int fd, const Command &command)
{
	std::string channel;
	std::string username;
	std::string comment;
	int user;

	if (command.params.size() > 3)
	{
		_sendMsg(fd, ERR_TOOMANYTARGETS("KICK"));
		return false;
	}
	if (command.params.size() < 2){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("KICK"));
		return false;
	}
	channel = command.params[0];
	username = command.params[1];
	if (command.hasTrailing)
		comment = command.trailing;
	else if (command.params.size() > 2)
		comment = command.params[2];
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
	_broadcastChannelCommand(fd, it->second, "KICK", username, comment);
	it->second.removeClient(user);
	return true;
}

bool Server::_handleInvite(int fd, const Command &command)
{
	std::string username;
	std::string channel;
	int user;

	if (command.params.size() > 2)
	{
		_sendMsg(fd, ERR_TOOMANYTARGETS("INVITE"));
		return false;
	}
	if (command.params.size() < 2){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("INVITE"));
		return false;
	}
	username = command.params[0];
	channel = command.params[1];
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

bool Server::_handleTopic(int fd, const Command &command) {
	if (command.params.empty()){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("TOPIC"));
		return false;
	}

	Channel *channel = _getChannel(command.params[0]);
	if (channel == NULL){
		_sendMsg(fd, ERR_NOSUCHCHANNEL(command.params[0]));
		return false;
	}
	if (!channel->isMember(fd)){
		_sendMsg(fd, ERR_NOTONCHANNEL(command.params[0]));
		return false;
	}
	if (!command.hasTrailing){
		if (channel->getTopic().empty())
			_sendMsg(fd, RPL_NOTOPIC(_clients[fd].getNickname(), channel->getName()));
		else
			_sendMsg(fd, RPL_TOPIC(_clients[fd].getNickname(), channel->getName(), channel->getTopic()));
		return true;
	}
	if (channel->isTopicRestricted() && !channel->isOperator(fd)){
		_sendMsg(fd, ERR_CHANOPRIVSNEEDED(channel->getName()));
		return false;
	}
	channel->setTopic(command.trailing);
	_broadcastChannelCommand(fd, *channel, "TOPIC", "", command.trailing);
	return true;
}

bool Server::_isValidChannelMode(char mode) const {
	return (mode == 'i' || mode == 't' || mode == 'k' || mode == 'o' || mode == 'l');
}

bool Server::_validateModeRequest(int fd, std::string channelName, std::string modeString, Channel **channel) {
	if (channelName.empty() || modeString.empty()){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
		return false;
	}
	*channel = _getChannel(channelName);
	if (*channel == NULL){
		_sendMsg(fd, ERR_NOSUCHCHANNEL(channelName));
		return false;
	}
	if (!(*channel)->isMember(fd)){
		_sendMsg(fd, ERR_NOTONCHANNEL(channelName));
		return false;
	}
	if (!(*channel)->isOperator(fd)){
		_sendMsg(fd, ERR_CHANOPRIVSNEEDED(channelName));
		return false;
	}
	if (modeString[0] != '+' && modeString[0] != '-'){
		_sendMsg(fd, ERR_UMODEUNKNOWNFLAG());
		return false;
	}
	for (size_t i = 1; i < modeString.size(); i++){
		if (!_isValidChannelMode(modeString[i])){
			std::string unknownMode;
			unknownMode += modeString[i];
			_sendMsg(fd, ERR_UNKNOWNMODE(unknownMode));
			return false;
		}
	}
	return true;
}

bool Server::_applyKeyMode(int fd, Channel *channel, std::string &modeString, std::string modeParam) {
	if (modeString == "+k"){
		if (modeParam.empty()){
			_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
			return false;
		}
		channel->setPass(modeParam);
		modeString += " " + modeParam;
	}
	else
		channel->removePass();
	return true;
}

int Server::_findClientFdByNick(std::string nick) const {
	std::map<int, Client>::const_iterator client = _clients.begin();

	while (client != _clients.end()){
		if (client->second.getNickname() == nick)
			return client->first;
		client++;
	}
	return -1;
}

bool Server::_applyOperatorMode(int fd, Channel *channel, std::string channelName, std::string &modeString, std::string modeParam) {
	if (modeParam.empty()){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
		return false;
	}
	int targetFd = _findClientFdByNick(modeParam);
	if (targetFd == -1){
		_sendMsg(fd, ERR_NOSUCHNICK(modeParam));
		return false;
	}
	if (!channel->isMember(targetFd)){
		_sendMsg(fd, ERR_USERNOTINCHANNEL(modeParam, channelName));
		return false;
	}
	if (modeString == "+o")
		channel->addOperator(targetFd);
	else
		channel->removeOperator(targetFd);
	modeString += " " + modeParam;
	return true;
}

bool Server::_applyLimitMode(int fd, Channel *channel, std::string &modeString, std::string modeParam) {
	if (modeString == "-l"){
		channel->setUserLimit(0);
		return true;
	}
	if (modeParam.empty()){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
		return false;
	}
	for (size_t i = 0; i < modeParam.size(); i++){
		if (!isdigit(modeParam[i])){
			_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
			return false;
		}
	}
	int limit = std::atoi(modeParam.c_str());
	if (limit <= 0){
		_sendMsg(fd, ERR_NEEDMOREPARAMS("MODE"));
		return false;
	}
	channel->setUserLimit(limit);
	modeString += " " + modeParam;
	return true;
}

bool Server::_applyMode(int fd, Channel *channel, std::string channelName, std::string &modeString, std::string modeParam) {
	if (modeString == "+i")
		channel->setInviteOnly(true);
	else if (modeString == "-i")
		channel->setInviteOnly(false);
	else if (modeString == "+t")
		channel->setTopicRestricted(true);
	else if (modeString == "-t")
		channel->setTopicRestricted(false);
	else if (modeString == "+k" || modeString == "-k")
		return _applyKeyMode(fd, channel, modeString, modeParam);
	else if (modeString == "+o" || modeString == "-o")
		return _applyOperatorMode(fd, channel, channelName, modeString, modeParam);
	else if (modeString == "+l" || modeString == "-l")
		return _applyLimitMode(fd, channel, modeString, modeParam);
	else
		return true;
	return true;
}

bool Server::_handleMode(int fd, const Command &command) {
	std::string channelName;
	std::string modeString;
	std::string modeParam;
	Channel *channel;

	if (!command.params.empty())
		channelName = command.params[0];
	if (command.params.size() > 1)
		modeString = command.params[1];
	if (command.params.size() > 2)
		modeParam = command.params[2];

	if (!_validateModeRequest(fd, channelName, modeString, &channel))
		return false;
	if (!_applyMode(fd, channel, channelName, modeString, modeParam))
		return false;
	_broadcastChannelCommand(fd, *channel, "MODE", modeString, "");
	return true;
}

void Server::_initCommandHandlers() {
	_commandHandlers["JOIN"] = &Server::_handleJoin;
	_commandHandlers["KICK"] = &Server::_handleKick;
	_commandHandlers["INVITE"] = &Server::_handleInvite;
	_commandHandlers["TOPIC"] = &Server::_handleTopic;
	_commandHandlers["MODE"] = &Server::_handleMode;
	_commandHandlers["PRIVMSG"] = &Server::_handleMsg;
}

bool Server::_dispatchCommand(int fd, const Command &command) {
	std::map<std::string, CommandHandler>::iterator it = _commandHandlers.find(command.name);

	if (it != _commandHandlers.end())
		return (this->*(it->second))(fd, command);
	_sendMsg(fd, ERR_UNKNOWNCOMMAND(command.name));
	return false;
}

bool Server::_checkRegistration(int fd, const Command &command) {
	if (command.name == "PASS")
		return true;
	if (!_clients[fd].getPassword()){
		_sendMsg(fd, ":server 451 * :You have not registered\r\n");
		return false;
	}
	if (command.name != "NICK" && command.name != "USER"
		&& command.name != "HELP" && !_clients[fd].getAuth()){
		_sendMsg(fd, ":server 451 * :You have not registered\r\n");
		return false;
	}
	return true;
}

bool Server::_dispatchRegistrationCommand(int fd, const Command &command) {
	if (command.name == "PASS"){
		_handlePass(fd, command);
	}
	else if (command.name == "NICK"){
		_handleNick(fd, command);
	}
	else if (command.name == "USER"){
		_handleUser(fd, command);
	}
	else if (command.name == "HELP")
	{
		_handleHelp(fd);
	}
	else
		return false;
	return true;
}

void Server::_tryAuthenticateClient(int fd) {
	if (_clients[fd].getAuth() == false && _clients[fd].getPassword() && !_clients[fd].getNickname().empty() && !_clients[fd].getUsername().empty())
	{
		_clients[fd].setAuth(true);
		// Created a welcome message according to IRC standards, Need to change servername.
		std::stringstream ss;
		ss << ":" << _serverName << " 001 " << _clients[fd].getNickname() << " :Welcome to the Internet Relay Network " << _clients[fd].getNickname() << "!" << _clients[fd].getUsername() << "@" << "localhost\r\n"; 
		_sendMsg(fd, ss.str());
	}
}

bool Server::_processCommand(int fd, const std::string &line) {
	Command command;

	if (!_parseCommand(line, command))
		return true;
	if (!_checkRegistration(fd, command))
		return true;
	if (_dispatchRegistrationCommand(fd, command)) {
		_tryAuthenticateClient(fd);
		return true;
	}
	_dispatchCommand(fd, command);
	return true;
}

bool	Server::_handleMsg(int fd, const Command &command){
	std::string username;
	std::string msg;
	int user;

	if (command.params.empty()){
		_sendMsg(fd, ERR_NORECIPIENT("PRIVMSG"));
		return false;
	}
	username = command.params[0];
	if (command.hasTrailing)
		msg = command.trailing;
	else if (command.params.size() > 1)
		msg = command.params[1];
	if (msg.empty()){
		_sendMsg(fd, ERR_NOTEXTTOSEND());
		return false;
	}
	if (username[0] == '#' || username[0] == '&'){
		Channel *channel = _getChannel(username);
		if (channel == NULL){
			_sendMsg(fd, ERR_NOSUCHCHANNEL(username));
			return false;
		}
		if (!channel->isMember(fd)){
			_sendMsg(fd, ERR_CANNOTSENDTOCHAN(username));
			return false;
		}
		_broadcastChannelCommand(fd, *channel, "PRIVMSG", "", msg, fd);
		return true;
	}
	user = _userToFd(username, fd, "PRIVMSG");
	if (user <= 0)
		return false;
	std::stringstream ss;
	ss << _clientPrefix(fd) << " PRIVMSG " << username << " :" << msg << "\r\n";
	_sendMsg(user, ss.str());
	return true;
}
