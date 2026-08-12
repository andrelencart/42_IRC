/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dicosta- <dicosta-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 18:48:58 by dicosta-          #+#    #+#             */
/*   Updated: 2026/05/27 19:11:45 by dicosta-         ###   ########.fr       */
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
