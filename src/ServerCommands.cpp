#include "Server.hpp"
#include "Client.hpp"
#include <iostream>
#include <set>

bool	Server::_dispatchCommand(int clientFd, const s_Line& sLine) {
	
	std::string clientNick = _clientList[clientFd].getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}

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
	else if (sLine.command == "QUIT") {
		return (_handleQuit(clientFd, sLine));
	}
	else if (_clientList[clientFd].getRegirstered()) {
		
		if (sLine.command == "JOIN") {
			return (_handleJoin(clientFd, sLine));
		}
		else if (sLine.command == "PART") {
			return (_handlePart(clientFd, sLine));
		}
		else if (sLine.command == "PRIVMSG") {
			return (_handlePrivmsg(clientFd, sLine));
		}
		else if (sLine.command == "KICK") {
			return (_handleKick(clientFd, sLine));
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
	std::string clientNick = _clientList[clientFd].getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}
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


bool	Server::_handleNick(int clientFd, const s_Line& sline) {
	
	// validation 
	Client*	pCli = getClient(clientFd);

	if (pCli == false) {
		return (false);
	}
	std::string clientNick = pCli->getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}
	if (sline.params.empty()) {
		_sendErrNoNickNameGiven(clientFd, clientNick);
		return (false);
	}
	if (_isValidNick(sline.params[0]) == false) {
		_sendErrOnUseNickName(clientFd, clientNick, sline.params[0]);
		return (false); 
	}
	if (_isUsedNick(_clientList , sline.params[0], clientFd) == true) {
		_sendErrNickNameInUse(clientFd, clientNick, sline.params[0]);
		return (false); 
	}		

	// execution
	if (pCli->hasNick) {
		pCli->setOldNickname(pCli->getNickname());
	}
	// TODO: SERVER MUST  SEND TO CLIENTS ACKNOLEDGMENT TO SAY THEIR NICK COMMAND WAS SUCCESSFUL, AND TELL OTHER CLIENTS ABOUT THE CHANGE OF NICKNAME. <source> of the message will be the old nickname 

	pCli->setNickname(sline.params[0]);
	pCli->hasNick = true;

	
	_updateRegisteredState(clientFd);
	return (true);
}

bool	Server::_handleUser(int clientFd, const s_Line& line) {

	// validation 
	std::string clientNick = _clientList[clientFd].getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}	
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

	std::string clientNick = _clientList[clientFd].getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}	if (sline.params.empty()) {
		_sendErrNeedMoreParams(clientFd, clientNick, sline.command);
		return (false);
	}
	std::string chanParam = sline.params[0];
	Client& 	cli = _clientList[clientFd];

	while (!chanParam.empty()) {
		std::size_t commaPos = chanParam.find(',');
		std::string	chanName;
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
		if (chanName.size() <= 1 || chanName[0] != '#') {
			_sendErrBadChanMask(clientFd, clientNick, chanName);
			continue;
		}
		if (_channelList.find(chanName) == _channelList.end()) {
			_channelList.insert(std::make_pair(chanName, Channel(chanName)));
		}
		Channel& chan = _channelList[chanName]; 
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
	Client* pCli = getClient(clientFd);
	if (pCli == NULL) {
		return (false);
	}
	std::string	clientNick = (pCli->getNickname().empty() ? "*" :  pCli->getNickname());
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
		Channel*	pChan = getChannel(chanName);
		if (pChan != NULL) {
			if (pChan->isMember(clientFd)) {
				const std::map<int, Channel::MemberState>&	chanMembers = pChan->getMembers();
				std::map<int, Channel::MemberState>::const_iterator membersIt = chanMembers.begin();
				for (; membersIt != chanMembers.end(); ++membersIt) {
					std::string prefix = ":" + clientNick + "!" + pCli->getUsername() + "@" + pCli->ipAddr;
					std::string line = prefix + " " + sline.command + " " + chanName + (reason.empty() ? "" : (" :" + reason)); 
					_sendToClient(membersIt->first, line);
				}
				pChan->removeMember(clientFd);
				pCli->removeSubscribedChannel(chanName);
				_printChannel(*pChan);
				if (pChan->empty()) {
					_channelList.erase(_channelList.find(chanName));
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

// client send QUIT command to quit the server
bool	Server::_handleQuit(int clientFd, const s_Line& sline) {
	Client* 					pCli = getClient(clientFd);
	if (pCli == NULL) {
		return (false);
	} 
	typedef std::map<int, Channel::MemberState> ChannelMembersMap;
	std::string					reason = (sline.params.empty() ? "" : sline.params[0]); 

	// construct the notification message
	std::string					nick = pCli->getNickname().empty() ? "*" : pCli->getNickname();
	std::string					username = pCli->getUsername().empty() ? "*" : pCli->getUsername(); 
	std::string					prefix = ":" + nick + "!" + username + "@" + pCli->ipAddr;
	std::string					line = prefix + " " + sline.command + " :Quit:" + (reason.empty() ? "" : " " + reason);


	std::vector<std::string>	cliChannels = pCli->getSubscribedChannels();
	std::set<int>				listMembersToNotify;

	// loop on the client's channel list
	for (std::size_t i = 0; i < cliChannels.size(); ++i) {
		std::string			chanName = cliChannels[i];
		std::map<std::string, Channel>::iterator chanIt = _channelList.find(chanName);
		if (chanIt == _channelList.end()) {
			continue;
		}
		Channel&			channel = chanIt->second;
		ChannelMembersMap& 	members = channel.getMembers();

		// create list of the members to notify 
		// with a unique key container std::set<int>,
		//  to only send the msg once even if the member shares > 1 channel with the quitting member
		ChannelMembersMap::const_iterator it = members.begin();
		for (; it != members.end(); ++it) {
			if (it->first == clientFd) {
				continue;
			}
			listMembersToNotify.insert(it->first);
		}
	}
	// send QUIT notification to the members on the list of the members to notify
	for (std::set<int>::iterator it = listMembersToNotify.begin(); it != listMembersToNotify.end(); ++it) {
		_sendToClient(*it, line);
	}
	// send a QUIT acknowledgement to the client
	_sendToClient(clientFd, "ERROR :Closing Link: Quit:" + (reason.empty() ? "" : " " + reason));
	
	// clean the client from the server and close his socket
	_cleanupClient(*pCli);
	return (true);
}

bool	Server::_handlePing(int clientFd, const s_Line& sline) {
	Client*	pCli = getClient(clientFd);
	if (pCli == NULL) {
		return (false);
	}
	std::string clientNick = (pCli->getNickname().empty() ? "*" : pCli->getNickname());
	
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(clientFd, clientNick, sline.command);
		return (false);
	}
	std::string line = ":" + _serverName + " PONG " + _serverName + " " + sline.params[0];
	_sendToClient(clientFd, line);
	return (true);
}

bool	Server::_handlePrivmsg(int clientFd, const s_Line& sline) {
	Client* pCli = getClient(clientFd);
	if (pCli == NULL) {
		return (false);
	}
	std::string clientNick = (pCli->getNickname().empty() ? "*" : pCli->getNickname());
	std::string clientUsername = (pCli->getUsername().empty() ? "*" : pCli->getUsername());
	
	if (sline.params.empty()) {
		_sendErrNoRecipient(clientFd, clientNick, sline.command);
		return (false);
	}
	if (sline.params.size() < 2 || sline.params[1].empty()) {
		_sendErrNoTextToSend(clientFd, clientNick);
		return (false);
	}
	bool		sendAtLeastOne = false;
	std::string targetParam = sline.params[0];
	std::string message = sline.params[1];
	std::string prefix = ":" + clientNick + "!" + clientUsername + "@" + pCli->ipAddr;
	
	// to handle multi target ex: #chan1,#chan2... split on every comma ',' until the end
	while (!targetParam.empty()) {
		std::size_t	commaPos = targetParam.find(',');
		std::string targetName;
		if (commaPos == std::string::npos) {
			targetName = targetParam;
			targetParam.clear();
		}
		else {
			targetName = targetParam.substr(0, commaPos);
			targetParam.erase(0, commaPos + 1);
		}
		if (targetName.empty()) {
			continue;
		}
		
		// construct the full message

		std::string ret = prefix + " PRIVMSG " + targetName + " :" + message;
		
		// check if is target is a channel or a user
		if (targetName[0] != '#') {
			// check if client exist
			Client*	pUser = getUserByNick(targetName);
			if (pUser == NULL) {
				_sendErrNoSuchNick(clientFd, clientNick, targetName);
				continue;
			}
			// send the message to targetName
			_sendToClient(pUser->fd, ret);
			sendAtLeastOne = true;
		}
		else {
			// check if channel exist
			Channel* pChan = getChannel(targetName); 
			if (pChan == NULL) {
				_sendErrNoSuchChannel(clientFd, clientNick, targetName);
				continue;
			}
			// check if client is member
			if (!pChan->isMember(clientFd)) {
				_sendErrCannotSendToChan(clientFd, clientNick, targetName);
				continue;
			}
			// send the message
			const std::map<int, Channel::MemberState>& members = pChan->getMembers();
			std::map<int, Channel::MemberState>::const_iterator membersIt = members.begin();
			for (; membersIt != members.end(); ++membersIt) {
				if (membersIt->first == clientFd) {
					continue;
				}
				_sendToClient(membersIt->first, ret);
				sendAtLeastOne = true;                
			}
		}
	}
	// if the targe is a user and the user is not connected send RPL_AWAY (301)
	return (sendAtLeastOne);
}

//? Command: KICK 
//? Parameters: <channel> <user> *( "," <user> ) [<comment>]
//? KICK #Finnish Matthew ; Command to kick Matthew from #Finnish
//? KICK #Finnish John :Speaking English; Command to kick John from #Finnish using "Speaking English" as the reason (comment).

bool	Server::_handleKick(int clientFd, const s_Line& sline) {
	(void)clientFd;
	(void)sline;
	// Client &cli = getClient(clientFd);

	// // check params size
	// //! ERR_NEEDMOREPARAMS (461)   "<client> <command> :Not enough parameters"
	// if (sline.params.size() < 2) {
	// 	_sendErrNeedMoreParams(clientFd, )
	// }                           	                                                                    

	// // check channel if exist
	// //! ERR_NOSUCHCHANNEL (403) "<client> <channel> :No such channel"


	// // check op privileges of the client
	// //! ERR_NOTONCHANNEL (442)  "<client> <channel> :You're not on that channel"
	// //! ERR_CHANOPRIVSNEEDED (482)  "<client> <channel> :You're not channel operator"


	// // check user in channel 
	// //! ERR_USERNOTINCHANNEL (441)   "<client> <nick> <channel> :They aren't on that channel"


	// // 
	return (true);
}
