#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "ServerParsing.cpp"
#include "ServerCommands.cpp"
#include "ServerUtils.cpp"
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

Server::Server(uint16_t port, const std::string& password) : _serverName("irc.local"), _password(password), _passwordEnabled(!(_password.empty())), _port(port) {}

Server::~Server(void) {}

void	Server::init(void) {
	struct pollfd	pollfd;
	
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
		//?INFO: end of line reached or client disconnected. cleanup...
		//TODO: cleanup: remove the client from the channels where he is a member
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
		//?INFO: DATA IS NOT READY AND OPTION O_NONBLOCK IS ENABLED ON THE SOCKET (silently ignored)
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
	
	// process received data
	for (std::size_t i = 0; i < vLines.size(); ++i) {
		sLine = _parseLine(vLines[i]);
		_dispatchCommand(clientFd, sLine);
		_printLine(sLine);
	}
	if (_clientList.find(clientFd) != _clientList.end()) {
		_printClient(_clientList[clientFd]);
	}
}

bool	Server::_updateRegisteredState(int clientFd) {
	Client*	cli = &_clientList[clientFd];
	cli->setRegirstered((cli->getPassAccepted() || !_passwordEnabled) && cli->hasNick && cli->hasUser);
	return (cli->getRegirstered());
}

void	Server::_cleanupClient(int clientFd) {

	std::vector<std::string>	cliChannels = _clientList.find(clientFd)->second.getSubscribedChannels();
	
	// boucler sur la liste des channels dont le client est membre
	for (std::size_t i = 0; i < cliChannels.size(); ++i) {
		
		// recuperer le channel
		std::string			chanName = cliChannels[i];
		std::map<std::string, Channel>::iterator chanIt = _channelList.find(chanName);
		if (chanIt == _channelList.end()) {
			continue;
		} 
		Channel&			channel = chanIt->second;

		// retirer le client de la liste des membres du channel
		channel.removeMember(clientFd);
		
		// supprimer le channel si vide
		if (channel.empty()) {
			_channelList.erase(chanIt);
		}
	}
	
	// retirer la structure poll qui surveillait les events sur le socket du client
	for (std::size_t i = 0; i < _pollfdList.size(); ++i) {
		if (_pollfdList[i].fd == clientFd) {
			_pollfdList.erase(_pollfdList.begin() + i);
			break;
		}
	}
	
	// fermer le socket
	if (clientFd != -1) {
		std::cout << "cleanup client [" << clientFd << "]\n";
		close(clientFd);
	}
	
	// retirer le client de la liste des clients du serveur
	std::map<int, Client>::iterator cliIt = _clientList.find(clientFd);
	if (cliIt != _clientList.end()) {
		_clientList.erase(cliIt);
	}
	
}

void	Server::closeSockets(void) {
	std::map<int, Client>::iterator it = _clientList.begin();
	for (; it != _clientList.end(); ++it) {
		std::cout << "cleanup client [" << it->second.fd << "]\n";
		if (it->second.fd != -1) {
			close(it->second.fd);
		}
	}
	// std::map<std::string, Channel>::iterator itc = _channelList.begin();
	// for (; itc != _channelList.end(); ++itc) {
	// 	std::cout << "cleanup channel [" << itc->first << "]\n";
	// 	_channelList.erase(itc);
	// }
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


std::size_t	Server::clientCount(void) const {
	return (_clientList.size());
}

std::size_t	Server::channelCount(void) const {
	return (_channelList.size());
}

std::map<int, Client>::iterator				Server::getClient(int clientFd) {
	return (_clientList.find(clientFd));
}

std::map<int, Client>::iterator				Server::getUserByNick(const std::string& nick) {
	std::map<int, Client>::iterator it = _clientList.begin();
	for (; it != _clientList.end(); ++it) {
		if (it->second.getNickname() == nick) {
			return (it);
		}
	}
	return (_clientList.end());
}

std::map<std::string, Channel>::iterator	Server::getChannelIt(const std::string& name) {
	return (_channelList.find(name));
}

