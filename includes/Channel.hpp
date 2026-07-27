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
		std::string	_topic;
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
		std::string	getTopic() const;
		bool hasPass() const;
		bool isInviteOnly() const;
		bool isOperator(int fd) const;
		int	getUserLimit() const;
		bool isInvited(int fd) const;
		bool isFull() const;
		bool isMember(int fd) const;
		size_t	getMemberCount() const;
		const std::set<int>& getMembers() const;

		// Setters
		void setPass(std::string pass);
		void setTopic(std::string topic);
		void setInviteOnly(bool i);
		void setUserLimit(int limit);

		//Others
		void	addMember(int fd);
		void	removeMember(int fd);
		void	addOperator(int fd);
		void	removeOperator(int fd);
		void	invite(int fd);
		
};


#endif