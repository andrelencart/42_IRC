/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Handles.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dicosta- <dicosta-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 16:49:17 by dicosta-          #+#    #+#             */
/*   Updated: 2026/05/27 21:16:50 by dicosta-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"

void Server::_handleHelp(int fd)
{
	_sendMsg(fd,"IRC Connection Manual\r\n\n");
	_sendMsg(fd,"1. PASS <server password>\r\n");
	_sendMsg(fd,"2. NICK <user nickname>\r\n");
	_sendMsg(fd,"3. USER <user username>\r\n");
	_sendMsg(fd,"4. JOIN #<channel name> (optional)<password>\r\n");
};

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
		_clients[fd].appendReadBuffer(std::string(buffer, bytes));
	//	std::cout << _clients[fd].getReadBuffer(); // ADICIONADA PARA TESTE
		if (!_processBuffer(fd))
			return true;
		return false;
	}
}

bool Server::_handlePass(int fd, std::string password){
	if (password.empty())
	{
		_sendMsg(fd, ERR_NEEDMOREPARAMS("PASS"));
		_removeClient(fd);
		return false; // disconnect fd,
	}
	else if (password != _password)
	{
		_sendMsg(fd, ERR_PASSWDMISMATCH());
		_removeClient(fd);
		return false;
	}
	else
	{
		_clients[fd].setPassword(true);
		return true;
	}
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
	for (size_t i = 1; i < nick.size(); i++)
	{
		if (isspace(nick[i]) || !isascii(nick[i]) || nick[i] == '@' || nick[i] == '!' || nick[i] == '.' || nick[i] == ':' || nick[i] == ',') // cant have any of the following chars
		{
			_sendMsg(fd, ERR_ERRONEUSNICKNAME(nick));
			return (false);
		}
	}
	if(_checkDupes("nickname", nick))
	{
		_sendMsg(fd, ERR_NICKNAMEINUSE(nick));
		return false;
	}
	_clients[fd].setNickname(nick);
	return (true);
}

bool Server::_handleUser(int fd, std::string user)
{
	/* TO DO
		check for repeated nicks / users 
	*/
	if (user.empty()){
		 _sendMsg(fd, ERR_NEEDMOREPARAMS("USER"));
		 return false;
	}
	//Deleted user dupe check as usernames can be duped
	//if (_checkDupes("username", user)) //Added this check to see if Username is duped
	//{
	//	_sendMsg(fd, ":server DUNNOYET * : Username is already in use \r\n");
	//	return false;
	//}
	//!_clients[fd].getUsername().empty();
	//else if (_usernames.count(fd) > 0){
	else if (!_clients[fd].getUsername().empty()){ //changed this check to see if string username is empty
		_sendMsg(fd, ERR_ALREADYREGISTED() );
		return false;
	}
	_clients[fd].setUsername(user);
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