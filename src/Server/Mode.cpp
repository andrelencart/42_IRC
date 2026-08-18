#include "../../includes/Server.hpp"

bool Server::_isValidChannelMode(char mode) const {
	return (mode == 'i' || mode == 't' || mode == 'k'
		|| mode == 'o' || mode == 'l');
}

ModeRequestResult Server::_prepareModeRequest(int fd, const Command &command,
	Channel **channel) {
	if (command.params.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("MODE"));
		return MODE_REQUEST_ERROR;
	}
	*channel = _getChannel(command.params[0]);
	if (*channel == NULL) {
		_sendNumericReply(fd, ERR_NOSUCHCHANNEL(command.params[0]));
		return MODE_REQUEST_ERROR;
	}
	if (command.params.size() == 1) {
		std::string modes("+");
		std::stringstream reply;

		if ((*channel)->isInviteOnly())
			modes += "i";
		if ((*channel)->isTopicRestricted())
			modes += "t";
		if ((*channel)->hasPass())
			modes += "k";
		if ((*channel)->getUserLimit() > 0)
			modes += "l";
		reply << (*channel)->getName() << " " << modes;
		if ((*channel)->hasPass())
			reply << " *";
		if ((*channel)->getUserLimit() > 0)
			reply << " " << (*channel)->getUserLimit();
		_sendNumericReply(fd, RPL_CHANNELMODEIS(reply.str()));
		return MODE_REQUEST_QUERY;
	}
	if (!(*channel)->isMember(fd)) {
		_sendNumericReply(fd, ERR_NOTONCHANNEL((*channel)->getName()));
		return MODE_REQUEST_ERROR;
	}
	if (!(*channel)->isOperator(fd)) {
		_sendNumericReply(fd, ERR_CHANOPRIVSNEEDED((*channel)->getName()));
		return MODE_REQUEST_ERROR;
	}
	return MODE_REQUEST_CHANGE;
}

bool Server::_parseModeChanges(int fd, const Command &command,
	std::vector<ModeChange> &changes) {
	const std::string &modeString = command.params[1];
	size_t parameterIndex = 2;
	char sign = '\0';
	bool errorSent = false;

	changes.clear();
	for (size_t i = 0; i < modeString.size(); i++) {
		if (modeString[i] == '+' || modeString[i] == '-') {
			sign = modeString[i];
			continue;
		}
		if (sign == '\0') {
			_sendNumericReply(fd,
				ERR_UNKNOWNMODE(std::string(1, modeString[i])));
			return false;
		}
		if (!_isValidChannelMode(modeString[i])) {
			_sendNumericReply(fd,
				ERR_UNKNOWNMODE(std::string(1, modeString[i])));
			errorSent = true;
			continue;
		}
		ModeChange change;
		change.sign = sign;
		change.mode = modeString[i];
		change.hasParameter = false;
		change.parameter.clear();
		bool needsParameter = (change.mode == 'o'
			|| (change.sign == '+' && change.mode == 'k')
			|| (change.sign == '+' && change.mode == 'l'));
		if (needsParameter) {
			if (parameterIndex >= command.params.size()) {
				_sendNumericReply(fd, ERR_NEEDMOREPARAMS("MODE"));
				errorSent = true;
				continue;
			}
			change.hasParameter = true;
			change.parameter = command.params[parameterIndex++];
		}
		changes.push_back(change);
	}
	if (changes.empty() && !errorSent)
		_sendNumericReply(fd, ERR_UNKNOWNMODE(modeString));
	return !changes.empty();
}

bool Server::_applyKeyMode(int fd, Channel *channel,
	const std::string &modeString, const std::string &modeParam) {
	if (modeString == "+k") {
		if (modeParam.empty()) {
			_sendNumericReply(fd, ERR_NEEDMOREPARAMS("MODE"));
			return false;
		}
		if (channel->hasPass()) {
			_sendNumericReply(fd, ERR_KEYSET(channel->getName()));
			return false;
		}
		channel->setPass(modeParam);
		return true;
	}
	if (!channel->hasPass())
		return false;
	channel->removePass();
	return true;
}

bool Server::_applyOperatorMode(int fd, Channel *channel,
	const std::string &channelName, const std::string &modeString,
	const std::string &modeParam) {
	if (modeParam.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("MODE"));
		return false;
	}
	int targetFd = _findClientFdByNick(modeParam);
	if (targetFd == -1) {
		_sendNumericReply(fd, ERR_NOSUCHNICK(modeParam));
		return false;
	}
	if (!channel->isMember(targetFd)) {
		_sendNumericReply(fd, ERR_USERNOTINCHANNEL(modeParam, channelName));
		return false;
	}
	if (modeString == "+o") {
		if (channel->isOperator(targetFd))
			return false;
		channel->addOperator(targetFd);
	}
	else {
		if (!channel->isOperator(targetFd))
			return false;
		channel->removeOperator(targetFd);
	}
	return true;
}

bool Server::_applyLimitMode(int fd, Channel *channel,
	const std::string &modeString, const std::string &modeParam) {
	if (modeString == "-l") {
		if (channel->getUserLimit() == 0)
			return false;
		channel->setUserLimit(0);
		return true;
	}
	if (modeParam.empty()) {
		_sendNumericReply(fd, ERR_NEEDMOREPARAMS("MODE"));
		return false;
	}
	for (size_t i = 0; i < modeParam.size(); i++) {
		if (!std::isdigit(static_cast<unsigned char>(modeParam[i]))) {
			_sendNumericReply(fd,
				ERR_INVALIDMODEPARAM(channel->getName(), "l", modeParam));
			return false;
		}
	}
	int limit = std::atoi(modeParam.c_str());
	if (limit <= 0) {
		_sendNumericReply(fd,
			ERR_INVALIDMODEPARAM(channel->getName(), "l", modeParam));
		return false;
	}
	if (channel->getUserLimit() == limit)
		return false;
	channel->setUserLimit(limit);
	return true;
}

bool Server::_applyMode(int fd, Channel *channel,
	const std::string &channelName, const std::string &modeString,
	const std::string &modeParam) {
	if (modeString == "+i") {
		if (channel->isInviteOnly())
			return false;
		channel->setInviteOnly(true);
	}
	else if (modeString == "-i") {
		if (!channel->isInviteOnly())
			return false;
		channel->setInviteOnly(false);
	}
	else if (modeString == "+t") {
		if (channel->isTopicRestricted())
			return false;
		channel->setTopicRestricted(true);
	}
	else if (modeString == "-t") {
		if (!channel->isTopicRestricted())
			return false;
		channel->setTopicRestricted(false);
	}
	else if (modeString == "+k" || modeString == "-k")
		return _applyKeyMode(fd, channel, modeString, modeParam);
	else if (modeString == "+o" || modeString == "-o")
		return _applyOperatorMode(fd, channel, channelName, modeString, modeParam);
	else if (modeString == "+l" || modeString == "-l")
		return _applyLimitMode(fd, channel, modeString, modeParam);
	else
		return false;
	return true;
}

bool Server::_executeModeChanges(int fd, Channel &channel,
	const std::vector<ModeChange> &changes) {
	std::string appliedModes;
	std::vector<std::string> appliedParameters;
	char appliedSign = '\0';

	for (size_t i = 0; i < changes.size(); i++) {
		std::string singleMode;
		singleMode += changes[i].sign;
		singleMode += changes[i].mode;
		if (!_applyMode(fd, &channel, channel.getName(), singleMode,
			changes[i].parameter))
			continue;
		if (changes[i].sign != appliedSign) {
			appliedModes += changes[i].sign;
			appliedSign = changes[i].sign;
		}
		appliedModes += changes[i].mode;
		if (changes[i].hasParameter)
			appliedParameters.push_back(changes[i].parameter);
	}
	if (appliedModes.empty())
		return true;
	std::string broadcastParameters = appliedModes;
	for (size_t i = 0; i < appliedParameters.size(); i++)
		broadcastParameters += " " + appliedParameters[i];
	_broadcastChannelCommand(fd, channel, "MODE", broadcastParameters, "",
		false);
	return true;
}

bool Server::_handleMode(int fd, const Command &command) {
	Channel *channel;
	ModeRequestResult result = _prepareModeRequest(fd, command, &channel);

	if (result == MODE_REQUEST_ERROR)
		return false;
	if (result == MODE_REQUEST_QUERY)
		return true;
	std::vector<ModeChange> changes;
	if (!_parseModeChanges(fd, command, changes))
		return false;
	return _executeModeChanges(fd, *channel, changes);
}
