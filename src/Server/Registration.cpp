#include "../../includes/Server.hpp"

void Server::_handleHelp(int fd)
{
	_sendMsg(fd,"IRC Connection Manual\r\n\n");
	_sendMsg(fd,"1. PASS <server password>\r\n");
	_sendMsg(fd,"2. NICK <user nickname>\r\n");
	_sendMsg(fd,"3. USER <user username>\r\n");
	_sendMsg(fd,"4. JOIN #<channel name> (optional)<password>\r\n");
}

bool Server::_handlePass(int fd, const Command &command) {
	std::string password;

	if (!command.params.empty())
		password = command.params[0];
	else if (command.hasTrailing)
		password = command.trailing;
	if (_clients[fd].getPassword() == true) {
		_sendMsg(fd, ERR_ALREADYREGISTED());
		return true;
	}
	if (password.empty()) {
		_sendMsg(fd, ERR_NEEDMOREPARAMS("PASS"));
		return true;
	}
	if (password != _password) {
		_sendMsg(fd, ERR_PASSWDMISMATCH());
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
		_sendMsg(fd, ERR_NONICKNAMEGIVEN());
		return false;
	}
	if (nick.size() > 9 || isdigit(nick[0]) || nick[0] == '-') {
		_sendMsg(fd, ERR_ERRONEUSNICKNAME(nick));
		return false;
	}
	for (size_t i = 0; i < nick.size(); i++) {
		if (isspace(nick[i]) || !isascii(nick[i]) || nick[i] == '@'
			|| nick[i] == '!' || nick[i] == '.' || nick[i] == ':'
			|| nick[i] == ',') {
			_sendMsg(fd, ERR_ERRONEUSNICKNAME(nick));
			return false;
		}
	}
	if (_nickInUse(nick, fd)) {
		_sendMsg(fd, ERR_NICKNAMEINUSE(nick));
		return false;
	}
	_clients[fd].setNickname(nick);
	return true;
}

bool Server::_handleUser(int fd, const Command &command)
{
	if (command.params.size() < 3
		|| (command.params.size() < 4 && !command.hasTrailing)) {
		_sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (command.params[1] != "0" || command.params[2] != "*") {
		_sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		return false;
	}
	if (!_clients[fd].getUsername().empty()) {
		_sendMsg(fd, ERR_ALREADYREGISTED());
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
		std::stringstream ss;
		ss << ":" << _serverName << " 001 " << _clients[fd].getNickname()
			<< " :Welcome to the Internet Relay Network "
			<< _clients[fd].getNickname() << "!"
			<< _clients[fd].getUsername() << "@localhost\r\n";
		_sendMsg(fd, ss.str());
	}
}
