#include "Server.hpp"
#include "Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>

bool Server::_signalReceived = false;

Server::Server(void) {}

Server::~Server(void) {
	closeSockets();
}

void	Server::init(uint16_t port) {
	struct pollfd	pollfd;
	_port = port;
	//* create socket internet
	if ((_serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		throw (ErrorException("Failed to create socket: " + std::string(strerror(errno))));
	}
	//* create socket structure
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(_port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;
	//* add option SO_REUSEADDR to bypass the timewait that block server address when it shutdown
	int on = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		throw (std::runtime_error("Error: setsockopt failed to set option SO_REUSEADDR on socket."));
	}
	//* add option O_NONBLOCK to make socket asyncronous
	if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) == -1) {
		throw (std::runtime_error("Error: fcntl failed to set option O_NONBLOCK on socket."));
	} 
	//* link address to socket
	if (bind(_serverSocket, (struct sockaddr*)&_serverAddress, sizeof(_serverAddress)) == -1) {
		throw (ErrorException("Failed to bind socket: " + std::string(strerror(errno))));
	}
	//* make socket passive and wait for incoming connection
	if (listen(_serverSocket, 10) == -1) {
		if (errno == EOPNOTSUPP) {
			throw (ErrorException("Failed to listen on socket: " + std::string(strerror(errno))));
		} else {
			throw (std::runtime_error("Failed to listen on socket\n"));
		}
	}
	//* add server socket to pollfd list, for poll() who will loop on it
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

void	Server::HandleNewClient(void) {
	//TODO create new object Client, create a socket and push objet to Client list 
}

void	Server::HandleReceivedData(int clientSocket) {
	std::cout << "handleReceivedData: fd = " << "fd" << "\n";
	//TODO FRAMING, PARSING, EXECUTION, RESPONSE"
}

void	Server::run(void) {
 while (_signalReceived == false) {
	for (size_t i = 0; i < _pollfd_list.size(); ++i) {
		if (poll(&_pollfd_list[0], _pollfd_list.size(), 0) == -1 && _signalReceived == false)
			throw (std::runtime_error("Error: poll()"));
		if (_pollfd_list[i].revents & POLLIN) {
			if (_pollfd_list[i].fd == _serverSocket)
				HandleNewClient();
			else
				HandleReceivedData(_pollfd_list[i].fd);
		}
	}
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
}