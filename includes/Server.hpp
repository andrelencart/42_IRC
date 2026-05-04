#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <unistd.h>

class Server {
	private:
		int _port;
		std::string _password;
		int _servFd;
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