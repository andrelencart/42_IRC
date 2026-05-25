#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "../includes/Server.hpp"

//We have leaks in poll when the CTRL C happens inside the server. Are they valid or need to be handled?

int main(int ac, char *av[])
{
	if (ac != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}
	int port = atoi(av[1]);
	if (port <= 0 || port > 65535){
		std::cerr << "Error: Port not valid!" << std::endl;
		return 1;
	}
	try {
		std::cout << "Trying to connect\n";
		Server s(port, av[2]);
		s.start();
	}
	catch(std::exception &e){
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
