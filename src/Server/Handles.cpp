#include "../../includes/Server.hpp"

bool Server::_handleClient(int fd) {
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes == 0) {
		_removeClient(fd);
		return true;
	}
	else if (bytes == -1){
		std::cerr << "recv() error on fd " << fd << std::endl;
		_removeClient(fd);
		return true;
	}
	else {
		_clientBuffers[fd] += std::string(buffer, bytes);
		if (!_processBuffer(fd))
			return true;
		return false;
	}
}

bool Server::_handlePass(int fd, std::istringstream& iss) {
	std::string password;
	iss >> password;

	if (password.empty()){
		_sendMsg(fd, ":server 464 * :Password empty\r\n");
		return false; // disconnect fd,
	}
	else if (password != _password){
		_sendMsg(fd, ":server 464 * :Password incorrect\r\n");
		return false;
	}
	else{
		_authenticated[fd] = true;
		return true;
	}
}

bool Server::_handleNick(int fd, std::istringstream& iss){
	std::string nick;
	iss >> nick;

	std::map<int, std::string>::iterator i;
	if (nick.empty()){
		_sendMsg(fd, ":server 431 * :Nickname is empty\r\n");
		return false;
	}
	for (i = _nicknames.begin(); i != _nicknames.end(); i++){
		if (i->second == nick){
			_sendMsg(fd, ":server 433 * " + nick + " :Nickname is already in use\r\n");
			return false;
		}
	}
	_nicknames[fd] = nick;
	return true;
}

bool Server::_handleUser(int fd, std::istringstream& iss){
	std::string user;
	iss >> user;

	if (user.empty()){
		 _sendMsg(fd, ":server 461 * USER :Not enough parameters\r\n");
		 return false;
	}
	else if (_usernames.count(fd) > 0){
		_sendMsg(fd, ":server 462 * :You may not reregister\r\n");
		return false;
	}
	_usernames[fd] = user;
	return true;
}