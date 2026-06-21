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
#include <set>
#include "Server.hpp"

class Channel
{
	private:

		std::string	_name;
		std::string	_pass;
		bool	_hasPass;
		size_t		_userLimit;
		bool	_inviteOnly;
		std::set<int>	_members;
		std::set<int>	_operators;
		std::set<int>	_invited;

	public:

		// Constructors
		Channel();
		Channel(std::string channel);
		Channel(const Channel &other);

		// Operators
		Channel& operator=(const Channel &other);

		// Getters
		std::string	getName() const;
		std::string	getPass() const;
		bool hasPass() const;
		bool getInviteOnly() const;
		int	getUserLimit() const;
		size_t	getMemberCount() const;
		const std::set<int>& getMembers() const;

		// Setters
		void setPass(std::string pass);
		void setInviteOnly(bool i);
		void setUserLimit(int limit);

		//Others
		void	addMember(int fd);
		// void	removeMember(int fd);
		// bool	isMember(int fd) const;
		// void	addOperator(int fd);
		// void	removeOperator(int fd);
		// bool	isOperator(int fd) const;
		// void	invite(int fd);
		bool	isInvited(int fd) const;
		bool	isFull() const;  
};

void _sendMsg2(int fd, std::string msg);
bool _handleJoin(int fd, std::string line);

#endif