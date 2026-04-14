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

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

#define BUFFER_SIZE 1024

class Server {
	public:
		Server(uint16_t port, const std::string& password);
		~Server(void);
		void	init(void);
		void	run(void);
		
		std::size_t									clientCount(void) const;
		std::size_t									channelCount(void) const;
		Client*										getClient(int clienFd);
		Client*										getClientByNick(const std::string& nick);
		Channel*									getChannel(const std::string& name);
		struct s_Line {
			std::string					raw;
			std::string					prefix;
			std::string					command;
			std::vector<std::string>	params;
		};

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
		void							_cleanupClient(const Client& cli);
		void							_closeServer(void);

		// messaging
		void							_sendToClient(int clientFd, const std::string& message) const;

		void							_sendErrNeedMoreParams(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const;
		void							_sendErrAlreadyRegistered(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendErrNoNickNameGiven(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendErrOnUseNickName(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetNick) const;
		void							_sendErrNickNameInUse(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetNick) const;
		void							_sendErrBadChanMask(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrUnknownCommand(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const;
		void							_sendErrUnregistered(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendErrPassMisMatch(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendErrNotOnChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrNoSuchChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrNoRecipient(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const;
		void							_sendErrNoTextToSend(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendErrNoSuchNick(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetName) const;
		void							_sendErrCannotSendToChan(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrChaNoPrivsNeeded(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrUserOnChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrChanIsFull(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrBadChannelKey(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendErrInviteOnlyChan(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		
		
		void							_sendRplInviteList(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendEndOfInviteList(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const;
		void							_sendRplInviting(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendRplNoTopic(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const;
		void							_sendRplTopic(Client& cli, const std::string& nick, const s_Line& sline, const std::string& topic) const;
		void							_sendRplWhoTime(Client& cli, const std::string& nick, const s_Line& sline, const Channel::s_Topic& sTopic) const;
		void							_sendRplChannelModeIs(Client& cli, const std::string& nick, const s_Line& sline, const std::string& modeStringAndParams) const;
		void							_sendRplCreationTime(Client& cli, const std::string& nick, const s_Line& sline, const time_t& creationTime) const;
		

		
		// helper framing/parsing	
		std::vector<std::string>		_splitCRLF(int clientFd);
		std::string						_spaceTrim(const std::string& sline) const;

		// parsing	
		struct s_Line					_parseLine(const std::string& sline);
		std::string						_handlePrefix(std::string& sline);
		std::string						_handleCommand(std::string& sline);
		std::vector<std::string>		_handleParams(std::string& sline);

		// validation/execution	
		bool							_dispatchCommand(int clientFd, s_Line& sLine);

		bool							_handlePass(Client& cli, const s_Line& sline);
		bool							_handleNick(Client& cli, const s_Line& sline);
		bool							_handleUser(Client& cli, const s_Line& sline);
		bool							_handleJoin(Client& cli, const s_Line& sline);
		bool							_handlePart(Client& cli, const s_Line& sline);
		bool							_handleQuit(Client& cli, const s_Line& sline);
		bool							_handlePing(Client& cli, const s_Line& sline);
		bool							_handlePrivmsg(Client& cli, const s_Line& sline);
		bool							_handleKick(Client& cli, const s_Line& sline);
		bool							_handleInvite(Client& cli, s_Line& sline);
		bool							_handleTopic(Client& cli, s_Line& sline);
		bool							_handleMode(Client& cli, s_Line& sline);

		// state update	
		bool							_updateRegisteredState(int clientFd);
		
		// utils 	
		std::string						_generateLogInfo(const s_Line& sline, const Client& cli, const std::string& info) const;
		void							_printLogServer(const std::string& type, const Client& cli, const s_Line& sline, const std::string& info) const;
		void							_printServerInfo(void) const;
		void    						_printLine(const Server::s_Line& sLine) const;
		void    						_printClient(const Client& client) const;
		void							_printChannel(Channel& channel) const;
		bool    						_isValidNick(const std::string& nick) const;
		bool							_isUsedNick(std::map<int, Client>& ClientsList, const std::string& nick, int clientFd) const;
		bool							_isValidChannelName(const std::string& name) const;
	public:
		static void	sighandler(int signum);
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
};
#endif // !SERVER_HPP
