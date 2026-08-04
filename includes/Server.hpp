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
#include "Client.hpp"
#include "Channel.hpp"

class Channel;

class Server {
	private:
		typedef bool (Server::*CommandHandler)(int, std::string);

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
		bool _processBuffer(int fd);
		bool _processCommand(int fd, std::string line);
		void _initCommandHandlers();
		bool _dispatchCommand(int fd, std::string command, std::string line);
		bool _checkPasswordRegistration(int fd, std::string command);
		bool _dispatchRegistrationCommand(int fd, std::string command, std::string param, std::string line);
		void _tryAuthenticateClient(int fd);
		bool _handlePass(int fd, std::string password);
		bool _handleNick(int fd, std::string nick);
		bool _handleUser(int fd, std::string line);
		void _handleHelp(int fd);
		bool _handleJoin(int fd, std::string line);
		bool _handleKick(int fd, std::string line);
		bool _handleTopic(int fd, std::string line);
		bool _handleMode(int fd, std::string line);
		bool _handleInvite(int fd, std::string line);
		void _handleMsg(int fd, std::string line);
		bool buildChan(std::map<std::string, std::string>::const_iterator channel, int fd);
		void broadcastToChannel(std::string chanName, std::string msg, int fd);
		std::string _clientPrefix(int fd);
		void _broadcastToChannel(const Channel &channel, const std::string &msg, int exceptFd = -1);
		void _broadcastChannelCommand(int fd, const Channel &channel, const std::string &command, const std::string &params, const std::string &trailing, int exceptFd = -1);
		bool _checkDupes(std::string type, std::string toCheck) const;
		bool _nickInUse(std::string nick, int currentFd) const;
		//Helpers / Errors
		int _userToFd(std::string username, int fd, std::string cmdErr);
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

void signalhHandler(int sig);
void _sendMsg(int fd, std::string msg);

#define ERR_NOSUCHNICK(nick)				(std::string("401 ") + (nick) + " :No such nick/channel\r\n")
#define ERR_NOSUCHSERVER(server)			(std::string("402 ") + (server) + " :No such server\r\n")
#define ERR_NOSUCHCHANNEL(chan)				(std::string("403 ") + (chan) + " :No such channel\r\n")
#define ERR_CANNOTSENDTOCHAN(chan)			(std::string("404 ") + (chan) + " :Cannot send to channel\r\n")
#define ERR_TOOMANYCHANNELS(chan)   		(std::string("405 ") + (chan) + " :You have joined too many channels\r\n")
#define ERR_WASNOSUCHNICK(nick)     		(std::string("406 ") + (nick) + " :There was no such nickname\r\n")
#define ERR_TOOMANYTARGETS(target)  		(std::string("407 ") + (target) + " :Too many recipients delivered\r\n")
#define ERR_NOSUCHSERVICE(service)  		(std::string("408 ") + (service) + " :No such service\r\n")
#define ERR_NOORIGIN()              		(std::string("409 :No origin specified\r\n"))
#define ERR_NORECIPIENT(cmd)        		(std::string("411 :No recipient given (") + (cmd) + ")\r\n")
#define ERR_NOTEXTTOSEND()          		(std::string("412 :No text to send\r\n"))
#define ERR_NOTOPLEVEL(mask)        		(std::string("413 ") + (mask) + " :No toplevel domain specified\r\n")
#define ERR_WILDTOPLEVEL(mask)      		(std::string("414 ") + (mask) + " :Wildcard in toplevel domain\r\n")
#define ERR_BADMASK(mask)           		(std::string("415 ") + (mask) + " :Bad Server/host mask\r\n")
#define ERR_UNKNOWNCOMMAND(cmd)     		(std::string("421 ") + (cmd) + " :Unknown command\r\n")
#define ERR_NOMOTD()                		(std::string("422 :MOTD File is missing\r\n"))
#define ERR_NOADMININFO(server)     		(std::string("423 ") + (server) + " :No administrative info available\r\n")
#define ERR_FILEERROR(op, file)     		(std::string("424 :File error doing ") + (op) + " on " + (file) + "\r\n")
#define ERR_NONICKNAMEGIVEN()       		(std::string("431 :No nickname given\r\n"))
#define ERR_ERRONEUSNICKNAME(nick)  		(std::string("432 ") + (nick) + " :Erroneous Nickname\r\n")
#define ERR_NICKNAMEINUSE(nick)     		(std::string("433 ") + (nick) + " :Nickname is already in use\r\n")
#define ERR_NICKCOLLISION(nick)     		(std::string("436 ") + (nick) + " :Nickname collision KILL\r\n")
#define ERR_UNAVAILRESOURCE(res)    		(std::string("437 ") + (res) + " :Nick/channel is temporarily unavailable\r\n")
#define ERR_USERNOTINCHANNEL(nick, chan)	(std::string("441 ") + (nick) + " " + (chan) + " :They aren't on that channel\r\n")
#define ERR_NOTONCHANNEL(chan)          	(std::string("442 ") + (chan) + " :You're not on that channel\r\n")
#define ERR_USERONCHANNEL(user, chan)   	(std::string("443 ") + (user) + " " + (chan) + " :is already on channel\r\n")
#define ERR_NOLOGIN(user)               	(std::string("444 ") + (user) + " :User not logged in\r\n")
#define ERR_SUMMONDISABLED()            	(std::string("445 :SUMMON has been disabled\r\n"))
#define ERR_USERSDISABLED()             	(std::string("446 :USERS has been disabled\r\n"))
#define ERR_NOTREGISTERED()             	(std::string("451 :You have not registered\r\n"))
#define ERR_NEEDMOREPARAMS(cmd)       		(std::string("461 ") + (cmd) + " :Not enough parameters\r\n")
#define ERR_ALREADYREGISTED()         		(std::string("462 :Unauthorized command (already registered)\r\n"))
#define ERR_NOPERMFORHOST()           		(std::string("463 :Your host isn't among the privileged\r\n"))
#define ERR_PASSWDMISMATCH()          		(std::string("464 :Password incorrect\r\n"))
#define ERR_YOUREBANNEDCREEP()        		(std::string("465 :You are banned from this server\r\n"))
#define ERR_KEYSET(chan)              		(std::string("467 ") + (chan) + " :Channel key already set\r\n")
#define ERR_CHANNELISFULL(chan)       		(std::string("471 ") + (chan) + " :Cannot join channel (+l)\r\n")
#define ERR_UNKNOWNMODE(c)            		(std::string("472 ") + (c) + " :is unknown mode char to me\r\n")
#define ERR_INVITEONLYCHAN(chan)      		(std::string("473 ") + (chan) + " :Cannot join channel (+i)\r\n")
#define ERR_BANNEDFROMCHAN(chan)      		(std::string("474 ") + (chan) + " :Cannot join channel (+b)\r\n")
#define ERR_BADCHANNELKEY(chan)       		(std::string("475 ") + (chan) + " :Cannot join channel (+k)\r\n")
#define ERR_BADCHANMASK(chan)         		(std::string("476 ") + (chan) + " :Bad Channel Mask\r\n")
#define ERR_NOCHANMODES(chan)         		(std::string("477 ") + (chan) + " :Channel doesn't support modes\r\n")
#define ERR_BANLISTFULL(chan)         		(std::string("478 ") + (chan) + " :Channel ban list is full\r\n")
#define ERR_NOPRIVILEGES()            		(std::string("481 :Permission Denied- You're not an IRC operator\r\n"))
#define ERR_CHANOPRIVSNEEDED(chan)    		(std::string("482 ") + (chan) + " :You're not channel operator\r\n")
#define ERR_CANTKILLSERVER()          		(std::string("483 :You can't kill a server!\r\n"))
#define ERR_RESTRICTED()              		(std::string("484 :Your connection is restricted!\r\n"))
#define ERR_UNIQOPRIVSNEEDED()        		(std::string("485 :You're not the original channel operator\r\n"))
#define ERR_NOOPERHOST()              		(std::string("491 :No O-lines for your host\r\n"))
#define ERR_UMODEUNKNOWNFLAG()        		(std::string("501 :Unknown MODE flag\r\n"))
#define ERR_USERSDONTMATCH()          		(std::string("502 :Cannot change mode for other users\r\n"))
#define RPL_NOTOPIC(nick, chan) (std::string("331 ") + (nick) + " " + (chan) + " :No topic is set\r\n")
#define RPL_TOPIC(nick, chan, topic) (std::string("332 ") + (nick) + " " + (chan) + " :" + (topic) + "\r\n")

#endif
