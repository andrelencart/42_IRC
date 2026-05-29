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

Client::Client(): _clientFD(0), _Auth(0), _nickname(""), _username(""), _password(""){};

Client::Client(int fd): _clientFD(fd), _Auth(0), _nickname(""), _username(""), _password(""){};

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
		//teste
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
