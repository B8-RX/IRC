#ifndef SERVER_HPP
# define SERVER_HPP

#include <exception>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <signal.h>

// class Client;

class Server {
	public:
		Server(void);
		~Server(void);
		static void	sighandler(int signum);
		void	init(uint16_t port);
		void	run(void);
		void	closeSockets(void);
		class	SocketError : public std::exception {
			private:
				std::string		_message;
				std::string		_getErrnoMsg(int code);
			public:
				SocketError(int code) : _message(_getErrnoMsg(code)) {}
				~SocketError(void) throw() {}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
		};
		class	BindError : public std::exception {
			private:
				std::string		_message;
				std::string		_getErrnoMsg(int code);
			public:
				BindError(int code) : _message(_getErrnoMsg(code)) {}
				~BindError(void) throw() {}
				virtual const char* what() const throw() {
					return (_message.c_str());
				}
		};
	private:
		static bool					_signalReceived;
		// std::vector<Client>		_client_list;
		std::vector<struct pollfd>	_pollfd_list;
		uint16_t					_port;
		int							_clientSocket;
		sockaddr_in					_serverAddress;
		sockaddr_in					_remoteAddress;
		socklen_t					_remoteAddressLen;
		int							_serverSocket;
	};
#endif // !SERVER_HPP
