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
		throw SocketError(errno);
	}
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(_port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverSocket, (struct sockaddr*)&_serverAddress, sizeof(_serverAddress)) == -1) {
		throw BindError(errno);
	}
	
	if (listen(_serverSocket, 10) == -1) {
		if (errno == EOPNOTSUPP) {
			throw SocketError(errno);
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

std::string	Server::SocketError::_getErrnoMsg(int code) {
	std::string	message;
	switch (code) {
	case EAFNOSUPPORT:
	message = "SocketError: The implementation does not support the specified address family.";
	break;
	case EMFILE:
		message = "SocketError: All file descriptors available to the process are currently open.";
		break;
	case ENFILE:
		message = "SocketError: No more file descriptors are available for the system.";
		break;
	case EPROTONOSUPPORT:
		message = "SocketError: The protocol is not supported by the address family, or the protocol is not supported by the implementation.";
		break;
	case EPROTOTYPE:
		message = "SocketError: The socket type is not supported by the protocol.";
		break;
	case EACCES:
		message = "SocketError: The process does not have appropriate privileges.";
		break;
	case ENOBUFS:
		message = "SocketError: Insufficient resources were available in the system to perform the operation.";
		break;
	case ENOMEM:
		message = "SocketError: Insufficient memory was available to fulfill the request.";
		break;
	default:
		message = "SocketError: An unknown error occurred.";
		break;
	}
	return (message);
}
std::string	Server::BindError::_getErrnoMsg(int code) {
	std::string	message;
	switch (code) {
		case EACCES:
			message = "BindError: The process does not have appropriate privileges.";
			break;
		case EADDRINUSE:
			message = "BindError: The specified address is already in use.";
			break;
		case EBADF:
			message = "BindError: The socket argument is not a valid file descriptor.";
			break;
		case EINVAL:
			message = "BindError: The socket is already bound to an address.";
			break;
		case EADDRNOTAVAIL:
			message = "BindError: The specified address is not available from the local machine.";
			break;
		case EFAULT:
			message = "BindError: The socket structure address is outside the user's address space.";
			break;
		case ELOOP:
			message = "BindError: The socket is already bound to an address.";
			break;
		case ENAMETOOLONG:
			message = "BindError: The pathname is too long.";
			break;
		case ENOENT:
			message = "BindError: The file does not exist.";
			break;
		case ENOTDIR:
			message = "BindError: A component of the path prefix is not a directory.";
			break;
		default:
			message = "BindError: An unknown error occurred.";
			break;
	}
	return (message);
}