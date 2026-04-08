#include "Server.hpp"
#include "Client.hpp"
#include <iostream>

bool	Server::_checkAndExecuteLine(int clientFd, const s_Line& sLine) { // change name to checkAndExecuteLine
	
	
	if (sLine.command == "PASS") {
		_handlePass(clientFd, sLine);
	}
	else if (sLine.command == "NICK") {
		_handleNick(clientFd, sLine);
	}
	else if (sLine.command == "USER") {
		_handleUser(clientFd, sLine);
	}
	else if (sLine.command == "JOIN") {
		_handleJoin(clientFd, sLine);
	}
	else if (sLine.command == "PING") {
		_handlePing(clientFd, sLine);
	}
	else if (sLine.command == "PONG") {
		return (true);
	}
	else {
		std::cout << "match else. command =[" << sLine.command << "]\n";
		std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
		if (_clientList[clientFd].getRegirstered()) {	
			_sendUnknownCommand(clientFd, clientNick , sLine.command); // << ERR_UNKNOWNCOMMAND (421) "<client> <command> :Unknown command"
		}
		else {
			_sendUnregistered(clientFd, clientNick); // ERR_NOTREGISTERED (451)  "<client> :You have not registered"
		}
		return (false);
	}	
	return (true);
}

bool	Server::_handlePass(int clientFd, const s_Line& line) {

	// validation 
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	if (_clientList[clientFd].getRegirstered()) {
		_sendAlreadyRegistered(clientFd, clientNick); // ERR_ALREADYREGISTERED (462) "<client> :You may not reregister"
		return (false);
	}
	if (line.params.empty()) {
		_sendNeedMoreParams(clientFd, clientNick, line.command); // ERR_NEEDMOREPARAMS (461) "<client> <command> :Not enough parameters"
		return (false);
	}
	
	// execution
	
	Client&	cli = _clientList[clientFd];
	
	if (_passwordEnabled) {
		if (line.params[0] == _password) {
			cli.setPassAccepted(true);
		}
		else {
			_sendPassMisMatch(clientFd, clientNick); // ERR_PASSWDMISMATCH (464) "<client> :Password incorrect"
			
			cli.setPassAccepted(false);
			return (false);
		}
	}
	_updateRegisteredState(clientFd);
	return (true);
}


bool	Server::_handleNick(int clientFd, const s_Line& line) {
	
	// validation 
	
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	if (line.params.empty()) {
		_sendNoNickNameGiven(clientFd, clientNick); // ERR_NONICKNAMEGIVEN (431)  "<client> :No nickname given"
		return (false);
	}
	if (_isValidNick(line.params[0]) == false) {
		_sendErrOnUseNickName(clientFd, clientNick, line);
		return (false); //  ERR_ERRONEUSNICKNAME (432) "<client> <nick> :Erroneus nickname"

	}
	if (_isUsedNick(_clientList , line.params[0], clientFd) == true) {
		_sendNickNameInUse(clientFd, clientNick, line);
		return (false); // ERR_NICKNAMEINUSE (433)  "<client> <nick> :Nickname is already in use"

	}		

	// execution
	
	_clientList[clientFd].setNickname(line.params[0]);
	_clientList[clientFd].hasNick = true;


	// TODO: SERVER MUST  SEND TO CLIENTS ACKNOLEDGMENT TO SAY THEIR NICK COMMAND WAS SUCCESSFUL, AND TELL OTHER CLIENTS ABOUT THE CHANGE OF NICKNAME. <source> of the message will be the old nickname 
	_updateRegisteredState(clientFd);
	return (true);
}

bool	Server::_handleUser(int clientFd, const s_Line& line) {

	// validation 
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	
	if (line.params.size() < 4 || line.params[0].empty()) {
		_sendNeedMoreParams(clientFd, clientNick, line.command); // ERR_NEEDMOREPARAMS (461) "<client> <command> :Not enough parameters"
		return (false);
	}
	if (_clientList[clientFd].getRegirstered()) {
		_sendAlreadyRegistered(clientFd, clientNick); // ERR_ALREADYREGISTERED (462) "<client> :You may not reregister"
		return (false);
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
	
	// validation 

	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	if (!_clientList[clientFd].getRegirstered()) {
		_sendUnregistered(clientFd, clientNick); // ERR_NOTREGISTERED (451)  "<client> :You have not registered"
		return (false);
	}
	if (line.params.empty()) {
		_sendNeedMoreParams(clientFd, clientNick, line.command); // ERR_NEEDMOREPARAMS (461) "<client> <command> :Not enough parameters"
		return (false);
	}
	// check if valid prefix Chantype (default= '#') or channel name <= 1 character.	
	if (line.params[0].size() <= 1 || line.params[0][0] != '#') {
		_sendErrBadChanMask(clientFd, clientNick, line.params[0]);
		return (false); // ERR_BADCHANMASK (476)  "<client> <channel> :Bad Channel Mask"  . in rpl_infos tells channel start with '#'
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
	// send messages see JOIN part in the modern IRC documentation
	_printChannel(chan);
	return (true);
}

bool	Server::_handlePrivmsg(int clientFd, const s_Line& line) {
	std::cout << "handle privmsg command\n";
	(void)clientFd;
	(void)line;
	return (true);
}

bool	Server::_handlePart(int clientFd, const s_Line& line) {
	std::cout << "handle part command\n";
	(void)clientFd;
	(void)line;
	return (true);
}

bool	Server::_handleQuit(int clientFd, const s_Line& line) {
	std::cout << "handle quit command\n";
	(void)clientFd;
	(void)line;
	return (true);
}

bool	Server::_handlePing(int clientFd, const s_Line& sline) {
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	
	if (sline.params.empty()) {
		_sendNeedMoreParams(clientFd, clientNick, sline.command);
		return (false);
	}
	std::string line = ":" + _serverName + " PONG " + _serverName + " " + sline.params[0];
	_sendToClient(clientFd, line);
	return (true);
}
