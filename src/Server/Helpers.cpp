/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Helpers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dicosta- <dicosta-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 16:49:41 by dicosta-          #+#    #+#             */
/*   Updated: 2026/05/27 18:42:22 by dicosta-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"

void Server::_removeClient(int fd) {
	_clients.erase(fd);
	std::cout << "Client disconnected: fd " << fd << std::endl;

}

void Server::_sendMsg(int fd, std::string msg) {
	send(fd, msg.c_str(), msg.size(), 0);
}

