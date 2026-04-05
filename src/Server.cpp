#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
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

void	Server::init(uint16_t port, const std::string& password) {
	struct pollfd	pollfd;
	_port = port;

	//* set password state
	_password = password;
	_passwordEnabled = !(_password.empty());

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
	
	for (std::size_t i = 0; i < vLines.size(); ++i) {
		sLine = _parseLine(vLines[i]);
		_checkAndExecuteLine(clientFd, sLine);
		printLine(sLine);
	}
	printClient(_clientList[clientFd]);
}

bool	Server::_checkAndExecuteLine(int clientFd, const s_Line& sLine) { // change name to checkAndExecuteLine
	
	// TODO check if command is known
	// TODO check if params are good
	// TODO update registration
	
	bool	isValid = false;	
	
	if (sLine.command == "PASS")		
		isValid = _handlePass(clientFd, sLine);
	else if (sLine.command == "NICK")
		isValid = _handleNick(clientFd, sLine);
	else if (sLine.command == "USER")
		isValid = _handleUser(clientFd, sLine);
	else {
		std::cout << "send error: ERR_UNKNOWNCOMMAND (421)\n";
		isValid = false;
	}	
	return (isValid);
}

	/* TODO: FRAMING */ //!done 
	/* TODO: PARSING */ //!done
	/* TODO: VALIDATE COMBINAISON */ //? in progress
	// TODO: EXECUTE 
	// TODO: REPEAT 

	/*
		COMMANDS ARRAY

			 PASS
	- nb params = 1
	- pre-register allowed = yes

			 NICK
	- nb params = 1
	- pre-register allowed = yes

			 USER
	- nb params = 4
	- pre-register allowed = yes

				QUIT
	- nb params = 0
	- pre-register allowed = yes

	/////////////////////////////////////////////////////////

			 JOIN
	- nb params = 1
	- pre-register allowed = no


			 PRIVMSG
	- nb params = 2
	- pre-register allowed = no

			 PART
	- nb params = 1
	- pre-register allowed = no

	*/

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

std::string	Server::_handlePrefix(std::string& line) {
	std::string prefix;

	if (line[0] == ':') {
		for (std::size_t j = 0; j < line.size(); ++j) {
			if (line[j] == ' ' || j == (line.size() - 1)) {
				if (line[j] != ' ') {
					prefix = line.substr(0, j + 1);
					line.erase(0, j + 1);
					prefix.erase(0, 1);
					break ;
				}
				prefix = line.substr(0, j);
				while (line[j] == ' ')
					++j;
				line.erase(0, j);
				prefix.erase(0, 1);
				break ;
			}
		}
	}
	return (prefix);	
}

std::string	Server::_handleCommand(std::string& line) {
	std::string command;

	while (!line.empty()) {
		int start = 0;

		while (line[start] == ' ')
			start++;
		if (start > 0) {
			line.erase(0, start);
			break;
		}
		if (line.empty())
			break;
		
		std::size_t	pos = line.find(' ');
		if (pos == std::string::npos) {
			command = line.substr();
			line.erase(0, command.size());
			break;
		}
		command = line.substr(0, pos);
		line.erase(0, command.size());
	}

	// for (std::size_t j = 0; j < line.size(); ++j) { // store command
	// 	if (line[j] == ' ' || j == (line.size() - 1)) {
	// 		if (line[j] != ' ') {
	// 			command = line.substr(0, j + 1);
	// 			line.erase(0, j + 1);
	// 			break ;
	// 		}
	// 		command = line.substr(0, j);
	// 		while (line[j] == ' ')
	// 			++j;
	// 		line.erase(0, j);
	// 		break ;
	// 	}
	// }
	return (command);	
}

std::vector<std::string>	Server::_handleParams(std::string& line) {
	std::vector<std::string>	params;

	while (!line.empty()) {
		int start = 0;

		while (line[start] == ' ')
			start++;
		
		if (start > 0)
			line.erase(0, start);
		
		if (line.empty())
			break ;

		if (line[0] == ':') {
			params.push_back(line.substr(1));
			break ;
		}

		std::size_t pos = line.find(' ');
		if (pos == std::string::npos) {
			params.push_back(line.substr(0, line.size()));
			break ;
		}

		params.push_back(line.substr(0, pos));
		line.erase(0, pos);
	}
	return (params);
}


Server::s_Line	Server::_parseLine(const std::string& line) {
	struct s_Line				sLine;
	if (line.empty())
		return (sLine);
	std::string					lineCpy = _spaceTrim(line.substr(0));
	
	sLine.raw = lineCpy;
	sLine.prefix = _handlePrefix(lineCpy);
	sLine.command = _handleCommand(lineCpy);
	sLine.params = _handleParams(lineCpy);
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

bool	Server::_updateRegisteredState(int clientFd) {
	Client*	cli = &_clientList[clientFd];
	cli->setRegirstered((cli->getPassAccepted() || !_passwordEnabled) && cli->hasNick && cli->hasUser);
	return (cli->getRegirstered());
}

bool	Server::_handlePass(int clientFd, const s_Line& line) {
	Client*	cli = &_clientList[clientFd];
	
	if (line.params.empty()) {
		std::cout << "send an error:  ERR_NEEDMOREPARAMS (461) \n";
		return (false); // IGNORE AND CONTINUE
	}
	if (cli->getRegirstered()) {
		std::cout << "send an error: ERR_ALREADYREGISTERED (462)\n";
		return (false);
	}
	if (line.params[0] == _password || !_passwordEnabled) {
		std::cout << "password accepted\n";
		cli->setPassAccepted(true);
	}
	else {
		std::cout << "send an error: ERR_PASSWDMISMATCH (464)\n"; 
		cli->setPassAccepted(false);
		return (false);
	}
	_updateRegisteredState(clientFd);
	return (true);
}


bool	Server::_handleNick(int clientFd, const s_Line& line) {
	std::cout << "handle nick command\n";

	if (line.params.empty()) {
		std::cout << "send error:   ERR_NONICKNAMEGIVEN (431)  \n";
		return (false);
	}

	if (isValidNick(line.params[0]) == false)
		return (std::cout << "send error: ERR_ERRONEUSNICKNAME (432)\n", false);
		
	if (isUsedNick(_clientList , line.params[0], clientFd) == true)
		return (std::cout << "send error: ERR_NICKNAMEINUSE (433)\n", false);
		
	_clientList[clientFd].setNickname(line.params[0]);
	_clientList[clientFd].hasNick = true;

	// TODO: SERVER MUST  SEND TO CLIENTS ACKNOLEDGMENT TO SAY THEIR NICK COMMAND WAS SUCCESSFUL, AND TELL OTHER CLIENTS ABOUT THE CHANGE OF NICKNAME. <source> of the message will be the old nickname 
	_updateRegisteredState(clientFd);
	return (true);
}

bool	Server::_handleUser(int clientFd, const s_Line& line) {
	std::cout << "handle user command\n";
	Client* cli = &_clientList[clientFd];

	if (cli->getRegirstered()) {
		std::cout << "send an error: ERR_ALREADYREGISTERED (462)\n";
		return (false);
	}
	if (line.params.size() < 4 || line.params[0].empty())
		return (std::cout << "send an error:  ERR_NEEDMOREPARAMS (461) \n", false);
	cli->setUsername(line.params[0]);
	cli->setRealname(line.params[3]);
	cli->hasUser = true;
	_updateRegisteredState(clientFd);
	return (true);
}

void	Server::_handleJoin(int clientFd, const s_Line& line) const {
	std::cout << "handle join command\n";
	(void)clientFd;
	(void)line;
}
void	Server::_handlePrivmsg(int clientFd, const s_Line& line) const {
	std::cout << "handle privmsg command\n";
	(void)clientFd;
	(void)line;
}
void	Server::_handlePart(int clientFd, const s_Line& line) const {
	std::cout << "handle part command\n";
	(void)clientFd;
	(void)line;
}
void	Server::_handleQuit(int clientFd, const s_Line& line) const {
	std::cout << "handle quit command\n";
	(void)clientFd;
	(void)line;
}
void	Server::_cleanupClient(int clientFd) {
	std::cout << "cleanup client [" << clientFd << "]\n";
}