#include "../../includes/Channel.hpp"

Channel::Channel() : _name("#default"), _hasPass(false), _userLimit(-1), _inviteOnly(false) {};

Channel::Channel(std::string channel) : _name(channel), _hasPass(false), _userLimit(-1), _inviteOnly(false){};

Channel::Channel(const Channel &other)
{
	*this = other;
}

Channel &Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		_name = other._name;
		_pass = other._pass;
		_hasPass = other._hasPass;
		_userLimit = other._userLimit;
		_inviteOnly = other._inviteOnly;
		_members = other._members;
		_operators = other._operators;
		_invited = other._invited;
		return (*this);
	}
	return (*this);
}

std::string	Channel::getName() const{
	return _name;
}

std::string	Channel::getPass() const{
	return _pass;
}

bool Channel::hasPass() const{
	return _hasPass;
}

bool Channel::getInviteOnly() const{
	return _inviteOnly;
}

int	Channel::getUserLimit() const{
	return _userLimit;
}

size_t	Channel::getMemberCount() const{
	return _members.size();
}

const std::set<int>& Channel::getMembers() const{
	return _members;
}

void Channel::setPass(std::string pass){
	_hasPass = true;
	_pass = pass;
}

void Channel::setInviteOnly(bool i){
	_inviteOnly = i;
}

void Channel::setUserLimit(int limit){
	_userLimit = limit;
}

void	Channel::addMember(int fd){
	if(_members.find(fd) == _members.end())
		_members.insert(fd);
}

bool	Channel::isInvited(int fd) const{
	if(_members.find(fd) != _members.end())
		return true;
	return false;
}

bool	Channel::isFull() const{
	if (getMemberCount() >= _userLimit)
		return true;
	return false;
}

void _sendMsg2(int fd, std::string msg)
{
	send(fd, msg.c_str(), msg.size(), 0);
}