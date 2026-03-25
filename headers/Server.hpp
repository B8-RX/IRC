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
				std::string		_message;
			public:
				ErrorException(const std::string& message) : _message(message) {}
				virtual ~ErrorException() throw() {}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
		};
	private:
		static bool					_signal_received;
		std::map<int, Client>		_client_list; 
		std::map<int, Channel>		_channel_list;
		struct Line {
			std::string					raw;
			std::string					prefix; // (source) when message server-to-client (description about the message) may not be present 
			std::string					command;
			std::vector<std::string>	params; // strip ':'
			std::size_t					size; // use MAX_SIZE_MSG to check/ERR_INPUTTOOLONG
		};
		std::vector<Line>			_vec_line;
		std::vector<struct pollfd>	_pollfd_list;
		uint16_t					_port;
		sockaddr_in					_serverAddress;
		int							_serverSocket;
		void						_HandleNewClient(void);
		void						_HandleReceivedData(int client_socket);
		void						_makeCompleteLines(int client_socket);
		void						_printClients(void);
	};
#endif // !SERVER_HPP
