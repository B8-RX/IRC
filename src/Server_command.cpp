#include "Server.hpp"
#include "Client.hpp"
#include <iostream>


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
	else if (sLine.command == "JOIN")
		isValid = _handleJoin(clientFd, sLine);
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
	if (!_passwordEnabled)
		return (true);
	if (line.params[0] == _password) {
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

	if (_isValidNick(line.params[0]) == false)
		return (std::cout << "send error: ERR_ERRONEUSNICKNAME (432)\n", false);
		
	if (_isUsedNick(_clientList , line.params[0], clientFd) == true)
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
	
bool	Server::_handleJoin(int clientFd, const s_Line& line) {
	std::cout << "handle join command\n";
	Client* cli = &_clientList[clientFd];

	if (!cli->getRegirstered())
		return (std::cout << "send error: ERR_NOTREGISTERED (451)\n", false);
	if (line.params.empty())
		return (std::cout << "send an error:  ERR_NEEDMOREPARAMS (461) \n", false);
	// check if valid prefix Chantype (default= '#')	
	if (line.params[0][0] != '#')
		return (std::cout << "send error: ERR_NOSUCHCHANNEL (403)\n", false); // in rpl_info tells channel start with '#' ??
	
    std::string chanName = line.params[0];
    std::map<std::string, Channel>::iterator chanIt = getChannel(chanName);

    if (chanIt == _channelList.end())
    {
        // create channel
        Channel newChan(chanName);
        newChan.addMember(clientFd, true);
        _channelList[chanName] = newChan;
        cli->addChannelMembership(chanName);
        // send messages see JOIN part in the modern IRC documentation
        std::cout << "\n===========================================\n";
        std::cout << "create channel: [" << chanName << "]\n";
        std::cout << "first member [" << getClient(clientFd)->second.getNickname() << "]\n";
        std::cout << "is operator: [" << (newChan.isChanOpMember(clientFd) ? "true" : "false") << "]\n";
        std::cout << "===========================================\n\n";
    }
    else {
        chanIt->second.addMember(clientFd, false);
        cli->addChannelMembership(chanName);
        std::cout << "\n===========================================\n";
        std::cout << "new member [" << getClient(clientFd)->second.getNickname() << "]\n";
        std::cout << "join channel: [" << chanName << "]\n";
        std::cout << "is operator: [" << (chanIt->second.isChanOpMember(clientFd) ? "true" : "false") << "]\n";
        std::cout << "===========================================\n\n";
        // send messages see JOIN part in the modern IRC documentation
    }
	return (true);
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