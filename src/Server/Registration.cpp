#include "../../includes/Server.hpp"

void Server::_handleHelp(int fd)
{
	_sendMsg(fd,"IRC Connection Manual\r\n\n");
	_sendMsg(fd,"1. PASS <server password>\r\n");
	_sendMsg(fd,"2. NICK <user nickname>\r\n");
	_sendMsg(fd,"3. USER <username> 0 * <realname>\r\n");
	_sendMsg(fd,"4. JOIN #<channel> (optional)<password>\r\n");
	_sendMsg(fd,"5. TOPIC #<channel> :<topic>\r\n");
	_sendMsg(fd,"6. INVITE #<channel> <nickname>\r\n");
	_sendMsg(fd,"7. KICK #<channel> <nickname>\r\n");
	_sendMsg(fd,"8. MODE #<channel> <rule>\r\n");
	_sendMsg(fd,"\trule: +i invite-only\r\n");
	_sendMsg(fd,"\trule: -i remove invite-only\r\n");
	_sendMsg(fd,"\trule: +t topic\r\n");
	_sendMsg(fd,"\trule: +k <password> add password\r\n");
	_sendMsg(fd,"\trule: -k remove password\r\n");
	_sendMsg(fd,"\trule: +o <nick> add operator previleges\r\n");
	_sendMsg(fd,"\trule: -o <nick> remove operator previleges\r\n");
	_sendMsg(fd,"\trule: +l add user limit\r\n");
	_sendMsg(fd,"\trule: -l remove user limit\r\n");
}

bool Server::_handlePass(int fd, const Command &command) {
	std::string password;

	if (!command.params.empty())
		password = command.params[0];
	else if (command.hasTrailing)
		password = command.trailing;
	if (_clients[fd].getPassword() == true) {
		_sendNumericReply(fd, ERR_ALREADYREGISTED());
		return true;
	}
	if (password.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("PASS"));
		return true;
	}
	if (password != _password) {
		_sendNumericReply(fd, ERR_PASSWDMISMATCH());
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
	if (nick.empty()) {
		_sendNumericReply(fd, ERR_NONICKNAMEGIVEN());
		return false;
	}
	if (nick.size() > 9 || isdigit(nick[0]) || nick[0] == '-') {
		_sendNumericReply(fd, ERR_ERRONEUSNICKNAME(nick));
		return false;
	}
	for (size_t i = 0; i < nick.size(); i++) {
		if (isspace(nick[i]) || !isascii(nick[i]) || nick[i] == '@'
			|| nick[i] == '!' || nick[i] == '.' || nick[i] == ':'
			|| nick[i] == ',') {
			_sendNumericReply(fd, ERR_ERRONEUSNICKNAME(nick));
			return false;
		}
	}
	if (_nickInUse(nick, fd)) {
		_sendNumericReply(fd, ERR_NICKNAMEINUSE(nick));
		return false;
	}
	_clients[fd].setNickname(nick);
	return true;
}

bool Server::_handleUser(int fd, const Command &command)
{
	if (command.params.size() < 3
		|| (command.params.size() < 4 && !command.hasTrailing)) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (command.params[1] != "0" || command.params[2] != "*") {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (!_clients[fd].getUsername().empty()) {
		_sendNumericReply(fd, ERR_ALREADYREGISTED());
		return false;
	}
	_clients[fd].setUsername(command.params[0]);
	return true;
}

void Server::_tryAuthenticateClient(int fd) {
	if (_clients[fd].getAuth() == false && _clients[fd].getPassword()
		&& !_clients[fd].getNickname().empty()
		&& !_clients[fd].getUsername().empty()) {
		_clients[fd].setAuth(true);
		_sendNumericReply(fd, RPL_WELCOME(
			std::string("Welcome to the Internet Relay Network ")
			+ _clients[fd].getNickname() + "!"
			+ _clients[fd].getUsername() + "@localhost"));
	}
}
