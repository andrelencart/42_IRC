#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <map>
#include <poll.h>
#include <csignal>


class Server {
	private:
		int _port;
		std::string _password;
		int _servFd;
		std::vector<struct pollfd> _fds;
		std::map<int, std::string> _clientBuffers;
		std::map<int, bool> _authenticated; // will be deleted after the migration to the Client Class
		std::map<int, std::string> _nicknames; // will be deleted after the migration to the Client Class
		std::map<int, std::string> _usernames; // will be deleted after the migration to the Client Class

		void _setupSocket();
		void _loopServer();
		void _acceptNewClient();
		bool _handleClient(int fd);
		bool _processBuffer(int fd);
		bool _processCommand(int fd, std::string line);
		bool _handlePass(int fd, std::istringstream& iss);
		bool _handleNick(int fd, std::istringstream& iss);
		bool _handleUser(int fd, std::istringstream& iss);

		//Helpers / Errors

		void _sendMsg(int fd, std::string msg);
		void _removeClient(int fd);

		Server(const Server& other);
		Server& operator=(const Server& other);

	public:
		Server();
		Server(int port, std::string password);
		~Server();

		void start();

};

#endif