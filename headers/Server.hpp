#ifndef SERVER_HPP
# define SERVER_HPP

#include <exception>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <signal.h>
#include "Client.hpp"
#include "Channel.hpp"
#include <map>
#include <utility>

#define BUFFER_SIZE 1024

class Server {
	public:
		Server(void);
		~Server(void);
		static void	sighandler(int signum);
		void	init(uint16_t port, const std::string& password);
		void	run(void);
		void	closeSockets(void);
		class	ErrorException : public std::exception {
			private:
				std::string				_message;
			public:
				ErrorException(const std::string& message) : _message(message) {}
				virtual ~ErrorException() throw() {}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
		};
		struct s_Line {
			std::string					raw;
			std::string					prefix;
			std::string					command;
			std::vector<std::string>	params;
		};

		std::size_t									clientCount(void) const;
		std::size_t									channelCount(void) const;
		std::map<int, Client>::iterator				getClient(int clienFd);
		std::map<std::string, Channel>::iterator	getChannel(const std::string& name);

	private:
		static bool						_signalReceived;
		std::string						_password;
		bool							_passwordEnabled;

		// structures metier	
		std::map<int, Client>			_clientList; 
		std::map<std::string, Channel>	_channelList;

		// reseau	
		uint16_t						_port;
		std::vector<struct pollfd>		_pollfdList;
		sockaddr_in						_serverAddress;
		int								_serverSocket;
		void							_handleNewClient(void);
		void							_handleReceivedData(int clientFd);
		void							_cleanupClient(int clinetFd);

		// helper framing/parsing	
		std::vector<std::string>		_splitCRLF(int clientFd);
		std::string						_spaceTrim(const std::string& line) const;

		// parsing	
		struct s_Line					_parseLine(const std::string& line);
		std::string						_handlePrefix(std::string& line);
		std::string						_handleCommand(std::string& line);
		std::vector<std::string>		_handleParams(std::string& line);

		// validation/execution	
		bool							_checkAndExecuteLine(int clientFd, const s_Line& sLine);

		bool							_handlePass(int clientFd, const s_Line& line);
		bool							_handleNick(int clientFd, const s_Line& line);
		bool							_handleUser(int clientFd, const s_Line& line);
		bool							_handleJoin(int clientFd, const s_Line& line);
		void							_handlePrivmsg(int clientFd, const s_Line& line) const;
		void							_handlePart(int clientFd, const s_Line& line) const;
		void							_handleQuit(int clientFd, const s_Line& line) const;

		// state update	
		bool							_updateRegisteredState(int clientFd);


		// utils 	
		void    						_printLine(const Server::s_Line& sLine);
		void    						_printClient(const Client& client);
		bool    						_isValidNick(const std::string& nick);
		bool							_isUsedNick(std::map<int, Client>& ClientsList, const std::string& nick, int clientFd);
};
#endif // !SERVER_HPP
