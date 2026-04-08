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
		Server(uint16_t port, const std::string& password);
		~Server(void);
		static void	sighandler(int signum);
		void	init(void);
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
		Channel&									getChannel(const std::string& name); // if not found the channel it will create one

	private:
		std::string						_serverName;
		std::string						_password;
		bool							_passwordEnabled;
		static bool						_signalReceived;

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

		// messaging
		void							_sendToClient(int clientFd, const std::string& message) const;
		void							_sendErrUnregistered(int clienFd, const std::string& nick);
		void							_sendErrUnknownCommand(int clientFd, const std::string& nick, const std::string& cmd);
		void							_sendErrNeedMoreParams(int clientFd, const std::string& nick, const std::string& cmd);
		void							_sendErrAlreadyRegistered(int clientFd, const std::string& nick);
		void							_sendErrPassMisMatch(int clienFd, const std::string& nick);
		void							_sendErrNoNickNameGiven(int clienFd, const std::string& nick);
		void							_sendErrOnUseNickName(int clienFd, const std::string& nick, const s_Line& line);
		void							_sendErrNickNameInUse(int clienFd, const std::string& nick, const s_Line& line);
		void							_sendErrBadChanMask(int clientFd, const std::string& nick, const std::string& channel) const;
		void							_sendErrNoSuchChannel(int clientFd, const std::string& nick, const std::string& channel) const;
		void							_sendErrNotOnChannel(int clientFd, const std::string& nick, const std::string& channel) const;
		
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
		bool							_handlePrivmsg(int clientFd, const s_Line& line);
		bool							_handlePart(int clientFd, const s_Line& line);
		bool							_handleQuit(int clientFd, const s_Line& line);
		bool							_handlePing(int clientFd, const s_Line& line);

		// state update	
		bool							_updateRegisteredState(int clientFd);


		// utils 	
		void    						_printLine(const Server::s_Line& sLine) const;
		void    						_printClient(const Client& client) const;
		void							_printChannel(const Channel& channel) const;
		bool    						_isValidNick(const std::string& nick) const;
		bool							_isUsedNick(std::map<int, Client>& ClientsList, const std::string& nick, int clientFd) const;
};
#endif // !SERVER_HPP
