#include "Server.hpp"
#include "Client.hpp"
#include <iostream>
#include <set>

bool	Server::_dispatchCommand(int clientFd, s_Line& sLine) {
	
	Client* pCli = getClient(clientFd);
	if (pCli == NULL) {
		return (NULL);
	}
	std::string clientNick = pCli->getNickname();
	if (clientNick.empty()) {
		clientNick = "*";
	}

	if (sLine.command == "PASS") {
		return (_handlePass(*pCli, sLine));
	}
	else if (sLine.command == "NICK") {
		return (_handleNick(*pCli, sLine));
	}
	else if (sLine.command == "USER") {
		return (_handleUser(*pCli, sLine));
	}
	else if (sLine.command == "PING") {
		return (_handlePing(*pCli, sLine));
	}
	else if (sLine.command == "PONG") {
		return (true);
	}
	else if (sLine.command == "QUIT") {
		return (_handleQuit(*pCli, sLine));
	}
	else if (pCli->getRegirstered()) {
		
		if (sLine.command == "JOIN") {
			return (_handleJoin(*pCli, sLine));
		}
		else if (sLine.command == "PART") {
			return (_handlePart(*pCli, sLine));
		}
		else if (sLine.command == "PRIVMSG") {
			return (_handlePrivmsg(*pCli, sLine));
		}
		else if (sLine.command == "KICK") {
			return (_handleKick(*pCli, sLine));
		}
		else if (sLine.command == "INVITE") {
			return (_handleInvite(*pCli, sLine));
		}
		else {
			_sendErrUnknownCommand(*pCli, clientNick, sLine, sLine.command);
			return (false);
		}
	}
	else {
		_sendErrUnregistered(*pCli, clientNick, sLine, "");
		return (false);
	}	
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
bool	Server::_handlePass(Client& cli, const s_Line& sline) {

	// validation 
	std::string clientNick = cli.getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}
	if (cli.getRegirstered()) {
		_sendErrAlreadyRegistered(cli, clientNick, sline, ""); 
		return (false);
	}
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command); 
		return (false);
	}
	
	// execution
	
	if (_passwordEnabled) {
		if (sline.params[0] == _password) {
			cli.setPassAccepted(true);
		}
		else {
			_sendErrPassMisMatch(cli, clientNick, sline, "");
			cli.setPassAccepted(false);
			return (false);
		}
	}
	_updateRegisteredState(cli.fd);
	_printLogServer("INFO", cli, sline, sline.command);
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
bool	Server::_handleNick(Client& cli, const s_Line& sline) {
	
	// validation 

	std::string clientNick = cli.getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}
	if (sline.params.empty()) {
		_sendErrNoNickNameGiven(cli, clientNick, sline, "");
		return (false);
	}
	std::string	targetNick = sline.params[0];
	if (_isValidNick(sline.params[0]) == false) {
		_sendErrOnUseNickName(cli, clientNick, sline, targetNick);
		return (false); 
	}
	if (_isUsedNick(_clientList , targetNick, cli.fd) == true) {
		_sendErrNickNameInUse(cli, clientNick, sline, targetNick);
		return (false); 
	}		

	// execution
	if (cli.hasNick) {
		cli.setOldNickname(cli.getNickname());
	}
	// TODO: SERVER MUST  SEND TO CLIENTS ACKNOLEDGMENT TO SAY THEIR NICK COMMAND WAS SUCCESSFUL, AND TELL OTHER CLIENTS ABOUT THE CHANGE OF NICKNAME. <source> of the message will be the old nickname 

	cli.setNickname(sline.params[0]);
	cli.hasNick = true;

	
	_updateRegisteredState(cli.fd);
	_printLogServer("INFO", cli, sline, sline.command);
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
bool	Server::_handleUser(Client& cli, const s_Line& sline) {

	// validation 
	std::string clientNick =cli.getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}	
	if (sline.params.size() < 4 || sline.params[0].empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command); 
		return (false);
	}
	if (cli.getRegirstered()) {
		_sendErrAlreadyRegistered(cli, clientNick, sline, ""); 
		return (false);
	}
	
	// execution

	cli.setUsername(sline.params[0]);
	cli.setRealname(sline.params[3]);
	cli.hasUser = true;
	_updateRegisteredState(cli.fd);
	_printLogServer("INFO", cli, sline, sline.command);
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
bool	Server::_handleJoin(Client& cli, const s_Line& sline) {
	
	// validation 
	
	std::string clientNick = cli.getNickname();	
	if (clientNick.empty()) {
		clientNick = "*";
	}
		
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
		return (false);
	}
	std::string chanParam = sline.params[0];
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
			_sendErrBadChanMask(cli, clientNick, sline, chanName);
			continue;
		}
		if (_channelList.find(chanName) == _channelList.end()) {
			_channelList.insert(std::make_pair(chanName, Channel(chanName)));
		}
		Channel* pChan = getChannel(chanName);
		if (pChan == NULL) {
			continue;
		} 
		bool		isFirstMember = (pChan->memberCount() == 0);
		
		if (pChan->isMember(cli.fd)) {
			std::cerr << "channel [" << pChan->getName() << "]: Client [" << cli.getNickname() << "] is already member of the channel!" << "\n";
			return (false);
		}
		if (!pChan->addMember(cli.fd, isFirstMember)) {
			std::cerr << "channel [" << pChan->getName() << "]: Error occured. Cannot add member [" << cli.getNickname() << "] to the channel!\n";
			return (false);
		}
		if (!cli.isMemberChan(chanName)) {
			cli.addSubscriptionChan(chanName);
		}
		_printLogServer("INFO", cli, sline, chanName);

		//TODO send messages see JOIN part in the modern IRC documentation
		// _printChannel(chan); //! define a DEBUG macro, when true true it will trigger this logs
		//!TODO handle multiple keys (sline.params[1]) separated by colon see documentation of JOIN cmd.
			
	}
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
bool	Server::_handlePart(Client& cli, const s_Line& sline) {

	std::string	clientNick = (cli.getNickname().empty() ? "*" :  cli.getNickname());
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
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
			if (pChan->isMember(cli.fd)) {
				const std::map<int, Channel::MemberState>&	chanMembers = pChan->getMembers();
				std::map<int, Channel::MemberState>::const_iterator membersIt = chanMembers.begin();
				for (; membersIt != chanMembers.end(); ++membersIt) {
					std::string prefix = ":" + clientNick + "!" + cli.getUsername() + "@" + cli.ipAddr;
					std::string message = CYAN + prefix + " " + sline.command + " " + chanName + std::string(RESET);
					if (!reason.empty()) {
						message += " :"  + std::string(RESET) + reason;
					}
					_sendToClient(membersIt->first, message);
				}
				pChan->removeMember(cli.fd);
				cli.removeSubscribedChannel(chanName);
				_printLogServer("INFO", cli, sline, chanName);
				if (pChan->empty()) {
					_channelList.erase(_channelList.find(chanName));
					std::cout << "channel [" << chanName << "] have 0 member. Channel have been deleted successfully.\n";
				}
			}
			else {
				_sendErrNotOnChannel(cli, clientNick, sline, chanName);
			}
		}
		else {
			_sendErrNoSuchChannel(cli, clientNick, sline, chanName);
		}

	}
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
// client send QUIT command to quit the server
bool	Server::_handleQuit(Client& cli, const s_Line& sline) {

	typedef std::map<int, Channel::MemberState> ChannelMembersMap;
	std::string					reason = (sline.params.empty() ? "" : sline.params[0]); 

	// construct the notification message
	std::string					nick = cli.getNickname().empty() ? "*" : cli.getNickname();
	std::string					username = cli.getUsername().empty() ? "*" : cli.getUsername(); 
	std::string					prefix = ":" + nick + "!" + username + "@" + cli.ipAddr;
	std::string					message = CYAN +  prefix + " " + sline.command + " :Quit:" + (reason.empty() ? "" : " " + reason) + RESET;


	std::vector<std::string>	cliChannels = cli.getSubscribedChannels();
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
			if (it->first == cli.fd) {
				continue;
			}
			listMembersToNotify.insert(it->first);
		}
	}
	// send QUIT notification to the members on the list of the members to notify
	for (std::set<int>::iterator it = listMembersToNotify.begin(); it != listMembersToNotify.end(); ++it) {
		_sendToClient(*it, message);
	}
	// send a QUIT acknowledgement to the client
	message = std::string(CYAN) + "ERROR :Closing Link: Quit:" + std::string(RESET);
	if (!reason.empty()) {
		message += " "  + std::string(RESET) + reason;
	}
	_sendToClient(cli.fd, message);
	_printLogServer("INFO", cli, sline, sline.command);
	
	// clean the client from the server and close his socket
	_cleanupClient(cli);
	return (true);
}


//? Command:  
//? Parameters:  
//? ===> 
//? message <=== 
//? message <=== 
bool	Server::_handlePing(Client& cli, const s_Line& sline) {

	std::string clientNick = (cli.getNickname().empty() ? "*" : cli.getNickname());
	
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
		return (false);
	}
	std::string message =  std::string(CYAN) + ":" + _serverName + " PONG " + _serverName + " " + sline.params[0] + std::string(RESET);
	_sendToClient(cli.fd, message);
	_printLogServer("INFO", cli, sline, sline.command);
	return (true);
}


//? Command: PRIVMSG 
//? Parameters: <target>{,<target>} <text to be sent> 
//? ===> PRIVMSG Angel :yes I'm receiving it ! ; Command to send a message to Angel.
//? message <=== :Angel PRIVMSG Wiz :Hello are you receiving this message ? ; Message from Angel to Wiz.
//? message <=== :dan!~h@localhost PRIVMSG #coolpeople :Hi everyone!  ; Message from dan to the channel #coolpeople
bool	Server::_handlePrivmsg(Client& cli, const s_Line& sline) {

	std::string clientNick = (cli.getNickname().empty() ? "*" : cli.getNickname());
	std::string clientUsername = (cli.getUsername().empty() ? "*" : cli.getUsername());
	
	if (sline.params.empty()) {
		_sendErrNoRecipient(cli, clientNick, sline, sline.command);
		return (false);
	}
	if (sline.params.size() < 2 || sline.params[1].empty()) {
		_sendErrNoTextToSend(cli, clientNick, sline, "");
		return (false);
	}
	bool		sendAtLeastOne = false;
	std::string targetParam = sline.params[0];
	std::string privMsg = sline.params[1];
	std::string prefix = ":" + clientNick + "!" + clientUsername + "@" + cli.ipAddr;
	
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

		std::string message = CYAN + prefix + " PRIVMSG " + targetName + " :" + RESET + privMsg + std::string(RESET);
		
		// check if is target is a channel or a user
		if (targetName[0] != '#') {
			// check if client exist
			Client*	pUser = getClientByNick(targetName);
			std::string	targetUser = targetName;
			if (pUser == NULL) {
				_sendErrNoSuchNick(cli, clientNick, sline, targetUser);
				continue;
			}
			// send the message to targetUser
			_sendToClient(pUser->fd, message);
			_printLogServer("INFO", cli, sline, targetUser);

			sendAtLeastOne = true;
		}
		else {
			// check if channel exist
			Channel* pChan = getChannel(targetName); 
			std::string	targetChannel = targetName;
			if (pChan == NULL) {
				_sendErrNoSuchChannel(cli, clientNick, sline, targetChannel);
				continue;
			}
			// check if client is member
			if (!pChan->isMember(cli.fd)) {
				_sendErrCannotSendToChan(cli, clientNick, sline, targetChannel);
				continue;
			}
			// send the message
			const std::map<int, Channel::MemberState>& members = pChan->getMembers();
			std::map<int, Channel::MemberState>::const_iterator membersIt = members.begin();
			for (; membersIt != members.end(); ++membersIt) {
				if (membersIt->first == cli.fd) {
					continue;
				}
				_sendToClient(membersIt->first, message);
				_printLogServer("INFO", cli, sline, targetChannel);
				sendAtLeastOne = true;                
			}
		}
	}
	// if the targe is a user and the user is not connected send RPL_AWAY (301)
	return (sendAtLeastOne);
}

//? Command: KICK 
//? Parameters: <channel> <user> *( "," <user> ) [<comment>]
//? ===> KICK #Finnish Matthew ; Command to kick Matthew from #Finnish
//? ===> KICK #Finnish John :Speaking English; Command to kick John from #Finnish using "Speaking English" as the reason (comment).
//? message <=== :WiZ!jto@tolsun.oulu.fi KICK #Finnish John; KICK message on channel #Finnish from WiZ to remove John from channel
bool	Server::_handleKick(Client& cli, const s_Line& sline) {

	// check params size
	//! ERR_NEEDMOREPARAMS (461)   "<client> <command> :Not enough parameters"
	if (sline.params.size() < 2) {
		_sendErrNeedMoreParams(cli, cli.getNickname(), sline, sline.command);
		return (false);
	}                           	                                                                    

	std::string chanName = sline.params[0];
	// check channel if exist
	//! ERR_NOSUCHCHANNEL (403) "<client> <channel> :No such channel"
	Channel* pChan = getChannel(chanName);
	if (pChan == NULL) {
		_sendErrNoSuchChannel(cli, cli.getNickname(), sline, chanName);
		return (false);
	}
	// check op privileges of the client
	//! ERR_NOTONCHANNEL (442)  "<client> <channel> :You're not on that channel"
	//! ERR_CHANOPRIVSNEEDED (482)  "<client> <channel> :You're not channel operator"
	if (pChan->isMember(cli.fd) == false) {
		_sendErrNotOnChannel(cli, cli.getNickname(), sline, chanName);
		return (false);
	}
	if (pChan->isChanOp(cli.fd) == false) {
		_sendErrChaNoPrivsNeeded(cli, cli.getNickname(), sline, chanName);
		return (false);
	}
	std::string	userParam = sline.params[1];
	std::string userName;
	std::string reason = (sline.params.size() >= 3 ? sline.params[2] : "");
	std::string prefix = ":" + cli.getNickname() + "!" + cli.getUsername() + "@" + cli.ipAddr;

	while (!userParam.empty()) {
		std::size_t commaPos = userParam.find(',');
		if (commaPos == std::string::npos) {
			userName = userParam;
			userParam.clear();
		}
		else {
			userName = userParam.substr(0, commaPos);
			userParam.erase(0, commaPos + 1);
		}
		if (userName.empty()) {
			continue;
		}
		// check user in channel 
		//! ERR_USERNOTINCHANNEL (441)   "<client> <nick> <channel> :They aren't on that channel"
		Client* user = getClientByNick(userName);
		if (user == NULL) {
			continue;
		}
		if (pChan->isMember(user->fd) == false) {
			_sendErrNotOnChannel(cli, user->getNickname(), sline, chanName);
			continue;
		}
		const std::map<int, Channel::MemberState>& membList = pChan->getMembers();
		std::map<int, Channel::MemberState>::const_iterator membIt = membList.begin();
		std::string message = CYAN + prefix + " " + sline.command + " " + chanName + " " + user->getNickname();
		if (!reason.empty()) {
			message += " reason:" + std::string(RED) + "(" + reason + ")" + std::string(RESET);
		}
		for (; membIt != membList.end(); ++membIt) {
			_sendToClient(membIt->first, message);
		}
		pChan->removeMember(user->fd);
		user->removeSubscribedChannel(pChan->getName());
		_printLogServer("INFO", cli, sline, user->getNickname());
		if (pChan->empty()) {
			_channelList.erase(pChan->getName());
			break;
		}
	}
	return (true);
}


//? Command: INVITE
//? Parameters: <nickname> <channel>
//? ====> INVITE Wiz #foo_bar    ; Invite Wiz to #foo_bar
//? message <==== :dan-!d@localhost INVITE Wiz #test    ; dan- has invited Wiz to the channel #test
bool	Server::_handleInvite(Client& cli, s_Line& sline) {

	std::string	nick = cli.getNickname();

	if (nick.empty()) {
		nick = "*";
	}
	// check if 2 parameters
	//! ERR_NEEDMOREPARAMS (461) 
	if (sline.params.size() < 2) {
		if (sline.params.empty()) {
			//! send RPL_INVITELIST (336) numerics, ending with a RPL_ENDOFINVITELIST (337)
			std::map<std::string, Channel>::iterator chanIt = _channelList.begin();
			for (; chanIt != _channelList.end(); ++chanIt) {
				if (chanIt->second.isInvited(cli.fd)) {
					sline.params.push_back(chanIt->first);
				}
			}
			_sendRplInviteList(cli, nick, sline, "");
			_sendEndOfInviteList(cli, nick, sline, "");
			return (true);
		}
		else {
			_sendErrNeedMoreParams(cli, nick, sline, sline.command);
			return (false);
		}
	}

	std::string	targetUser = sline.params[0];
	std::string chanName = sline.params[1];

	// check if the channel exist
	//! ERR_NOSUCHCHANNEL (403)
	Channel*	pChan = getChannel(chanName);
	if (pChan == NULL) {
		_sendErrNoSuchChannel(cli, nick, sline, chanName);
		return (false);
	}

	// check if the client is member of the channel
	//! ERR_NOTONCHANNEL (442)
	if (pChan->isMember(cli.fd) == false) {
		_sendErrNotOnChannel(cli, nick, sline, chanName);
		return (false);
	}

	// if the mode Invite_only is activated check if the client have the operator privileges
	//! ERR_CHANOPRIVSNEEDED (482)
	if (pChan->isMode("i") && !pChan->isChanOp(cli.fd)) {
		_sendErrChaNoPrivsNeeded(cli, nick, sline, chanName);
		return (false);
	}

	// check if the user <nickname> is already member of the channel
	//! ERR_USERONCHANNEL (443)
	
	Client* pUser = getClientByNick(targetUser);
	if (pUser == NULL) {
		_sendErrNoSuchNick(cli, nick, sline, targetUser);
		return (false);
	}
	if (pChan->isMember(pUser->fd)) {
		_sendErrUserOnChannel(cli, nick, sline, chanName);
		return (false);
	}

	pChan->addInvite(pUser->fd);
	// send RPL_INVITING (341)  "<client> <nick> <channel>" to the issuer 
	_sendRplInviting(cli, nick, sline, chanName);
	
	// send INVITE message to the invited user with issuer as <source>
	std::string prefix = ":" + nick + "!" + cli.getUsername() + "@" + cli.ipAddr;
	std::string message = prefix + " INVITE " + targetUser + " " + chanName;
	_printLogServer("INFO", cli, sline, chanName);
	_sendToClient(pUser->fd, message);
	
	return (true);
}