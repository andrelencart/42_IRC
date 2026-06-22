/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dicosta- <dicosta-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 16:49:29 by dicosta-          #+#    #+#             */
/*   Updated: 2026/05/27 19:51:27 by dicosta-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"


volatile sig_atomic_t g_stop = 0;

void signalHandler(int sig){
	(void)sig;
	g_stop = 1;
}

Server::Server(): _port(0), _password(""), _servFd(-1) {}

Server::Server(int port, std::string password, std::string serverName): _port(port), _password(password), _serverName(serverName), _servFd(-1) {}

Server::~Server() {
	for (size_t i = 1; i < _fds.size(); i++)
		close(_fds[i].fd);
	if (_servFd != -1)
		close(_servFd);
	std::cout << "Server Shutdown!" << std::endl;
}

void Server::_setupSocket() {
	_servFd = socket(AF_INET, SOCK_STREAM, 0); //Creates a TCP socket. AF_INET = IPv4, SOCK_STREAM = TCP (reliable, ordered). Returns a file descriptor (_servFd)
	if (_servFd == -1)
		throw std::runtime_error("socket() failed!");

	int opt = 1;
	if (setsockopt(_servFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) //Tells the OS to allow reusing the port immediately after the server stops. Without this, if you restart the server quickly you get "address already in use" for ~60 seconds
		throw std::runtime_error("setsockopt() failed!");
	fcntl(_servFd, F_SETFL, O_NONBLOCK); //Sets the listening socket to non-blocking mode. This means accept() won't freeze the server if called when no client is waiting — it just returns -1 with EWOULDBLOCK instead

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET; // IPv4
	addr.sin_addr.s_addr = INADDR_ANY; // accept connections on any network interface
	addr.sin_port = htons(_port); // the port in network byte order (big-endian)

	if (bind(_servFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) //Attaches the socket to that address/port. After this, the OS knows "this socket owns port X".
		throw std::runtime_error("bind() failed!");
	
	if (listen(_servFd, SOMAXCONN) == -1) // Tells the OS to start accepting incoming connection requests on that socket. SOMAXCONN is the max queue of pending connections waiting to be accept()ed
		throw std::runtime_error("listen() failed!");
}

void Server::_acceptNewClient() {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(_servFd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
	if (clientFd == -1)
		throw std::runtime_error("accept() failed!");
	
	fcntl(clientFd, F_SETFL, O_NONBLOCK);
	Client newClient(clientFd);
	_clients[clientFd] = newClient;
	std::cout << "New client connected: fd " << newClient.getClientFD() << std::endl;
  
	struct pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN; // This Flag means this "wake me up when this fd has data ready to read"
	clientPollFd.revents = 0;
	_fds.push_back(clientPollFd);
}

std::map<std::string, std::string> buildMap(std::string channel, std::string pass, int fd, int *check){
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
		if (channel[0] == ','){
			_sendMsg2(fd, ERR_BADCHANMASK("JOIN"));
			*check = 1;
		}
		pos = 0;
		pos2 = 0;
		temp = "";
	}
	if (pass[0]){
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
	if (pass[0]){
		_sendMsg2(fd, ERR_NEEDMOREPARAMS("JOIN"));
		*check = 1;
	}
	return channels;
}

bool parseChan(std::map<std::string, std::string>::const_iterator it, int fd){
	if (it->first[0] != '&' && it->first[0] != '#')
	{
		_sendMsg2(fd, ERR_BADCHANMASK("JOIN"));
		return false;
	}
	if (it->first.find(7) != std::string::npos)
	{
		_sendMsg2(fd, ERR_BADCHANMASK("JOIN"));
		return false;
	}
	if (it->first.size() > 200)
	{
		_sendMsg2(fd, ERR_BADCHANMASK("JOIN"));
		return false;
	}
	return true;
}

bool Server::buildChan(std::map<std::string, std::string>::const_iterator channels, int fd){
	std::map<std::string, Channel>::iterator it = _channels.find(channels->first);
	if(it == _channels.end()){
		Channel newChan(channels->first);
		if(channels->second != "")
			newChan.setPass(channels->second);
		newChan.addMember(fd);
		_channels.insert(std::pair<std::string, Channel>(channels->first, newChan));
		return true;
	}
	if(it->second.isFull()){
		return false;
	}
	if(it->second.isInviteOnly() && !it->second.isInvited(fd)){
		return false;
	}
	if(it->second.hasPass() && it->second.getPass() != channels->second){
		return false;
	}
	it->second.addMember(fd);
	return true;
}

bool Server::_handleJoin(int fd, std::string line)
{
	std::istringstream iss(line);
	std::string channel;
	std::string pass;
	std::string check_no;
	std::map<std::string, std::string> channels;
	int check = 0;

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
	channels = buildMap(channel, pass, fd, &check);
	if(check)
		return false;
	std::map<std::string, std::string>::const_iterator it;
    for (it = channels.begin(); it != channels.end(); it++) {
        std::cout << "Chave: " << it->first 
                  << " | Valor: " << it->second 
                  << std::endl;
		if(!parseChan(it, fd))
			return false;
    }
	std::cout << "check_no: " << check_no << std::endl;
	std::cout << "channel: " << channel << std::endl;
	std::cout << "pass: " << pass << std::endl;
	bool ret = true;
	for (it = channels.begin(); it != channels.end(); it++) {
		if(!buildChan(it, fd))
			ret = false;
    }
	return ret;
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
		_handleUser(fd, param);
		
	}
	else if (command == "HELP")
	{
		_handleHelp(fd);
	}
	//else if (command == "KICK")
	//{
	//	_handleKick(fd);
	//}
	//else if (command == "INVITE")
	//{
	//	_handleInvite(fd);
	//}
	//else if (command == "TOPIC")
	//{
	//	_handleTopic(fd);
	//}
	//else if (command == "MODE")
	//{
	//	_handleMode(fd);
	//}
	if (_clients[fd].getAuth() == false && _clients[fd].getPassword() && !_clients[fd].getNickname().empty() && !_clients[fd].getUsername().empty())
	{
		_clients[fd].setAuth(true);
		// Created a welcome message according to IRC standards, Need to change servername.
		std::stringstream ss;
		ss << ":" << _serverName << " 001 " << _clients[fd].getNickname() << ":Welcome to the Internet Relay Network " << _clients[fd].getNickname() << "!" << _clients[fd].getUsername() << "@" << "localhost\r\n"; 
		_sendMsg(fd, ss.str());
	}
	if (command == "JOIN" && _clients[fd].getAuth() == true)
	{
		_handleJoin(fd, line);
	}
	if(command[0] == '#')
		broadcastToChannel(command, param, fd);
	return true;
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

bool Server::_processBuffer(int fd) {
	size_t pos;

	while ((pos = _clients[fd].getReadBuffer().find("\r\n")) != std::string::npos) {
		std::string line = _clients[fd].getReadBuffer().substr(0, pos);
		if (_clients[fd].getAuth())
			std::cout << _clients[fd].getNickname() << ": " << line << std::endl;
		_clients[fd].eraseBuffer(pos);
		if (!_processCommand(fd, line))
			return false;
	}
	return true;
}

void Server::_loopServer() {
	struct pollfd servPollFd;
	servPollFd.fd = _servFd;
	servPollFd.events = POLLIN; // This Flag means this "wake me up when this fd has data ready to read"
	servPollFd.revents = 0;
	_fds.push_back(servPollFd);
	signal(SIGINT, signalHandler);
	while (!g_stop) {
		int connected = poll(_fds.data(), _fds.size(), -1);
		if(connected == -1){
			if (errno == EINTR)
				break;
			throw std::runtime_error("poll() failed!");
		}
		for(size_t i = 0; i < _fds.size(); i++){
			if (_fds[i].revents & POLLIN) { // if there is data to read in that fd
				if (i == 0){
					_acceptNewClient();
				}
				else {
					if (_handleClient(_fds[i].fd)){
						close(_fds[i].fd);
						_fds.erase(_fds.begin() + i);
						i--;
					}
				}
			}
		}
	}
}

void Server::start() {
	//signal(SIGPIPE, SIG_IGN); //registers a handler for the SIGPIPE signal and sets it to SIG_IGN (ignore).
	_setupSocket();
	std::cout << "Server is up on port " << _port << std::endl;
	_loopServer();
}