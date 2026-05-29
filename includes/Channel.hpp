#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <poll.h>
#include "Server.hpp"

class Channel
{
	private:

        std::string _name;
		
	public:

		// Constructors
		Channel();
		Channel(std::string channel);
		Channel(const Channel &other);

		// Operators
		Channel& operator=(const Channel &other);

		// Getters

		// Setters


		//Others
        
};

void _sendMsg2(int fd, std::string msg);
bool _handleJoin(int fd, std::string line);

#endif