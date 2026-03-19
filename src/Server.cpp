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
#include <algorithm>

bool Server::_signalReceived = false;

Server::Server(void) {}

Server::~Server(void) {}

void	Server::init(uint16_t port) {
	struct pollfd	pollfd;
	_port = port;
	
	//* create socket internet
	if ((_serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		throw (std::runtime_error("socket: Failed to create socket: " + std::string(strerror(errno))));
	}
	
	//* create socket structure
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(_port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;
	
	//* add option SO_REUSEADDR to bypass the timewait that block server address when it shutdown
	int on = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		throw (std::runtime_error("setsockopt: Failed to set option SO_REUSEADDR: " + std::string(strerror(errno))));
	}
	
	//* add option O_NONBLOCK to make socket asyncronous
	if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) == -1) {
		throw (std::runtime_error("fcntl: Failed to set option O_NONBLOCK: " + std::string(strerror(errno))));
	} 
	
	//* link address to socket
	if (bind(_serverSocket, (struct sockaddr*)&_serverAddress, sizeof(_serverAddress)) == -1)
		throw (std::runtime_error("bind: Failed to bind address on socket: " + std::string(strerror(errno))));
	
	//* make socket passive and wait for incoming connection
	if (listen(_serverSocket, 10) == -1) {
		if (errno == EOPNOTSUPP) {
			throw (std::runtime_error("listen: Failed to start listenning: " + std::string(strerror(errno))));
		} 
	}
	//* add server socket to pollfd list, for poll() who will loop on it
	pollfd.fd = _serverSocket;
	pollfd.events = POLLIN;
	pollfd.revents = 0;
	_pollfd_list.push_back(pollfd);
	std::cout << "server initialized successfully.\n"
				<< "Port: " << _port << "\n";
}


void	Server::_HandleNewClient(void) {
	Client 			client;
	int				clientFd;
	sockaddr_in		clientAdd;
	socklen_t		clientAddLen;
	struct	pollfd	newPollfd;
	
	
	clientAddLen = sizeof(clientAdd);
	if ((clientFd = accept(_serverSocket, (struct sockaddr*)&clientAdd, &clientAddLen)) == -1) {
		std::cerr << "accept: Failed to create and connect client socket: " + std::string(strerror(errno));
		return ;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "fcntl: Failed to add optin O_NONBLOCK to client socket: " + std::string(strerror(errno));
		return ;
	}
	newPollfd.fd = clientFd;
	newPollfd.events = POLLIN;
	newPollfd.revents = 0;
	client.fd = clientFd;
	client.ipAddr = inet_ntoa(clientAdd.sin_addr);
	client.connected = false;
	_client_list[clientFd] = client;
	_pollfd_list.push_back(newPollfd);

	std::cout << "Client " << clientFd << " connected!\n";
	std::cout << "port " << client.ipAddr << "\n";
}


void	Server::_HandleReceivedData(int clientSocket) {
	std::cout << "handleReceivedData: fd = " << clientSocket << "\n";
	char	buffer[BUFFER_SIZE];
	int		bytes;
	Client	client;
	
	client = _client_list[clientSocket];
	memset(buffer, 0, sizeof(buffer));
	bytes = recv(clientSocket, buffer, sizeof(buffer), MSG_DONTWAIT);
	if (bytes == 0) {
		// end of line reached or client disconnected.
		//TODO cleanup: remove the client from the channels where he is a member
		close(_client_list[clientSocket].fd);
		_client_list.erase(clientSocket);
		for (size_t i = 0; i < _pollfd_list.size(); ++i) {
			if (_pollfd_list[i].fd == clientSocket) {
				_pollfd_list.erase(_pollfd_list.begin() + i);
				break;
			}
		}
		std::cout << "cleanup client [" << clientSocket << "]\n";
		return ;
	}
	else if (bytes == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			std::cout << "recv: " + std::string(strerror(errno));
			return ;
		}
		else
		{
			std::cerr << "recv: " + std::string(strerror(errno));
			return ;
		}
	}
	if (buffer[0] == '\0'){
		std::cout << "buffer is empty\n";
	}
	client.buffer_in = buffer;
	// TODO: FRAMING /r/n and /n
	// TODO: PARSING 
	// TODO: VALIDATE COMBINAISON 
	// TODO: EXECUTE 
	// TODO: REPEAT 
	
	std::cout << "data received: buffer = |" << buffer << "|\n";
}

void	Server::_printClients(void) {
	for (std::map<int, Client>::iterator it = _client_list.begin(); it != _client_list.end(); ++it)
		std::cout << "client [" << it->first << "] connected at address [" << it->second.ipAddr << "]\n";	
}

void	Server::run(void) {
	while (_signalReceived == false) {
		if ((poll(&_pollfd_list[0], _pollfd_list.size(), 0) == -1) && _signalReceived == false)
			throw (std::runtime_error("Error: poll()"));
		for (size_t i = 0; i < _pollfd_list.size(); ++i) {
			if (_pollfd_list[i].revents & POLLIN) {
				if (_pollfd_list[i].fd == _serverSocket)
					_HandleNewClient();
				else
					_HandleReceivedData(_pollfd_list[i].fd);
			}
		}
	}
	closeSockets();
}

void	Server::closeSockets(void) {
	for (size_t i = 0; i < _client_list.size(); ++i) {
		std::cout << "cleanup client [" << _client_list[i].fd << "]\n";
		close(_client_list[i].fd);
		_client_list.erase(i);
	}
	if (_serverSocket != -1) {
		std::cout << "cleanup Sever [" << _serverSocket << "]\n";
		close(_serverSocket);
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