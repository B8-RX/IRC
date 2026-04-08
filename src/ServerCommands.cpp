#include "Server.hpp"
#include "Client.hpp"
#include <iostream>

bool	Server::_checkAndExecuteLine(int clientFd, const s_Line& sLine) {
	
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	
	if (sLine.command == "PASS") {
		return (_handlePass(clientFd, sLine));
	}
	else if (sLine.command == "NICK") {
		return (_handleNick(clientFd, sLine));
	}
	else if (sLine.command == "USER") {
		return (_handleUser(clientFd, sLine));
	}
	else if (sLine.command == "PING") {
		return (_handlePing(clientFd, sLine));
	}
	else if (sLine.command == "PONG") {
		return (true);
	}
	else if (_clientList[clientFd].getRegirstered()) {
		
		if (sLine.command == "JOIN") {
			return (_handleJoin(clientFd, sLine));
		}
		else if (sLine.command == "PART") {
			return (_handlePart(clientFd, sLine));
		}
		else {
			_sendErrUnknownCommand(clientFd, clientNick , sLine.command);
			return (false);
		}
	}
	else {
		_sendErrUnregistered(clientFd, clientNick);
		return (false);
	}	
	return (true);
}

bool	Server::_handlePass(int clientFd, const s_Line& line) {

	// validation 
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	if (_clientList[clientFd].getRegirstered()) {
		_sendErrAlreadyRegistered(clientFd, clientNick); 
		return (false);
	}
	if (line.params.empty()) {
		_sendErrNeedMoreParams(clientFd, clientNick, line.command); 
		return (false);
	}
	
	// execution
	
	Client&	cli = _clientList[clientFd];
	
	if (_passwordEnabled) {
		if (line.params[0] == _password) {
			cli.setPassAccepted(true);
		}
		else {
			_sendErrPassMisMatch(clientFd, clientNick);
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
		_sendErrNoNickNameGiven(clientFd, clientNick);
		return (false);
	}
	if (_isValidNick(line.params[0]) == false) {
		_sendErrOnUseNickName(clientFd, clientNick, line);
		return (false); 
	}
	if (_isUsedNick(_clientList , line.params[0], clientFd) == true) {
		_sendErrNickNameInUse(clientFd, clientNick, line);
		return (false); 
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
		_sendErrNeedMoreParams(clientFd, clientNick, line.command); 
		return (false);
	}
	if (_clientList[clientFd].getRegirstered()) {
		_sendErrAlreadyRegistered(clientFd, clientNick); 
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

bool	Server::_handleJoin(int clientFd, const s_Line& sline) {
	
	// validation 

	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(clientFd, clientNick, sline.command);
		return (false);
	}
	// check if valid prefix Chantype (default= '#') or channel name <= 1 character.	
	if (sline.params[0].size() <= 1 || sline.params[0][0] != '#') {
		_sendErrBadChanMask(clientFd, clientNick, sline.params[0]);
		return (false);
	}
	
	std::string chanParam = sline.params[0];
	Client& 									cli = _clientList[clientFd];

	while (!chanParam.empty()) {
		std::size_t commaPos = chanParam.find(',');
		std::string 								chanName;
		if (commaPos == std::string::npos)
		{
			chanName = chanParam;
			chanParam.clear();
		}
		else {
			chanName = chanParam.substr(0, commaPos);
			chanParam.erase(0, commaPos + 1);
		}
		if (chanName.empty()) {
			continue;
		}
		Channel& 	chan = getChannel(chanName);
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
		//!TODO handle multiple keys (sline.params[1]) separated by colon see documentation of JOIN cmd.
			
	}
	return (true);
}

bool	Server::_handlePart(int clientFd, const s_Line& sline) {
	std::string	clientNick =_clientList[clientFd].getNickname();
	
	if (clientNick.empty()) {
		clientNick = "*";
	}
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(clientFd, clientNick, sline.command);
		return (false);
	}
	std::string chanParam = sline.params[0];
	std::string reason = (sline.params.size() > 1 ? sline.params[1] : "");
	while (!chanParam.empty())
	{
		std::size_t commaPos = chanParam.find(',');
		std::string 								chanName;
		if (commaPos == std::string::npos)
		{
			chanName = chanParam;
			chanParam.clear();
		}
		else {
			chanName = chanParam.substr(0, commaPos);
			chanParam.erase(0, commaPos + 1);
		}
		if (chanName.empty())
			continue;
		std::map<std::string, Channel>::iterator	channelIt = _channelList.find(chanName);
		std::map<int, Client>::iterator				client = _clientList.find(clientFd);
		if (channelIt != _channelList.end()) {
			if (channelIt->second.isMember(clientFd)) {
				const std::map<int, Channel::MemberState>&	chanMembers = channelIt->second.getMembers();
				std::map<int, Channel::MemberState>::const_iterator membersIt = chanMembers.begin();
				for (; membersIt != chanMembers.end(); ++membersIt) {
					std::string line = ":" + clientNick + " " + sline.command + " " + chanName + (reason.empty() ? "" : (" :" + reason)); 
					_sendToClient(membersIt->first, line);
				}
				channelIt->second.removeMember(clientFd);
				client->second.removeSubscribedChannel(chanName);
				_printChannel(channelIt->second);
				if (channelIt->second.empty()) {
					_channelList.erase(channelIt);
					std::cout << "channel [" << chanName << "] have 0 member. Channel have been deleted successfully.\n";
				}
			}
			else {
				_sendErrNotOnChannel(clientFd, clientNick, chanName);
			}
		}
		else {
			_sendErrNoSuchChannel(clientFd, clientNick, chanName);
		}
	}
	return (true);
}

bool	Server::_handleQuit(int clientFd, const s_Line& line) {
	std::cout << "handle quit command\n";
	(void)clientFd;
	(void)line;
	return (true);
}

bool	Server::_handlePrivmsg(int clientFd, const s_Line& line) {
	std::cout << "handle privmsg command\n";
	(void)clientFd;
	(void)line;
	return (true);
}

bool	Server::_handlePing(int clientFd, const s_Line& sline) {
	std::string clientNick = (_clientList[clientFd].getNickname().empty() ? "*" : _clientList[clientFd].getNickname());
	
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(clientFd, clientNick, sline.command);
		return (false);
	}
	std::string line = ":" + _serverName + " PONG " + _serverName + " " + sline.params[0];
	_sendToClient(clientFd, line);
	return (true);
}
