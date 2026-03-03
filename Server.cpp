#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cerrno>

Server::Server(void) {}

Server::~Server(void) {
	close(_serverSocket);
}

void	Server::init(const std::string& dom, const std::string& tpe, int proto, unsigned int port) {
		_domain = (dom == "IPV6") ? AF_INET6 : AF_INET;
		_type = (tpe == "TCP") ? SOCK_STREAM : SOCK_DGRAM;
		_protocol = proto;
		_serverSocket = socket(_domain, _type, _protocol);
		_serverAddress.sin_family = _domain;
		_serverAddress.sin_port = htons(port);
		_serverAddress.sin_addr.s_addr = INADDR_ANY;
		bind(_serverSocket, (struct sockaddr*)&_serverAddress, sizeof(_serverAddress));
		listen(_serverSocket, 10);
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
			std::cout << "Message from client: " << buffer << "\n";
		}
}

std::string	Server::SocketError::_getErrnoMsg(int code) {
	std::string	message;
	switch (code) {
	case EAFNOSUPPORT:
		message = "The implementation does not support the specified address family.";
		break;
	case EMFILE:
		message = "All file descriptors available to the process are currently open.";
		break;
	case ENFILE:
		message = "No more file descriptors are available for the system.";
		break;
	case EPROTONOSUPPORT:
		message = "The protocol is not supported by the address family, or the protocol is not supported by the implementation.";
		break;
	case EPROTOTYPE:
		message = "The socket type is not supported by the protocol.";
		break;
	case EACCES:
		message = "The process does not have appropriate privileges.";
		break;
	case ENOBUFS:
		message = "Insufficient resources were available in the system to perform the operation.";
		break;
	case ENOMEM:
		message = "Insufficient memory was available to fulfill the request.";
		break;
	}
	return (message);
}
