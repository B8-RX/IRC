#include "Server.hpp"
#include "libs.hpp"
#include "ServerParsing.cpp"
#include "ServerCommands.cpp"
#include "ServerUtils.cpp"
#include "ServerMessaging.cpp"

bool Server::_signalReceived = false;

Server::Server(uint16_t port, const std::string& password) : _serverName("irc.local"), _creationTime(std::time(0)), _password(password), _passwordEnabled(!(_password.empty())), _port(port) {}

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
	std::cout << MAGENTA << "server initialized successfully.\n"
				<< "Port: " << _port << RESET "\n";
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
	_closeServer();
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
		std::cerr << RED << "accept: Failed to create and connect client socket: " + std::string(strerror(errno)) << RESET;
		return ;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << RED << "fcntl: Failed to add optin O_NONBLOCK to client socket: " + std::string(strerror(errno)) << RESET;
		return ;
	}
	newPollfd.fd = clientFd;
	newPollfd.events = POLLIN;
	newPollfd.revents = 0;
	client.fd = clientFd;
	client.ipAddr = inet_ntoa(clientAdd.sin_addr);
	_clientList.insert(std::make_pair(clientFd, client));
	_pollfdList.push_back(newPollfd);

	std::cout << MAGENTA "\nnew connection established on fd " << "[" << clientFd << "]\n";
	std::cout << "address: [" << client.ipAddr << "]\n" << RESET;
}


void	Server::_handleReceivedData(int clientFd) {

	Client*	pCli = getClient(clientFd);
	if (pCli == NULL) {
		return;
	}
	char	buffer[BUFFER_SIZE];
	int		readBytes;

	memset(buffer, 0, sizeof(buffer));
	readBytes = recv(clientFd, buffer, sizeof(buffer), MSG_DONTWAIT);
	
	if (readBytes == 0) {
		//?INFO: end of line reached or client disconnected. cleanup...
		//TODO send QUIT acknowledgement to other users whom share the same channel with this client
		//TODO add log of the quit
		_cleanupClient(pCli->fd, "");
		return ;
	}
	else if (readBytes == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
		//?INFO: DATA IS NOT READY AND OPTION O_NONBLOCK IS ENABLED ON THE SOCKET (silently ignored)
			return ;
		}
		else
		{
			std::cerr << RED << "recv: " + std::string(strerror(errno)) << RESET;
			return ;
		}
	}
	pCli->bufferIn.append(buffer, readBytes);
	
	struct s_Line				sLine;
	std::vector<std::string>	vLines;
	
	vLines = _splitCRLF(clientFd); // extract complete lines (lines ending with \r\n)
	
	// handle received data
	for (std::size_t i = 0; i < vLines.size(); ++i) {
		sLine = _parseLine(vLines[i]);
		_dispatchCommand(clientFd, sLine);
	}
}

bool	Server::_updateRegisteredState(int clientFd) {
	Client*	cli = &_clientList[clientFd];
	cli->setRegirstered((cli->getPassAccepted() || !_passwordEnabled) && cli->hasNick && cli->hasUser);
	return (cli->getRegirstered());
}

void	Server::_cleanupClient(int clientFd, const std::string& cmd) {

	Client* cli = getClient(clientFd);
	if (cli == NULL) {
		return;
	}
	std::string	nickname = cli->getNickname();
	std::string username = cli->getUsername();
	std::string ipAddr = cli->ipAddr;

	if (cmd != "PASS") {
		std::vector<std::string>	cliChannels = cli->getSubscribedChannels();
		if (!cliChannels.empty()) {
			std::string prefix = ":" + nickname + "!" + username + "@" + ipAddr;
			std::string	message;
			std::set<int>		notifiedUser;
			// boucler sur la liste des channels dont le client est membre
			for (std::size_t i = 0; i < cliChannels.size(); ++i) {
				
				// recuperer le channel
				std::string			chanName = cliChannels[i];
				std::map<std::string, Channel>::iterator chanIt = _channelList.find(chanName);
				if (chanIt == _channelList.end()) {
					continue;
				} 
				Channel&			channel = chanIt->second;
				if (channel.isMember(clientFd)) {
		
					// notifier tout les membre que le client a quitter le network
					const std::map<int, Channel::MemberState>&	chanMembers = channel.getMembers();
					std::map<int, Channel::MemberState>::const_iterator membersIt = chanMembers.begin();
					bool	foundChanOp = false;
					std::string prefix = ":" + nickname + "!" + username + "@" + ipAddr;
					message = prefix + " QUIT :Connection closed";
					for (; membersIt != chanMembers.end(); ++membersIt) {
						if (!foundChanOp && membersIt->first != clientFd && membersIt->second.isChanOp == true) {
							foundChanOp = true;
						}
						if (membersIt->first == clientFd || notifiedUser.find(membersIt->first) != notifiedUser.end()) {
							continue;
						}
						if (cmd.empty()) {
							_sendToClient(membersIt->first, message);
							notifiedUser.insert(membersIt->first);
						}
					}
					// retirer le client de la liste des membres du channel
					channel.removeMember(clientFd);
					
					// check if the client was operator if yes check if there is another op in the channel
					if (channel.memberCount() > 0 && !foundChanOp) {
						int newOpFd = -1;
						std::string	newOpNick;
						std::string msg = ":" + _serverName + " MODE " + chanName + " +o ";
						std::map<int, Channel::MemberState>&	members = channel.getMembers();
						std::map<int, Channel::MemberState>::iterator opMemberIt = members.begin();
						newOpFd = opMemberIt->first;
						channel.updateMemberState(newOpFd, true);
						newOpNick = getClient(newOpFd)->getNickname();
						msg += newOpNick;
						for (; opMemberIt != members.end(); ++opMemberIt) {
							_sendToClient(opMemberIt->first, msg);
						}
					}
				}
				// supprimer le channel si vide
				if (channel.empty()) {
					_channelList.erase(chanIt);
				}
				
			}
		}
	}
	
	// retirer la structure poll qui surveillait les events sur le socket du client
	for (std::size_t i = 0; i < _pollfdList.size(); ++i) {
		if (_pollfdList[i].fd == clientFd) {
			_pollfdList.erase(_pollfdList.begin() + i);
			break;
		}
	}
	
	// retirer le client de la liste des clients du serveur
	if (_clientList.find(clientFd) != _clientList.end()) {
		_clientList.erase(clientFd);
	}

	// fermer le socket
	if (clientFd != -1) {
		std::cout << "close client socket [" << clientFd << "]\n";
		close(clientFd);
	}

	if (cmd == "PASS") {
		std::cout << "[DEBUG]: QUIT :Connection closed [END]\n";
	}
	else {
		std::string prefix = nickname + "!" + username + "@" + ipAddr;
		std::string message = prefix + " QUIT :Connection closed";
		std::cout << "[" << "DEBUG" << "]: " << message << " [END]\n";
	}
}

void	Server::_closeServer(void) {
	while (!_clientList.empty()) {
		_cleanupClient(_clientList.begin()->second.fd, "");
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


std::size_t	Server::clientCount(void) const {
	return (_clientList.size());
}

std::size_t	Server::channelCount(void) const {
	return (_channelList.size());
}

Client*			Server::getClient(int clientFd) {
	std::map<int, Client>::iterator cliIt =  _clientList.find(clientFd);
	if (cliIt == _clientList.end()) {
		return (NULL);
	}
	return (&cliIt->second);
}

Client*				Server::getClientByNick(const std::string& nick) {
	std::map<int, Client>::iterator it = _clientList.begin();
	for (; it != _clientList.end(); ++it) {
		if (it->second.getNickname() == nick) {
			break;
		}
	}
	if (it == _clientList.end()) {
		return (NULL);
	}
	return (&it->second);
}

Channel*	Server::getChannel(const std::string& name) {
	std::map<std::string, Channel>::iterator it = _channelList.find(name);
	if (it == _channelList.end()) {
		return (NULL);
	}
	return (&it->second);
}

time_t	Server::getCreationTime(void) const {
	return (_creationTime);
}

std::string	Server::getChanListStr(void) const {
	std::map<std::string, Channel>::const_iterator chanListIt = _channelList.begin();
	std::string	chanListStr = "";
	for (; chanListIt != _channelList.end(); ++chanListIt) {
		chanListStr += (" " + chanListIt->first);
	}
	return (chanListStr);
}

std::size_t	Server::getChanListCount(void) const {
	std::map<std::string, Channel>::const_iterator chanListIt = _channelList.begin();
	std::size_t count = 0;
	for (; chanListIt != _channelList.end(); ++chanListIt) {
		count++;
	}
	return (count);
}