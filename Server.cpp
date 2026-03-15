#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <netdb.h>
#include <stdlib.h>

bool Server::_signalReceived = false;

Server::Server(void) {}

Server::~Server(void) {
	closeSockets();
}

void	Server::init(uint16_t port) {
	struct pollfd	pollfd;
	_port = port;
	if ((_serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		throw ErrorException("Failed to create socket: " + std::string(strerror(errno)));
	}
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(_port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverSocket, (struct sockaddr*)&_serverAddress, sizeof(_serverAddress)) == -1) {
		throw ErrorException("Failed to bind socket: " + std::string(strerror(errno)));
	}
	
	if (listen(_serverSocket, 10) == -1) {
		if (errno == EOPNOTSUPP) {
			throw ErrorException("Failed to listen on socket: " + std::string(strerror(errno)));
		} else {
			throw std::runtime_error("Failed to listen on socket\n");
		}
	}
	pollfd.fd = _serverSocket;
	pollfd.events = POLLIN;
	_pollfd_list.push_back(pollfd);
	std::cout << "server initialized successfully.\n"
				<< "Port: " << _port << "\n";
}

void	Server::closeSockets(void) {
	for (size_t i = 0; i < _pollfd_list.size(); ++i) {
		std::cout << "Closing socket fd: " << _pollfd_list[i].fd << "\n";
		close(_pollfd_list[i].fd);
	}
}

void	Server::run(void) {
 // TODO: implement the main server loop using poll to handle multiple clients
 while (_signalReceived == false) {
// TODO after catching an event, if it's a new connection, accept it and add the new client socket to the pollfd list
// TODO if it's an existing client socket, read the data, parse the request, execute the appropriate handler, and send the response back to the client:
//? FRAMING, PARSING, EXECUTION, RESPONSE"

 }
}

void Server::sighandler(int signum) {
	std::cout << "Signal received: ";
	if (signum == SIGINT)
		std::cout << "Ctrl+C\n";
	else if (signum == SIGQUIT)
		std::cout << "Ctrl+\\\n";
	else
		std::cout << "Unknown signal: " << signum << "\n";
	_signalReceived = true;
	// exit(0);
}