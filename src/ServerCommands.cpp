#include "libs.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

bool	Server::_dispatchCommand(int clientFd, s_Line& sLine) {
	
	Client* pCli = getClient(clientFd);
	if (pCli == NULL) {
		return (false);
	}
	std::string clientNick = pCli->getNickname();
	if (clientNick.empty()) {
		clientNick = "*";
	}
	if (sLine.command == "CAP") {
		return (_handleCap(*pCli, sLine));
	}
	if (sLine.command == "PASS") {
		if (_passwordEnabled && !_handlePass(*pCli, sLine)) {
			_cleanupClient(pCli->fd, "PASS");
			return (false);
		}
		return (true);
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
		else if (sLine.command == "TOPIC") {
			return (_handleTopic(*pCli, sLine));
		}
		else if (sLine.command == "MODE") {
			return (_handleMode(*pCli, sLine));
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

//? Command:  CAP
//? Parameters:  <subcommand> [:<capabilities>]
//? ===> Server: CAP * LS :

bool	Server::_handleCap(Client& cli, const s_Line& sline) {
	std::string option = sline.params[0];
	std::string prefix = ":" + _serverName;
	std::string message;
	if (option == "LS") {
		std::string nick = cli.getNickname();
		if (nick.empty()) {
			message = prefix + " LS * :";
		}
		else {
			message = prefix + " " + nick + " LS * :";
		}
		_sendToClient(cli.fd, message);
		_printLogServer("DEBUG", cli, sline, message);
	}
	return (true);
}

//? Command:  PASS
//? Parameters:  <password>
//? ===> PASS secretpasswordhere

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
	_printLogServer("INFO", cli, sline, sline.command);
	if (!cli.getRegirstered()) {
		if (_updateRegisteredState(cli.fd)) {
			_sendRplWelcome(cli);
			_sendRplYourHost(cli);
			_sendRplCreated(cli);
			_sendRplMyInfo(cli);
			_printLogServer("INFO", cli, sline, "registered");
		}
	}
	return (true);
}


//? Command:  NICK
//? Parameters:  <nickname>
//? ===> NICK Wiz	; Requesting the new nick "Wiz".
//? message <=== :dan-!d@localhost NICK Mamoped		; dan- changed his nickname to Mamoped.
//? message <=== :WiZ NICK Kilroy	; WiZ changed his nickname to Kilroy.	
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
	
	if (cli.hasNick) {
		// send NICK message to all the members that shares a channel with the issuer
		cli.setOldNickname(cli.getNickname());
		
		std::string					oldNick = cli.getOldNickname().empty() ? "*" : cli.getOldNickname();
		std::string					newNick = sline.params[0].empty() ? "*" : sline.params[0];
		std::string					username = cli.getUsername().empty() ? "*" : cli.getUsername(); 
		std::string					prefix = ":" + oldNick + "!" + username + "@" + cli.ipAddr;
		std::string					message = prefix + " NICK " + newNick;
		
		std::vector<std::string>	cliChannels = cli.getSubscribedChannels();
		if (!cliChannels.empty()) {
			_notifyMembersOfAllChannels(cliChannels, cli.fd, message, sline.command);
		}
	}

	cli.setNickname(sline.params[0]);
	cli.hasNick = true;

	
	_printLogServer("INFO", cli, sline, sline.command);
	if (!cli.getRegirstered()) {
		if (_updateRegisteredState(cli.fd)) {
			_sendRplWelcome(cli);
			_sendRplYourHost(cli);
			_sendRplCreated(cli);
			_sendRplMyInfo(cli);
			_printLogServer("INFO", cli, sline, "registered");
		}
	}
	return (true);
}


//? Command: USER
//? Parameters:  <username> 0 * <realname>
//? ===> USER guest 0 * :Ronnie Reagan ; User gets registered with username "~guest" and real name "Ronnie Reagan"
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
	std::string	username = sline.params[0];
	if (username.size() > 12) {
		 username =	username.substr(0, 12);
	}

	// execution

	cli.setUsername(( "~" + username));
	cli.setRealname(sline.params[3]);
	cli.hasUser = true;
	_printLogServer("INFO", cli, sline, sline.command);
	if (!cli.getRegirstered()) {
		if (_updateRegisteredState(cli.fd)) {
			_sendRplWelcome(cli);
			_sendRplYourHost(cli);
			_sendRplCreated(cli);
			_sendRplMyInfo(cli);
			_printLogServer("INFO", cli, sline, "registered");
		}
	}
	return (true);
}


//? Command:  JOIN
//? Parameters:  <channel>{,<channel>} [<key>{,<key>}]
//? ===> JOIN #foobar 	; join channel #foobar.
//? ===> JOIN #foo,#bar fubar,foobar	; join channel #foo using key "fubar". and channel #bar using key "foobar".	
//? message <=== :WiZ JOIN #Twilight_zone        ; WiZ is joining the channel #Twilight_zone
//? message <===  :dan-!d@localhost JOIN #test    ; dan- is joining the channel #test

bool	Server::_handleJoin(Client& cli , const s_Line& sline) {
	
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
	std::string keyParam;
	if (sline.params.size() > 1) {
		keyParam = sline.params[1];
	}
	while (!chanParam.empty()) {
		std::size_t commaPos = chanParam.find(',');
		std::string	channelName;
		if (commaPos == std::string::npos)
		{
			channelName = chanParam;
			chanParam.clear();
		}
		else {
			channelName = chanParam.substr(0, commaPos);
			chanParam.erase(0, commaPos + 1);
		}
		if (channelName.empty()) {
			continue;
		}
		if (!_isValidChannelName(channelName)) {
			_sendErrBadChanMask(cli, clientNick, sline, channelName);
			continue;
		}
		if (_channelList.find(channelName) == _channelList.end()) {
			_channelList.insert(std::make_pair(channelName, Channel(channelName)));
		}
		Channel* pChan = getChannel(channelName);
		if (pChan == NULL) {
			continue;
		} 

		bool		isFirstMember = (pChan->memberCount() == 0);
		
		if (pChan->isMember(cli.fd)) {
			std::cerr << "channel [" << pChan->getName() << "]: Client [" << cli.getNickname() << "] is already member of the channel!" << "\n";
			return (false);
		}
		// check if mode invite-only is enabled and if true check if the client is invited
		//!	ERR_INVITEONLYCHAN (473)
		if (pChan->isMode("i") && !pChan->isInvited(cli.fd)) {
			_sendErrInviteOnlyChan(cli, clientNick, sline, channelName);
			return (false);
		}
		// check if mode client limit is enabled and if true check if the limit is reached
		//!	ERR_CHANNELISFULL (471) 
		if (pChan->isMode("l") && pChan->memberCount() >= pChan->getLimitChannel()) {
			_sendErrChanIsFull(cli, clientNick, sline, channelName);
			return (false);
		}
		// check if mode key is enabled and if the key is incorrect or not supplied
		//!	ERR_BADCHANNELKEY (475)
		if (pChan->isMode("k")) {
			std::size_t commaPos = keyParam.find(',');
			std::string	keyName;
			if (commaPos == std::string::npos)
			{
				keyName = keyParam;
				keyParam.clear();
			}
			else {
				keyName = keyParam.substr(0, commaPos);
				keyParam.erase(0, commaPos + 1);
			}
			if (keyName != pChan->getKey()) {
			   _sendErrBadChannelKey(cli, clientNick, sline, channelName);
			   return (false);
			}
		}

		if (!pChan->addMember(cli.fd, isFirstMember)) {
			std::string message = "Cannot add member " + cli.getNickname() + " to the channel " + pChan->getName();
			_printLogServer("DEBUG", cli, sline, message);
			return (false);
		}
		if (!cli.isMemberChan(channelName)) {
			cli.addSubscriptionChan(channelName);
		}
		std::string	prefix = ":" + cli.getNickname() + "!" + cli.getUsername() + "@" + _serverName;
		std::string msg = prefix + " JOIN " + channelName;
		std::vector<int>	membersList = pChan->getMembersList();
		_printLogServer("INFO", cli, sline, channelName);
		_notifyMembersSingleChan(membersList, cli.fd, msg);
		_sendRplTopic(cli, clientNick, sline, pChan->getTopic()); 
	}
	return (true);
}


//? Command:  PART
//? Parameters:  <channel>{,<channel>} [<reason>]
//? ===>  PART #twilight_zone 		; leave channel "#twilight_zone"
//? ===>  PART #oz-ops,&group5 		; leave both channels "&group5" and "#oz-ops".
//? message <=== :dan-!d@localhost PART #test			; dan- is leaving the channel #test

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
		std::size_t	commaPos = chanParam.find(',');
		std::string channelName;
		if (commaPos == std::string::npos)
		{
			channelName = chanParam;
			chanParam.clear();
		}
		else {
			channelName = chanParam.substr(0, commaPos);
			chanParam.erase(0, commaPos + 1);
		}
		if (channelName.empty())
			continue;
		std::string prefix = ":" + clientNick + "!" + cli.getUsername() + "@" + cli.ipAddr;
		std::string message = prefix + " " + sline.command + " " + channelName;
		if (!reason.empty()) {
			message += (" :"  + reason);
		}
		Channel*	pChan = getChannel(channelName);
		if (pChan != NULL) {
			if (pChan->isMember(cli.fd)) {
				const std::map<int, Channel::MemberState>&	chanMembers = pChan->getMembers();
				std::map<int, Channel::MemberState>::const_iterator membersIt = chanMembers.begin();
				bool	foundChanOp = false;
				for (; membersIt != chanMembers.end(); ++membersIt) {
					if (!foundChanOp && membersIt->first != cli.fd && membersIt->second.isChanOp == true) {
						foundChanOp = true;
					}
					_sendToClient(membersIt->first, message);
				}
				pChan->removeMember(cli.fd);
				cli.removeSubscribedChannel(channelName);
				_printLogServer("INFO", cli, sline, channelName);

				// if the channel is not empty and there are no operator, give the status to the first member
				if (pChan->memberCount() > 0 && !foundChanOp) {
					int newOpFd = -1;
					std::string	newOpNick;
					std::string msg = ":" + _serverName + " MODE " + channelName + " +o ";
					std::map<int, Channel::MemberState>&	members = pChan->getMembers();
					std::map<int, Channel::MemberState>::iterator opMemberIt = members.begin();
					newOpFd = opMemberIt->first;
					pChan->updateMemberState(newOpFd, true);
					newOpNick = getClient(newOpFd)->getNickname();
					msg += newOpNick;
					for (; opMemberIt != members.end(); ++opMemberIt) {
						_sendToClient(opMemberIt->first, msg);
					}
				}
				if (pChan->empty()) {
					_channelList.erase(_channelList.find(channelName));
					std::cout << "channel [" << channelName << "] have 0 member. Channel have been deleted successfully.\n";
				} 
			}
			else {
				_sendErrNotOnChannel(cli, clientNick, sline, channelName);
			}
		}
		else {
			_sendErrNoSuchChannel(cli, clientNick, sline, channelName);
		}
	}
	return (true);
}


//? Command:  QUIT
//? Parameters:  [<reason>]
//? ===> QUIT :Gone to have lunch		; Client exiting from the network
//? message <=== :dan-!d@localhost QUIT :Quit: Bye for now!			; dan- is exiting the network with the message: "Quit: Bye for now!"

bool	Server::_handleQuit(Client& cli, const s_Line& sline) {

	
	// construct the notification message
	std::string					nick = cli.getNickname().empty() ? "*" : cli.getNickname();
	std::string					username = cli.getUsername().empty() ? "*" : cli.getUsername(); 
	std::string					prefix = ":" + nick + "!" + username + "@" + cli.ipAddr;
	std::string					reason = (sline.params.empty() ? "" : sline.params[0]); 
	std::string					message =  prefix + " " + sline.command + " :Quit:" + (reason.empty() ? "" : " " + reason);

	// send QUIT message to all the members that shares a channel with the issuer
	std::vector<std::string>	cliChannels = cli.getSubscribedChannels();
	if (!cliChannels.empty()) {
		_notifyMembersOfAllChannels(cliChannels, cli.fd, message, sline.command);
	}
	// send a QUIT acknowledgement to the client
	message = "ERROR :Closing Link: Quit:";
	if (!reason.empty()) {
		message += " ";
		message += reason;
	}
	_sendToClient(cli.fd, message);
	_printLogServer("INFO", cli, sline, sline.command);
	
	// clean the client from the server and close his socket
	_cleanupClient(cli.fd, sline.command);
	return (true);
}


//? Command:  PING
//? Parameters:  <token>

bool	Server::_handlePing(Client& cli, const s_Line& sline) {

	std::string clientNick = (cli.getNickname().empty() ? "*" : cli.getNickname());
	
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
		return (false);
	}
	std::string message = ":" + _serverName + " PONG " + _serverName + " " + sline.params[0];
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
	

	// validation 
	if (sline.params.empty()) {
		_sendErrNoRecipient(cli, clientNick, sline, sline.command);
		return (false);
	}
	if (sline.params.size() < 2 || sline.params[1].empty()) {
		_sendErrNoTextToSend(cli, clientNick, sline, "");
		return (false);
	}

	//construct the message prefix and crop the text to sent if > 400 characters
	std::string targetParam = sline.params[0];
	std::string prefix = ":" + clientNick + "!" + clientUsername + "@" + cli.ipAddr;	
	std::string privMsg = sline.params[1];
	if (privMsg.size() > 400) {
		privMsg = privMsg.substr(0, 400);
	}

	// execution
	// to handle multi target ex: #chan1,#chan2... split on every comma ',' until the end
	bool		sendAtLeastOne = false;
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
		std::string message = prefix + " PRIVMSG " + targetName + " :" + privMsg;
		
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
			// check if the channel exist
			Channel* pChan = getChannel(targetName); 
			std::string	targetChannel = targetName;
			if (pChan == NULL) {
				_sendErrNoSuchChannel(cli, clientNick, sline, targetChannel);
				continue;
			}
			// check if client is member of the channel
			if (!pChan->isMember(cli.fd)) {
				_sendErrCannotSendToChan(cli, clientNick, sline, targetChannel);
				continue;
			}
			// send the message on the channel
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
	return (sendAtLeastOne);
}

//? Command: KICK 
//? Parameters: <channel> <user> *( "," <user> ) [<comment>]
//? ===> KICK #Finnish Matthew ; Command to kick Matthew from #Finnish
//? ===> KICK #Finnish John :Speaking English; Command to kick John from #Finnish using "Speaking English" as the reason (comment).
//? message <=== :WiZ!jto@tolsun.oulu.fi KICK #Finnish John; KICK message on channel #Finnish from WiZ to remove John from channel

bool	Server::_handleKick(Client& cli, const s_Line& sline) {
	std::string	clientNick = cli.getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}
	// check params size
	//! ERR_NEEDMOREPARAMS (461)   "<client> <command> :Not enough parameters"
	if (sline.params.size() < 2) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
		return (false);
	}                           	                                                                    

	std::string channelName = sline.params[0];
	// check channel if exist
	//! ERR_NOSUCHCHANNEL (403) "<client> <channel> :No such channel"
	Channel* pChan = getChannel(channelName);
	if (pChan == NULL) {
		_sendErrNoSuchChannel(cli, clientNick, sline, channelName);
		return (false);
	}
	// check op privileges of the client
	//! ERR_NOTONCHANNEL (442)  "<client> <channel> :You're not on that channel"
	//! ERR_CHANOPRIVSNEEDED (482)  "<client> <channel> :You're not channel operator"
	if (pChan->isMember(cli.fd) == false) {
		_sendErrNotOnChannel(cli, clientNick, sline, channelName);
		return (false);
	}
	if (pChan->isChanOp(cli.fd) == false) {
		_sendErrChaNoPrivsNeeded(cli, clientNick, sline, channelName);
		return (false);
	}
	std::string	userParam = sline.params[1];
	std::string userName;
	std::string reason = (sline.params.size() >= 3 ? sline.params[2] : "");
	std::string prefix = ":" + clientNick + "!" + cli.getUsername() + "@" + cli.ipAddr;

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
			_sendErrNotOnChannel(cli, user->getNickname(), sline, channelName);
			continue;
		}
		const std::map<int, Channel::MemberState>& membList = pChan->getMembers();
		std::map<int, Channel::MemberState>::const_iterator membIt = membList.begin();
		std::string message = prefix + " " + sline.command + " " + channelName + " " + user->getNickname();
		if (!reason.empty()) {
			message += " reason:(" + reason + ")";
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

	std::string	clientNick = cli.getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
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
			_sendRplInviteList(cli, clientNick, sline, "");
			_sendEndOfInviteList(cli, clientNick, sline, "");
			return (true);
		}
		else {
			_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
			return (false);
		}
	}

	std::string	targetUser = sline.params[0];
	std::string channelName = sline.params[1];

	// check if the channel exist
	//! ERR_NOSUCHCHANNEL (403)
	Channel*	pChan = getChannel(channelName);
	if (pChan == NULL) {
		_sendErrNoSuchChannel(cli, clientNick, sline, channelName);
		return (false);
	}

	// check if the client is member of the channel
	//! ERR_NOTONCHANNEL (442)
	if (pChan->isMember(cli.fd) == false) {
		_sendErrNotOnChannel(cli, clientNick, sline, channelName);
		return (false);
	}

	// if the mode Invite_only is activated check if the client have the operator privileges
	//! ERR_CHANOPRIVSNEEDED (482)
	if (pChan->isMode("i") && !pChan->isChanOp(cli.fd)) {
		_sendErrChaNoPrivsNeeded(cli, clientNick, sline, channelName);
		return (false);
	}

	// check if the user <nickname> is already member of the channel
	//! ERR_USERONCHANNEL (443)
	
	Client* pUser = getClientByNick(targetUser);
	if (pUser == NULL) {
		_sendErrNoSuchNick(cli, clientNick, sline, targetUser);
		return (false);
	}
	if (pChan->isMember(pUser->fd)) {
		_sendErrUserOnChannel(cli, clientNick, sline, channelName);
		return (false);
	}

	pChan->addInvite(pUser->fd);
	//? send RPL_INVITING (341)  "<client> <nick> <channel>" to the issuer 
	_sendRplInviting(cli, clientNick, sline, channelName);
	
	//? Broadcast INVITE message to the invited user with issuer as <source>
	std::string prefix = ":" + clientNick + "!" + cli.getUsername() + "@" + cli.ipAddr;
	std::string message = prefix + " INVITE " + targetUser + " " + channelName;
	_printLogServer("INFO", cli, sline, channelName);
	_sendToClient(pUser->fd, message);
	
	return (true);
}

//? Command: TOPIC
//? Parameters: <channel> [<topic>]
//? ===> TOPIC #test :New topic		; Setting the topic on "#test" to "New topic".
//? ===> TOPIC #test :				; Clearing the topic on "#test"
//? ===> TOPIC #test				; Checking the topic for "#test"

bool	Server::_handleTopic(Client& cli, s_Line& sline) {
	std::string	clientNick = cli.getNickname();

	if (clientNick.empty()) {
		clientNick = "*";
	}

	// check if there is minimum 1 params
	//!  ERR_NEEDMOREPARAMS (461) 
	if (sline.params.empty()) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
		return (false);
	}

	// check if the channel exist
	//! EER_NOSUCHCHANNEL (403)
	std::string	channelName = sline.params[0];
	Channel*	pChan = getChannel(channelName);

	if (pChan == NULL) {
		_sendErrNoSuchChannel(cli, clientNick, sline, channelName);
		return (false);
	}

	// check if the issuer is on the channel
	//!  ERR_NOTONCHANNEL (442) 
	if (!pChan->isMember(cli.fd)) {
		_sendErrNotOnChannel(cli, clientNick, sline, channelName);
		return (false);
	}


	// if no [<topic>] is given, send RPL_TOPIC or RPL_NOTOPIC
	//?  RPL_NOTOPIC (331) /  RPL_TOPIC (332) 

	if (sline.params.size() == 1) {
		if (pChan->getTopic().empty()) {
			_sendRplNoTopic(cli, clientNick, sline, channelName);
		}
		else {
			_sendRplTopic(cli, clientNick, sline, pChan->getTopic());
			_sendRplWhoTime(cli, clientNick, sline, pChan->getTopicStruct());
		}
		_printLogServer("INFO", cli, sline, channelName);
	}
	else {
		// If the client want to modifiy the topic (sline.params[1]), check if the protected mode is enable "t", if yes check if the issuer is an operator
		//!  ERR_CHANOPRIVSNEEDED (482) 
		if (pChan->isMode("t") && !pChan->isChanOp(cli.fd)) {
			_sendErrChaNoPrivsNeeded(cli, clientNick, sline, channelName);
			return (false);
		}
		// crop the topic if > 400 characters
		std::string topic = sline.params[1];
		if (topic.size() > 400) {
			topic = topic.substr(0, 400);
		}
		// set/update the topic, author and creation date
		pChan->setTopic(topic);
		pChan->setTopicAuthor(clientNick);
		pChan->setTopicTimestamp();
		_printLogServer("INFO", cli, sline, channelName);

		// Broadcast a TOPIC command to every client on the channel also for the author
		std::string prefix = ":" + clientNick + "!" + cli.getUsername() + "@" + cli.ipAddr;
		std::string message = prefix + " TOPIC " + channelName + " " + topic;
		const std::map<int, Channel::MemberState>&	members = pChan->getMembers();
		std::map<int, Channel::MemberState>::const_iterator membersIt = members.begin();
		for (; membersIt != members.end(); ++membersIt) {
			_sendToClient(membersIt->first, message);
		}  
	}
	return (true);
}

//? Command:  MODE
//? Parameters:  <target> [<modestring> [<mode arguments>...]]
//? ===> MODE #chan1 +i			; Setting/Remove the "invite-only" mode on channel #chan1.
//? ===> MODE #chan1			; Getting modes for a channel (and channel creation time):
//? message <=== :dan!~h@localhost MODE #foobar -l+i  ; dan removed the client limit from, and set the #foobar channel to invite-only.
//? message <=== :irc.example.com MODE #foobar +o bunny		; The irc.example.com server gave channel operator privileges to bunny on #foobar.

bool	Server::_handleMode(Client& cli, s_Line& sline) {
	std::string	clientNick = cli.getNickname();
	if (clientNick.empty()) {
		clientNick = "*";
	}
	// check and handle if its user Mode
	if (sline.params[0][0] != '#') {
		//check if not owner nick
		//! ERR_USERSDONTMATCH (502)
		if (clientNick != sline.params[0]) {
			_sendErrUserDontMatch(cli, clientNick, sline, "");
			return (false);
		}
		//! RPL_UMODEIS (221)
		_sendRplUmodeIs(cli);
		return (false);
	}
	
	// check if param minimum is 1
	//!	ERR_NEEDMOREPARAMS (461) 
	if (sline.params.size() < 1) {
		_sendErrNeedMoreParams(cli, clientNick, sline, sline.command);
		return (false);
	}
	std::string	channelName = sline.params[0];
	// check if channel exist
	//!	ERR_NOSUCHCHANNEL (403)
	Channel*	pChan = getChannel(channelName);
	if (pChan == NULL) {
		_sendErrNoSuchChannel(cli, clientNick, sline, channelName);
		return (false);
	}

	// check if the issuer is on the channel
	//!  ERR_NOTONCHANNEL (442) 
	if (!pChan->isMember(cli.fd)) {
		_sendErrNotOnChannel(cli, clientNick, sline, channelName);
		return (false);
	}

	std::string	modestring;
	if (sline.params.size() > 1) {
		modestring = sline.params[1];
	}
	std::size_t paramKeyPos = std::string::npos;
	// check if <modestring> (sline.params[1]) is not given
	//!	RPL_CHANNELMODEIS (324)
	//!	RPL_CREATIONTIME (329)
	if (modestring.empty()) {
		modestring = pChan->buildModeString();
		if (pChan->isMode("k") && pChan->isChanOp(cli.fd)) {
			modestring += (" " + pChan->getKey());
		}
		if (pChan->isMode("l")) {
			modestring += " ";
			modestring += pChan->getLimitChannel();
		}
		_sendRplChannelModeIs(cli, clientNick, sline, modestring);
		_sendRplCreationTime(cli, clientNick, sline, pChan->getCreationTime());
	}
	else {
		// check the user have appropriate privileges to change modes given
		//!	ERR_CHANOPRIVSNEEDED (482) 
		if (!pChan->isChanOp(cli.fd)) {
			_sendErrChaNoPrivsNeeded(cli, clientNick, sline, channelName);
			return (false);
		}
		std::size_t	paramIndex = 2;
		Channel::s_mode	smode;
		smode.sign = '\0';
		for (std::size_t i = 0; i < modestring.size(); ++i) {
			smode.mode = modestring[i];
			if (smode.mode == '+') {
				smode.add = true;
				smode.minus = false;
			}
			else if (smode.mode == '-') {
				smode.add = false;
				smode.minus = true;
			}
			else {
				smode.param.clear();
				if (sline.params.size() >= (paramIndex + 1)) {
					smode.param = sline.params[paramIndex];
				}
				paramIndex += _handleSingleMode(smode, *pChan, cli, sline);	
				if (smode.add && smode.mode == 'k') {
						paramKeyPos = i - 1;
				}
				smode.sign = (smode.add ? '+' : smode.minus ? '-' : '\0');
			}
		}
		
		// send to members the changes if there is consumed mode(s)

		if (!smode.vMode.empty()) {
			// construct the consumed mode(s) 
			modestring.clear();
			for (std::size_t i = 0; i < smode.vMode.size(); ++i) {
				modestring += smode.vMode[i];
			}
	
			// construct the consumed parameter(s) of the consumed mode(s) above
			std::string paramsFull;
			std::string paramsFullChanOp;
			for (std::size_t i = 0; i < smode.vParam.size(); ++i) {
				if (paramKeyPos != std::string::npos && i == paramKeyPos) {
					paramsFullChanOp += (" " + smode.vParam[i]);
					continue;
				}
				paramsFull += (" " + smode.vParam[i]);
				paramsFullChanOp += (" " + smode.vParam[i]);
			}
	
			// construct broadcast message
			std::string prefix = ":" + cli.getNickname() + "!" + cli.getUsername() + "@" + cli.ipAddr;
			std::string message = prefix + " MODE " + channelName + " " + modestring;
			
			std::map<int, Channel::MemberState>& members = pChan->getMembers();
			std::map<int, Channel::MemberState>::const_iterator membIt = members.begin();
	
	
			// notify all channel members of the mode(s) changes 
			bool	foundChanOp = false;
			for (; membIt != members.end(); ++membIt) {
				if (membIt->second.isChanOp) {
					foundChanOp = true;
					_sendToClient(membIt->first, message + paramsFullChanOp);
				}
				else {
					_sendToClient(membIt->first, message + paramsFull);
				}
			}
			if (pChan->memberCount() > 1 && !foundChanOp) {
				std::map<int, Channel::MemberState>::iterator newOpIt = members.begin();
				if (newOpIt != members.end()) {
					if (newOpIt->first == cli.fd) {
						++newOpIt;
					}
					newOpIt->second.isChanOp = true;
					Client* newOp = getClient(newOpIt->first);
					if (newOp) {
						std::string source = ":" + _serverName;
						std::string msg = source + " MODE " + channelName + " +o " + newOp->getNickname();
						std::vector<int>	membList = pChan->getMembersList();
						_notifyMembersSingleChan(membList, -1, msg);
						_printLogServer("INFO", cli, sline, msg);
					}

				}

			}
			std::string messageServerLog = modestring + paramsFullChanOp;
			_printLogServer("INFO", cli, sline, messageServerLog);
		}
	}
	return (true);
}

int	Server::_handleSingleMode(Channel::s_mode& smode, Channel& chan, Client& cli, s_Line& sline) {
	int ret = 0;
	std::string	userName = smode.param;
	switch (smode.mode) {
		case 'o' : {
			if (userName.empty()) {
				_sendErrNeedMoreParams(cli, cli.getNickname(), sline, sline.command);
				return (ret);
			}
			Client* user = getClientByNick(userName);
			if (user == NULL) {
				_sendErrNoSuchNick(cli, cli.getNickname(), sline, userName);
			}
			else {
				if (chan.isMember(user->fd) == false) {
					_sendErrUserNotInChannel(cli, cli.getNickname(), sline, userName);
				}
				else {
					chan.updateMemberState(user->fd, smode.add);
					if (smode.add && (smode.sign != '+')) {
						smode.vMode.push_back('+');
					}
					else if (smode.minus && (smode.sign != '-')) {
						smode.vMode.push_back('-');
					}
					smode.vParam.push_back(smode.param);
					smode.vMode.push_back(smode.mode);
				}
			}
			ret = 1;
		}
		break;
		case 'i' : 
			if (smode.add && (smode.sign != '+')) {
				smode.vMode.push_back('+');
			}
			else if (smode.minus && (smode.sign != '-')) {
				smode.vMode.push_back('-');
			}
			smode.vMode.push_back(smode.mode);
			chan.setInviteOnly(smode.add);
		break;
		case 't' : 
			chan.setTopicRestricted(smode.add);
			if (smode.add && (smode.sign != '+')) {
				smode.vMode.push_back('+');
			}
			else if (smode.minus && (smode.sign != '-')) {
				smode.vMode.push_back('-');
			}
			smode.vMode.push_back(smode.mode);
		break;
		case 'l' : 
			if (smode.add && !smode.param.empty()) {
				std::istringstream iss(smode.param);
				std::size_t limit;
				char c = '\0';
				bool isValid = false;
				if (iss >> limit && !(iss >> c) && limit > 0 && limit != std::string::npos) {
					chan.setLimit(limit);
					isValid = true;
					if (smode.add && (smode.sign != '+')) {
						smode.vMode.push_back('+');
					}
					else if (smode.minus && (smode.sign != '-')) {
						smode.vMode.push_back('-');
					}
				}
				if (isValid) {
					smode.vMode.push_back(smode.mode);
					smode.vParam.push_back(smode.param);
				}
				ret = 1;
			}
			else if (smode.minus && (smode.sign != '-')) {
				smode.vMode.push_back('-');
				smode.vMode.push_back(smode.mode);
				chan.unsetLimit();
			}
			else {
				_sendErrNeedMoreParams(cli, cli.getNickname(), sline, sline.command);
			}	
		break;
		case 'k' : 
			if (smode.add && !smode.param.empty()) {
				chan.setKey(smode.param);
				if (smode.add && (smode.sign != '+')) {
					smode.vMode.push_back('+');
				}
				ret = 1;
				smode.vMode.push_back(smode.mode);
				smode.vParam.push_back(smode.param);
			}
			else if (smode.minus && (smode.sign != '-')) {
				smode.vMode.push_back('-');
				smode.vMode.push_back(smode.mode);
				chan.unsetKey();
			}
			else {
				_sendErrNeedMoreParams(cli, cli.getNickname(), sline, sline.command);
			}
		break;
		default:
			_sendErrUnknownMode(cli, cli.getNickname(), sline, smode.mode);
		break;
	}
	return (ret);
}
	
