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
		bool _password;
		std::string _readBuffer;
		std::string _writeBuffer;
	public:

		// Constructors
		Client();
		Client(int fd);
		Client(const Client &other);

		// Operators
		Client& operator=(const Client &other);

		// Getters
		std::string	getNickname() const { return _nickname; };
		std::string	getUsername() const { return _username; };
		bool		getPassword() const { return _password; };
		bool		getAuth() const { return _Auth; };
		int			getClientFD() const { return _clientFD;  };
		std::string getReadBuffer() const { return _readBuffer; };
		std::string	getWriteBuffer() const { return _writeBuffer; };

		// Setters
		void	setNickname(std::string nickname){_nickname = nickname;};
		void	setUsername(std::string username){_username = username;};
		void	setPassword(bool check){_password = check;};
		void	setAuth(bool Auth){ _Auth = Auth; };

		//Others
		void appendReadBuffer(std::string toAppend);
};


#endif