#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <netdb.h>

Server::Server(void) {}

Server::~Server(void) {
	close(_serverSocket);
}

void	Server::init(const std::string& dom, const std::string& tpe, int proto, unsigned int port) {
		_domain = (dom == "IPV6") ? AF_INET6 : AF_INET;
		_type = (tpe == "TCP") ? SOCK_STREAM : SOCK_DGRAM;
		_protocol = proto;

		;
		if ((_serverSocket = socket(_domain, _type, _protocol)) == -1) {
			throw SocketError(errno);
		}
		_serverAddress.sin_family = _domain;
		_serverAddress.sin_port = htons(port);
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

		std::cout << "server initialized successfully.\nDomain: "
					<< dom << "\nType: " << tpe 
					<< "\nProtocol: " 
					<< (_protocol == 0 ? "default" : "unknow") << "\n";
}

void	Server::run(void) {
		for (;;)
		{
			_clientSocket = accept(_serverSocket, (struct sockaddr*)&_remoteAddress, &_remoteAddressLen);
			char	buffer[1024] = {0};
			recv(_clientSocket, buffer, sizeof(buffer), 0);
			std::cout << "Connected in fd=" << _clientSocket << "\n";
			std::cout << "Protocol name: " << getprotobynumber(_protocol)->p_name << "\n";

			//  std::cout << " client connected from " << inet_ntoa(_remoteAddress.sin_addr) << ":" << ntohs(_remoteAddress.sin_port) << "\n";
			std::cout << "Message from client: " << buffer << "\n";
		}
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