#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <string>
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
#include <queue>

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
	_pollfdList.push_back(pollfd);
	std::cout << "server initialized successfully.\n"
				<< "Port: " << _port << "\n";
}


void	Server::_handleNewClient(void) {
	Client 			client;
	int				clientFd;
	sockaddr_in		clientAdd;
	socklen_t		clientAddLen;
	struct	pollfd	newPollfd;
	
	
	clientAddLen = sizeof(clientAdd);
	clientFd = accept(_serverSocket, (struct sockaddr*)&clientAdd, &clientAddLen);
	if (clientFd == -1) {
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
	_clientList[clientFd] = client;
	_pollfdList.push_back(newPollfd);

	std::cout << "\nClient: [" << clientFd << "] connected!\n";
	std::cout << "address: [" << client.ipAddr << "]\n";
}


void	Server::_handleReceivedData(int clientFd) {
	std::cout << "\n\nFunction HandleReceivedData: fd = " << clientFd << "\n";
	char	buffer[BUFFER_SIZE];
	int		readBytes;

	memset(buffer, 0, sizeof(buffer));
	readBytes = recv(clientFd, buffer, sizeof(buffer), MSG_DONTWAIT);
	if (readBytes == 0) {
		//INFO end of line reached or client disconnected. cleanup...
		//TODO cleanup: remove the client from the channels where he is a member
		std::cout << "cleanup client [" << clientFd << "]\n";
		close(_clientList[clientFd].fd);
		_clientList.erase(clientFd);
		for (size_t i = 0; i < _pollfdList.size(); ++i) {
			if (_pollfdList[i].fd == clientFd) {
				_pollfdList.erase(_pollfdList.begin() + i);
				break;
			}
		}
		return ;
	}
	else if (readBytes == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
		// DATA NOT READY AND OPTION O_NONBLOCK IS ENABLED ON THE SOCKET (silently ignored)
			return ;
		}
		else
		{
			std::cerr << "recv: " + std::string(strerror(errno));
			return ;
		}
	}
	_clientList[clientFd].bufferIn.append(buffer, readBytes);
	
	struct s_Line				sLine;
	std::vector<std::string>	vLines;
	
	vLines = _splitCRLF(clientFd); // extract complete lines
	
	for (std::size_t i = 0; i < vLines.size(); ++i) {
		sLine = _parseLine(vLines[i]);
		_validateLine(sLine);
	}

	std::cout << "buffer_in: [" << _clientList[clientFd].bufferIn << "]\n";

	/* TODO: FRAMING */ //!done 
	/* TODO: PARSING */ //? in progress
	// TODO: VALIDATE COMBINAISON 
	// TODO: EXECUTE 
	// TODO: REPEAT 
}

bool	Server::_validateLine(const s_Line& sLine) const {
	std::cout << "raw line: [" << sLine.raw << "]\n";
	std::cout << "prefix: [" << (sLine.prefix.empty() ? "" : sLine.prefix) << "]\n";
	std::cout << "command: [" << (sLine.command.empty() ? "" : sLine.command) << "]\n";		
	for (std::size_t i = 0; i < sLine.params.size(); ++i) {
		std::cout << "param:[" << sLine.params[i] << "]\n";
	}		
	std::cout << "\n";
	return (true);
}

std::vector<std::string>	Server::_splitCRLF(int clientFd) {

	std::vector<std::string>	vLines;
	std::string					line;
	size_t						posCRLF = std::string::npos;
	Client*						client;

	client = &_clientList[clientFd];
	while ((posCRLF = client->bufferIn.find("\r\n")) != std::string::npos) {
			line = client->bufferIn.substr(0, posCRLF);
			vLines.push_back(line);
			client->bufferIn.erase(0, posCRLF + 2);
	}
	return (vLines);
}

std::string	Server::_spaceTrim(const std::string& str) const {
	std::string	trimmed = str.substr(0);
	std::size_t i = 0;

	for (; i < trimmed.size(); ++i) {
		if (trimmed[i] != ' ')
			break;
	}
	trimmed.erase(0, i);
	if (trimmed.empty())
		return (trimmed);
	for (i = (trimmed.size() - 1); i > 0; --i) {
		if (trimmed[i] != ' ') 
			break;
	}
	std::string	res = trimmed.substr(0, (i + 1));
	return (res);
}

Server::s_Line	Server::_parseLine(const std::string& line) {
	struct s_Line				sLine;
	if (line.empty())
		return (sLine);
	std::string					lineCpy = _spaceTrim(line.substr(0));
	
	sLine.raw = lineCpy;
	if (lineCpy[0] == ':') {
		for (std::size_t j = 0; j < lineCpy.size(); ++j) {
			if (lineCpy[j] == ' ' || j == (lineCpy.size() - 1)) {
				if (lineCpy[j] != ' ') {
					sLine.prefix = lineCpy.substr(1, j);
					return (sLine);
				}
				sLine.prefix = lineCpy.substr(0, j);
				while (lineCpy[j] == ' ')
					++j;
				lineCpy.erase(0, j);
				sLine.prefix.erase(0, 1);
				break;
			}
		}
	}
	for (std::size_t j = 0; j < lineCpy.size(); ++j) { // store command
		if (lineCpy[j] == ' ' || j == (lineCpy.size() - 1)) {
			if (lineCpy[j] != ' ') {
				sLine.command = lineCpy.substr(0, j + 1);
				return (sLine);
			}
			sLine.command = lineCpy.substr(0, j);
			while (lineCpy[j] == ' ')
				++j;
			lineCpy.erase(0, j);
			break;
		}
	}
	for (std::size_t i = 0; i < lineCpy.size(); ++i) {
		if (lineCpy[i] == ' ' || (i == lineCpy.size() - 1)) {
			if (lineCpy[i] != ' ' && lineCpy[0] != ':') {
				sLine.params.push_back(lineCpy.substr(0, i + 1));
				return (sLine);
			}
			if (lineCpy[0] == ':') {
				sLine.params.push_back(lineCpy.substr(1, lineCpy.size() - 1));
				return (sLine);
			}
			sLine.params.push_back(lineCpy.substr(0, i));
			while (lineCpy[i] == ' ')
				++i;
			lineCpy.erase(0, i);
			i = 0;
		}
	}
	return (sLine);
}

void	Server::_printClients(void) const {
	for (std::map<int, Client>::const_iterator it = _clientList.begin(); it != _clientList.end(); ++it)
		std::cout << "client [" << it->first << "] connected at address [" << it->second.ipAddr << "]\n";	
}

void	Server::run(void) {
	while (_signalReceived == false) {
		if ((poll(&_pollfdList[0], _pollfdList.size(), 0) == -1) && _signalReceived == false)
			throw (std::runtime_error("Error: poll()"));
		for (size_t i = 0; i < _pollfdList.size(); ++i) {
			if (_pollfdList[i].revents & POLLIN) {
				if (_pollfdList[i].fd == _serverSocket)
					_handleNewClient();
				else
					_handleReceivedData(_pollfdList[i].fd);
			}
		}
	}
	closeSockets();
}

void	Server::closeSockets(void) {
	std::map<int, Client>::iterator it = _clientList.begin();
	for (size_t i = 0; i < _clientList.size(); ++i, ++it) {
		std::cout << "cleanup client [" << it->second.fd << "]\n";
		close(it->second.fd);
	}
	for (size_t i  = 0; i < _channelList.size(); ++i) {
		std::cout << "cleanup channel [" << _channelList[i].fd << "]\n";
		close(_channelList[i].fd);
		_channelList.erase(i);
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
