#include "../../includes/Server.hpp"

bool Server::_parseCommand(const std::string &line, Command &command) const {
	std::string::size_type pos = 0;
	std::string::size_type start;

	command.name.clear();
	command.params.clear();
	command.hasTrailing = false;
	command.trailing.clear();
	while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
		pos++;
	if (pos == line.size())
		return false;
	start = pos;
	while (pos < line.size() && line[pos] != ' ' && line[pos] != '\t')
		pos++;
	command.name = line.substr(start, pos - start);
	for (std::string::size_type i = 0; i < command.name.size(); i++)
		command.name[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(command.name[i])));
	while (pos < line.size()) {
		while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
			pos++;
		if (pos == line.size())
			break;
		if (line[pos] == ':') {
			command.hasTrailing = true;
			command.trailing = line.substr(pos + 1);
			break;
		}
		start = pos;
		while (pos < line.size() && line[pos] != ' ' && line[pos] != '\t')
			pos++;
		command.params.push_back(line.substr(start, pos - start));
	}
	return !command.name.empty();
}

void Server::_initCommandHandlers() {
	_commandHandlers["JOIN"] = &Server::_handleJoin;
	_commandHandlers["PART"] = &Server::_handlePart;
	_commandHandlers["KICK"] = &Server::_handleKick;
	_commandHandlers["INVITE"] = &Server::_handleInvite;
	_commandHandlers["TOPIC"] = &Server::_handleTopic;
	_commandHandlers["MODE"] = &Server::_handleMode;
	_commandHandlers["PRIVMSG"] = &Server::_handleMsg;
}

bool Server::_dispatchCommand(int fd, const Command &command) {
	std::map<std::string, CommandHandler>::iterator it = _commandHandlers.find(command.name);

	if (it != _commandHandlers.end())
		return (this->*(it->second))(fd, command);
	_sendNumericReply(fd, ERR_UNKNOWNCOMMAND(command.name));
	return false;
}

bool Server::_checkRegistration(int fd, const Command &command) {
	if (command.name == "PASS")
		return true;
	if (!_clients[fd].getPassword()) {
		_sendNumericReply(fd, ERR_NOTREGISTERED());
		return false;
	}
	if (command.name != "NICK" && command.name != "USER"
		&& command.name != "HELP" && !_clients[fd].getAuth()) {
		_sendNumericReply(fd, ERR_NOTREGISTERED());
		return false;
	}
	return true;
}

bool Server::_dispatchRegistrationCommand(int fd, const Command &command) {
	if (command.name == "PASS")
		_handlePass(fd, command);
	else if (command.name == "NICK")
		_handleNick(fd, command);
	else if (command.name == "USER")
		_handleUser(fd, command);
	else if (command.name == "HELP")
		_handleHelp(fd);
	else
		return false;
	return true;
}

bool Server::_processCommand(int fd, const std::string &line) {
	Command command;

	if (!_parseCommand(line, command))
		return true;
	if (!_checkRegistration(fd, command))
		return true;
	if (_dispatchRegistrationCommand(fd, command)) {
		_tryAuthenticateClient(fd);
		return true;
	}
	_dispatchCommand(fd, command);
	return true;
}
