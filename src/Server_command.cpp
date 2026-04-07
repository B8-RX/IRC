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

	// validation 

	if (_clientList[clientFd].getRegirstered()) {
		std::cout << "send an error: ERR_ALREADYREGISTERED (462)\n";
		return (false);
	}
	if (line.params.empty()) {
		std::cout << "send an error:  ERR_NEEDMOREPARAMS (461) \n";
		return (false); // IGNORE AND CONTINUE
	}

	// execution

	Client&	cli = _clientList[clientFd];

	if (_passwordEnabled) {
		if (line.params[0] == _password) {
			std::cout << "password accepted\n";
			cli.setPassAccepted(true);
		}
		else {
			std::cout << "send an error: ERR_PASSWDMISMATCH (464)\n"; 
			cli.setPassAccepted(false);
			return (false);
		}
	}
	_updateRegisteredState(clientFd);
	return (true);
}


bool	Server::_handleNick(int clientFd, const s_Line& line) {
	std::cout << "handle nick command\n";
	
	// validation 

	if (line.params.empty()) {
		std::cout << "send error:   ERR_NONICKNAMEGIVEN (431)  \n";
		return (false);
	}
	if (_isValidNick(line.params[0]) == false) {
		return (std::cout << "send error: ERR_ERRONEUSNICKNAME (432)\n", false);
	}
	if (_isUsedNick(_clientList , line.params[0], clientFd) == true) {
		return (std::cout << "send error: ERR_NICKNAMEINUSE (433)\n", false);
	}		

	// execution
	
	_clientList[clientFd].setNickname(line.params[0]);
	_clientList[clientFd].hasNick = true;

	// TODO: SERVER MUST  SEND TO CLIENTS ACKNOLEDGMENT TO SAY THEIR NICK COMMAND WAS SUCCESSFUL, AND TELL OTHER CLIENTS ABOUT THE CHANGE OF NICKNAME. <source> of the message will be the old nickname 
	_updateRegisteredState(clientFd);
	return (true);
}

bool	Server::_handleUser(int clientFd, const s_Line& line) {
	std::cout << "handle user command\n";

	// validation 

	if (_clientList[clientFd].getRegirstered()) {
		std::cout << "send an error: ERR_ALREADYREGISTERED (462)\n";
		return (false);
	}
	if (line.params.size() < 4 || line.params[0].empty()) {
		return (std::cout << "send an error:  ERR_NEEDMOREPARAMS (461) \n", false);
	}

	// execution
	Client& cli = _clientList[clientFd];

	cli.setUsername(line.params[0]);
	cli.setRealname(line.params[3]);
	cli.hasUser = true;
	_updateRegisteredState(clientFd);
	return (true);
}
	
bool	Server::_handleJoin(int clientFd, const s_Line& line) {
	std::cout << "handle join command\n";
	
	// validation 
	if (!_clientList[clientFd].getRegirstered()) {
		return (std::cout << "send error: ERR_NOTREGISTERED (451)\n", false);
	}
	if (line.params.empty()) {
		return (std::cout << "send an error:  ERR_NEEDMOREPARAMS (461) \n", false);
	}
	// check if valid prefix Chantype (default= '#') or channel name <= 1 character.	
	if (line.params[0].size() <= 1 || line.params[0][0] != '#') {
		return (std::cout << "send error: ERR_BADCHANMASK (476)\n", false); // in rpl_info tells channel start with '#' ??
	}

	// execution
	Client& 									cli = _clientList[clientFd];
	std::string									chanName = line.params[0];
	std::map<std::string, Channel>::iterator	chanIt = getChannel(chanName);
	
	if (chanIt == _channelList.end()) {
		chanIt = _channelList.insert(std::make_pair(chanName, Channel(chanName))).first;
	}
	Channel& 	chan = chanIt->second;
	bool		isFirstMember = (chan.memberCount() == 0);
	if (chan.isMember(clientFd)) {
		std::cerr << "channel [" << chan.getName() << "]: Client [" << cli.getNickname() << "] is already member of the channel!" << "\n";
		return (false);
	}
	if (!chan.addMember(clientFd, isFirstMember)) {
		std::cerr << "channel [" << chan.getName() << "]: Error occured. Cannot add member [" << cli.getNickname() << "] to the channel!\n";
		return (false);
	}
	if (!cli.isMemberChan(chanName)) {
		cli.addMemberChan(chanName);
	}
	// std::cout << "\n===========================================\n";
	// std::cout << "member [" << cli.getNickname() << "]\n";
	// std::cout << "join channel: [" << chanName << "]\n";
	// std::cout << "is operator: [" << (chan.isChanOp(clientFd) ? "true" : "false") << "]\n";
	// std::cout << "===========================================\n\n";
	
	// send messages see JOIN part in the modern IRC documentation
	_printChannel(chan);
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