/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dicosta- <dicosta-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 18:48:58 by dicosta-          #+#    #+#             */
/*   Updated: 2026/08/18 14:19:05 by dicosta-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Client.hpp"

Client::Client(): _clientFD(0), _Auth(false), _nickname(""), _username(""), _password(false), _closeAfterWrite(false){};

Client::Client(int fd): _clientFD(fd), _Auth(false), _nickname(""), _username(""), _password(false), _closeAfterWrite(false){};

Client::Client(const Client &other)
{
	*this = other;
};

Client& Client::operator=(const Client &other)
{
	if (this != &other)
	{
		_clientFD = other._clientFD;
		_Auth = other._Auth;
		_nickname = other._nickname;
		_password = other._password;
		_username = other._username;
		_readBuffer = other._readBuffer;
		_writeBuffer = other._writeBuffer;
		_closeAfterWrite = other._closeAfterWrite;
	}
	return (*this);
};

// Getters

std::string Client::getNickname() const
{
	return _nickname;
}

std::string Client::getUsername() const
{
	return _username;
}

bool Client::getPassword() const
{
	return _password;
}

bool Client::getAuth() const
{
	return _Auth;
}

int Client::getClientFD() const
{
	return _clientFD;
}

std::string Client::getReadBuffer() const
{
	return _readBuffer;
}

const std::string &Client::getWriteBuffer() const
{
	return _writeBuffer;
}

bool Client::getCloseAfterWrite() const
{
	return _closeAfterWrite;
}

// Setters

void Client::setNickname(std::string nickname)
{
	_nickname = nickname;
}

void Client::setUsername(std::string username)
{
	_username = username;
}

void Client::setPassword(bool check)
{
	_password = check;
}

void Client::setAuth(bool Auth)
{
	_Auth = Auth;
}


// Others 

void Client::setCloseAfterWrite(bool closeAfterWrite)
{
	_closeAfterWrite = closeAfterWrite;
}

void Client::appendReadBuffer(std::string toAppend)
{
	_readBuffer += toAppend;
};

void Client::eraseBuffer(size_t pos)
{
	_readBuffer.erase(0, pos + 2);
};

void Client::appendWriteBuffer(const std::string &message)
{
	_writeBuffer += message;
};

void Client::eraseWriteBuffer(size_t bytes)
{
	_writeBuffer.erase(0, bytes);
};


std::string Client::getLowerCaseNickname() const
{
	std::string lowerNickname = _nickname;

	for (unsigned int i = 0; i < lowerNickname.size(); i++)
		lowerNickname[i] = tolower(lowerNickname[i]);
	return (lowerNickname);
};