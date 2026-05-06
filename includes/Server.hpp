#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <poll.h>


class Server {
	private:
		int _port;
		std::string _password;
		int _servFd;
		std::vector<struct pollfd> _fds;

		void _setupSocket();
		void _loop();

		Server(const Server& other);
		Server& operator=(const Server& other);

	public:
		Server();
		Server(int port, std::string password);
		~Server();

		void start();

};

#endif