#include "../includes/Server.hpp"

int main(int ac, char *av[])
{
	if (ac != 3
	|| std::string(av[2]).empty()
	|| std::string(av[2]).find_first_of(" \t\r\n\v\f")
		!= std::string::npos)
	{
		std::cerr << "Usage: ./ircserv <port> <password-without-spaces>" << std::endl;
		return 1;
	}
	else if(std::string(av[2]).empty())
	{
		std::cerr << "Password can't be empty." << std::endl;
		return 1;
	}
	else if(std::string(av[2]).find_first_of(" \t\n\r\f\v") != std::string::npos)
	{
		std::cerr << "Password can't contain whitespaces." << std::endl;
		return 1;
	}
	int port = atoi(av[1]);
	if (port <= 0 || port > 65535){
		std::cerr << "Error: Port not valid!" << std::endl;
		return 1;
	}
	try {
		std::cout << "Trying to connect\n";
		Server s(port, av[2], "irc.server.42");
		s.start();
	}
	catch(std::exception &e){
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
