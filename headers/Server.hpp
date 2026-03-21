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
#include <map>
#define BUFFER_SIZE 1024

// class Channel;

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
				std::string		_message;
			public:
				ErrorException(const std::string& message) : _message(message) {}
				virtual ~ErrorException() throw() {}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
		};
	private:
		static bool					_signalReceived;
		std::map<int, Client>		_client_list; 
		// std::map<int, Channel>		_channel_list;
		std::vector<struct pollfd>	_pollfd_list;
		uint16_t					_port;
		sockaddr_in					_serverAddress;
		int							_serverSocket;
		void						_HandleNewClient(void);
		void						_HandleReceivedData(int clientSocket);
		void						_printClients(void);
	};
#endif // !SERVER_HPP
