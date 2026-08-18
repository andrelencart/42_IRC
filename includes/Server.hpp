#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <map>
#include <poll.h>
#include <csignal>
#include <cerrno>
#include <cctype>
#include <cstdlib>	
#include "Client.hpp"
#include "Channel.hpp"

class Channel;

struct Command {
	std::string name;
	std::vector<std::string> params;
	bool hasTrailing;
	std::string trailing;
};

struct ModeChange {
	char sign;
	char mode;
	bool hasParameter;
	std::string parameter;
};

enum ModeRequestResult {
	MODE_REQUEST_ERROR,
	MODE_REQUEST_QUERY,
	MODE_REQUEST_CHANGE
};

class Server {
	private:
		typedef bool (Server::*CommandHandler)(int, const Command &);

		int _port;
		std::string _password;
		std::string _serverName; //Added server name
		int _servFd;
		std::vector<struct pollfd> _fds;
		//std::map<int, std::string> _clientBuffers; // Commented, buffer has been migrated into _clients;
		//std::map<int, bool> _authenticated; // will be deleted after the migration to the Client Class
		//std::map<int, bool> _passverified; // will be deleted after the migration to the Client Class;
		//std::map<int, std::string> _nicknames; // will be deleted after the migration to the Client Class
		//std::map<int, std::string> _usernames; // will be deleted after the migration to the Client Class
		std::map<int, Client> _clients;
		std::map<std::string, Channel> _channels;
		std::map<std::string, CommandHandler> _commandHandlers;
		void _setupSocket();
		void _loopServer();
		void _acceptNewClient();
		bool _handleClient(int fd);
		bool _handleClientEvents(int fd, short revents);
		bool _flushClientOutput(int fd);
		void _setWritePolling(int fd, bool enabled);
		void _sendMsg(int fd, const std::string &message);
		void _sendNumericReply(int fd, const std::string &numeric,
			const std::string &parameters, const std::string &description,
			bool hasDescription = true);
		bool _processBuffer(int fd);
		bool _processCommand(int fd, const std::string &line);
		bool _parseCommand(const std::string &line, Command &command) const;
		void _initCommandHandlers();
		bool _dispatchCommand(int fd, const Command &command);
		bool _checkRegistration(int fd, const Command &command);
		bool _dispatchRegistrationCommand(int fd, const Command &command);
		void _tryAuthenticateClient(int fd);
		bool _handlePass(int fd, const Command &command);
		bool _handleNick(int fd, const Command &command);
		bool _handleUser(int fd, const Command &command);
		void _handleHelp(int fd);
		bool _handleJoin(int fd, const Command &command);
		bool _handlePart(int fd, const Command &command);
		bool _partChannel(int fd, const std::string &channelName, const std::string &partMessage);
		bool _handleKick(int fd, const Command &command);
		bool _kickFromChannel(int fd, Channel &channel, const std::string &targetNickname, const std::string &comment);
		bool _handleTopic(int fd, const Command &command);
		bool _handleMode(int fd, const Command &command);
		ModeRequestResult _prepareModeRequest(int fd, const Command &command, Channel **channel);
		bool _parseModeChanges(int fd, const Command &command, std::vector<ModeChange> &changes);
		bool _executeModeChanges(int fd, Channel &channel, const std::vector<ModeChange> &changes);
		bool _isValidChannelMode(char mode) const;
		bool _applyMode(int fd, Channel *channel, const std::string &channelName, const std::string &modeString, const std::string &modeParam);
		bool _applyKeyMode(int fd, Channel *channel, const std::string &modeString, const std::string &modeParam);
		bool _applyOperatorMode(int fd, Channel *channel, const std::string &channelName, const std::string &modeString, const std::string &modeParam);
		bool _applyLimitMode(int fd, Channel *channel, const std::string &modeString, const std::string &modeParam);
		int _findClientFdByNick(std::string nick) const;
		bool _handleInvite(int fd, const Command &command);
		bool _handleMsg(int fd, const Command &command);
		std::map<std::string, std::string> _buildChannelMap(std::string channel, std::string pass, int fd, int *check);
		bool _parseChannel(std::map<std::string, std::string>::const_iterator channel, int fd);
		bool buildChan(std::map<std::string, std::string>::const_iterator channel, int fd);
		void _sendJoinReplies(int fd, Channel &channel);
		std::string _buildNamesList(const Channel &channel);
		std::string _clientPrefix(int fd);
		void _broadcastToChannel(const Channel &channel, const std::string &msg, int exceptFd = -1);
		void _broadcastChannelCommand(int fd, const Channel &channel,
			const std::string &command, const std::string &params,
			const std::string &trailing, bool hasTrailing,
			int exceptFd = -1);
		bool _nickInUse(std::string nick, int currentFd) const;
		//Helpers / Errors
		void _removeClientFromChannel(Channel &channel, int fd, const std::string &quitMessage);
		void _removeClient(int fd);
		Channel *_getChannel(std::string channelName);

		Server(const Server& other);
		Server& operator=(const Server& other);

	public:
		Server();
		Server(int port, std::string password, std::string serverName); // Changed constructor to receive server name
		~Server();

		void start();

};

void signalHandler(int sig);

// Target and message errors
#define ERR_NOSUCHNICK(nick) \
	"401", (nick), "No such nick/channel"
#define ERR_NOSUCHSERVER(server) \
	"402", (server), "No such server"
#define ERR_NOSUCHCHANNEL(chan) \
	"403", (chan), "No such channel"
#define ERR_CANNOTSENDTOCHAN(chan) \
	"404", (chan), "Cannot send to channel"
#define ERR_TOOMANYCHANNELS(chan) \
	"405", (chan), "You have joined too many channels"
#define ERR_WASNOSUCHNICK(nick) \
	"406", (nick), "There was no such nickname"
#define ERR_TOOMANYTARGETS(target) \
	"407", (target), "Duplicate recipients. No message delivered"
#define ERR_NOSUCHSERVICE(service) \
	"408", (service), "No such service"
#define ERR_NOORIGIN() \
	"409", "", "No origin specified"
#define ERR_NORECIPIENT(cmd) \
	"411", "", (std::string("No recipient given (") + (cmd) + ")")
#define ERR_NOTEXTTOSEND() \
	"412", "", "No text to send"
#define ERR_NOTOPLEVEL(mask) \
	"413", (mask), "No toplevel domain specified"
#define ERR_WILDTOPLEVEL(mask) \
	"414", (mask), "Wildcard in toplevel domain"
#define ERR_BADMASK(mask) \
	"415", (mask), "Bad Server/host mask"

// Command and server errors
#define ERR_UNKNOWNCOMMAND(cmd) \
	"421", (cmd), "Unknown command"
#define ERR_NOMOTD() \
	"422", "", "MOTD File is missing"
#define ERR_NOADMININFO(server) \
	"423", (server), "No administrative info available"
#define ERR_FILEERROR(op, file) \
	"424", "", (std::string("File error doing ") + (op) + " on " + (file))

// Nickname and registration errors
#define ERR_NONICKNAMEGIVEN() \
	"431", "", "No nickname given"
#define ERR_ERRONEUSNICKNAME(nick) \
	"432", (nick), "Erroneous nickname"
#define ERR_NICKNAMEINUSE(nick) \
	"433", (nick), "Nickname is already in use"
#define ERR_NICKCOLLISION(nick) \
	"436", (nick), "Nickname collision KILL"
#define ERR_UNAVAILRESOURCE(resource) \
	"437", (resource), "Nick/channel is temporarily unavailable"
#define ERR_NOTREGISTERED() \
	"451", "", "You have not registered"
#define ERR_NEEDMOREPARAMS(cmd) \
	"461", (cmd), "Not enough parameters"
#define ERR_ALREADYREGISTED() \
	"462", "", "Unauthorized command (already registered)"
#define ERR_NOPERMFORHOST() \
	"463", "", "Your host isn't among the privileged"
#define ERR_PASSWDMISMATCH() \
	"464", "", "Password incorrect"
#define ERR_YOUREBANNEDCREEP() \
	"465", "", "You are banned from this server"

// Channel membership errors
#define ERR_USERNOTINCHANNEL(nick, chan) \
	"441", (std::string(nick) + " " + (chan)), "They aren't on that channel"
#define ERR_NOTONCHANNEL(chan) \
	"442", (chan), "You're not on that channel"
#define ERR_USERONCHANNEL(user, chan) \
	"443", (std::string(user) + " " + (chan)), "is already on channel"
#define ERR_NOLOGIN(user) \
	"444", (user), "User not logged in"
#define ERR_SUMMONDISABLED() \
	"445", "", "SUMMON has been disabled"
#define ERR_USERSDISABLED() \
	"446", "", "USERS has been disabled"

// Channel configuration errors
#define ERR_KEYSET(chan) \
	"467", (chan), "Channel key already set"
#define ERR_CHANNELISFULL(chan) \
	"471", (chan), "Cannot join channel (+l)"
#define ERR_UNKNOWNMODE(mode) \
	"472", (mode), "is unknown mode char to me"
#define ERR_INVITEONLYCHAN(chan) \
	"473", (chan), "Cannot join channel (+i)"
#define ERR_BANNEDFROMCHAN(chan) \
	"474", (chan), "Cannot join channel (+b)"
#define ERR_BADCHANNELKEY(chan) \
	"475", (chan), "Cannot join channel (+k)"
#define ERR_BADCHANMASK(chan) \
	"476", (chan), "Bad Channel Mask"
#define ERR_NOCHANMODES(chan) \
	"477", (chan), "Channel doesn't support modes"
#define ERR_BANLISTFULL(chan) \
	"478", (chan), "Channel ban list is full"

// Privilege and mode errors
#define ERR_NOPRIVILEGES() \
	"481", "", "Permission Denied- You're not an IRC operator"
#define ERR_CHANOPRIVSNEEDED(chan) \
	"482", (chan), "You're not channel operator"
#define ERR_CANTKILLSERVER() \
	"483", "", "You can't kill a server!"
#define ERR_RESTRICTED() \
	"484", "", "Your connection is restricted!"
#define ERR_UNIQOPRIVSNEEDED() \
	"485", "", "You're not the original channel operator"
#define ERR_NOOPERHOST() \
	"491", "", "No O-lines for your host"
#define ERR_UMODEUNKNOWNFLAG() \
	"501", "", "Unknown MODE flag"
#define ERR_USERSDONTMATCH() \
	"502", "", "Cannot change mode for other users"
#define ERR_INVALIDMODEPARAM(chan, mode, parameter) \
	"696", (std::string(chan) + " " + (mode) + " " + (parameter)), \
	"Invalid mode parameter"

// Successful numeric replies
#define RPL_WELCOME(message) \
	"001", "", (message)
#define RPL_CHANNELMODEIS(parameters) \
	"324", (parameters), "", false
#define RPL_NOTOPIC(chan) \
	"331", (chan), "No topic is set"
#define RPL_TOPIC(chan, topic) \
	"332", (chan), (topic)
#define RPL_INVITING(user, chan) \
	"341", (std::string(user) + " " + (chan)), "", false
#define RPL_NAMREPLY(symbol, chan, names) \
	"353", (std::string(symbol) + " " + (chan)), (names)
#define RPL_ENDOFNAMES(chan) \
	"366", (chan), "End of /NAMES list."

#endif
