#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <poll.h>

class Client
{
	private:

		int			_clientFD;
		bool		_Auth;
		std::string _nickname;
		std::string _username;
		bool 		_password;
		std::string _readBuffer;
		std::string _writeBuffer;
		bool		_closeAfterWrite;
		
	public:

		// Constructors
		Client();
		Client(int fd);
		Client(const Client &other);

		// Operators
		Client& operator=(const Client &other);

		// Getters
		std::string	getNickname() const;
		std::string	getUsername() const;
		bool		getPassword() const;
		bool		getAuth() const;
		int			getClientFD() const;
		std::string getReadBuffer() const;
		const std::string &getWriteBuffer() const;
		bool		getCloseAfterWrite() const;

		// Setters
		void	setNickname(std::string nickname);
		void	setUsername(std::string username);
		void	setPassword(bool check);
		void	setAuth(bool Auth);
		void	setCloseAfterWrite(bool closeAfterWrite);

		//Others
		void	appendReadBuffer(std::string toAppend);
		void	eraseBuffer(size_t pos);
		void	appendWriteBuffer(const std::string &message);
		void	eraseWriteBuffer(size_t bytes);
};


#endif
