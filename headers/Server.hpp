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
#define BUFFER_SIZE 1024

class Server {
	public:
		Server(void);
		~Server(void);
		static void	sighandler(int signum);
		void	init(uint16_t port);
		void	run(void);
		void	closeSockets(void);
		class	ErrorException : public std::exception {
			private:
				std::string			_message;
			public:
				ErrorException(const std::string& message) : _message(message) {}
				virtual ~ErrorException() throw() {}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
		};
	private:
		struct s_Line {
			std::string					raw;
			std::string					prefix;
			std::string					command;
			std::vector<std::string>	params;
		};
		static bool					_signalReceived;
		std::map<int, Client>		_clientList; 
		std::map<int, Channel>		_channelList;
		std::vector<struct pollfd>	_pollfdList;
		uint16_t					_port;
		sockaddr_in					_serverAddress;
		int							_serverSocket;
		void						_handleNewClient(void);
		void						_handleReceivedData(int client_socket);
		std::vector<std::string>	_splitCRLF(int client_socket);
		void						_parseLine(const std::string& line, struct s_Line sLine);
		void						_printClients(void) const;
		std::string					_spaceTrim(const std::string& line) const;
	};
#endif // !SERVER_HPP
