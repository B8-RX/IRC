#include "libs.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

void	Server::_sendToClient(int clientFd, const std::string& message) const {
	std::string	line = message + "\r\n";
	ssize_t 	sent = send(clientFd, line.c_str(), line.size(), 0);
	if (sent == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			std::cout << "errno -> EAGAIN/EWOULDBLOCK catched\n";
		}
		std::cerr << "send(): Cannot send to client\n";
		std::cerr << std::string(strerror(errno)) << "\n";
	}
}

void	Server::_sendErrNeedMoreParams(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const {
	std::string	numeric = " 461 ";
	std::string line = ":" + _serverName + numeric + nick + " " + cmd + " :Not enough parameters"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrAlreadyRegistered(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 462 ";
	std::string line = ":" + _serverName + numeric + nick + " :You may not reregister"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}	

void	Server::_sendErrNoNickNameGiven(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 431 ";
	std::string line = ":" + _serverName + numeric + nick + " :No nickname given"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrOnUseNickName(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetNick)  const {
	std::string	numeric = " 432 ";
	std::string line = ":" + _serverName + numeric + nick + " " + targetNick + " :Erroneus nickname"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNickNameInUse(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetNick)  const {
	std::string	numeric = " 433 ";
	std::string line = ":" + _serverName + numeric + nick + " " + targetNick + " :Nickname is already in use"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrBadChanMask(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 476 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :Bad Channel Mask"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUnknownCommand(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const {
	std::string	numeric = " 421 ";
	std::string line = ":" + _serverName + numeric + nick + " " + cmd + " :Unknown command";
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUnregistered(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 451 ";
	std::string line = ":" + _serverName + numeric + nick + " :You have not registered"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrPassMisMatch(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 464 ";
	std::string line = ":" + _serverName + numeric + nick + " :Password incorrect"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNotOnChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 442 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :You're not on that channel"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUserOnChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 443 ";
	std::string targetUser = sline.params[0];
	std::string line = ":" + _serverName + numeric + nick + " " + targetUser + " " + chanName + " :is already on channel"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}


void	Server::_sendErrNoSuchChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 403 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :No such channel"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoRecipient(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const {
	std::string	numeric = " 411 ";
	std::string line = ":" + _serverName + numeric + nick + " :No recipient given (" + cmd + ")"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoTextToSend(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 412 ";
	std::string line = ":" + _serverName + numeric + nick + " :No text to send"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoSuchNick(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetName) const {
	std::string	numeric = " 401 ";
	std::string line = ":" + _serverName + numeric + nick + " " + targetName + " :No such nick/channel"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrCannotSendToChan(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 404 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :Cannot send to channel"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrChanIsFull(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 471 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :Cannot join channel (+l)"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrBadChannelKey(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 475 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :Cannot join channel (+k)"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}	

void	Server::_sendErrInviteOnlyChan(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 473 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :Cannot join channel (+i)"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrChaNoPrivsNeeded(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 482 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :You're not channel operator"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}	


void	Server::_sendRplInviteList(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	inviteChanList = "(";
	for (std::size_t i = 0; i < sline.params.size(); ++i) {
		inviteChanList += sline.params[i];
		if ((i + 1) < sline.params.size()) {
			inviteChanList += ", ";
		}
	}
	inviteChanList += ")";
	std::string	numeric = " 336 ";
	std::string line = ":" + _serverName + numeric + nick + " " + inviteChanList; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendEndOfInviteList(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 337 ";
	std::string line = ":" + _serverName + numeric + nick + " :End of /INVITE list"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplInviting(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 341 ";
	std::string invitedUser = sline.params[0];
	std::string line = ":" + _serverName + numeric + nick + " " + invitedUser + " " + chanName; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplNoTopic(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 331 ";
	std::string line = ":" + _serverName + numeric + nick + " " + chanName + " :No topic is set"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplTopic(Client& cli, const std::string& nick, const s_Line& sline, const std::string& topic) const {
	std::string numeric = " 332 ";
	std::string line = ":" + _serverName + numeric + nick + " " + sline.params[0] + " :" + topic; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplWhoTime(Client& cli, const std::string& nick, const s_Line& sline, const Channel::s_Topic& sTopic) const {
	std::string numeric = " 333 ";
	std::ostringstream oss;
	oss << sTopic.time;
	std::string line = ":" + _serverName + numeric + nick + " " + sline.params[0] + " " + sTopic.topicAuthor + " " + oss.str(); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplChannelModeIs(Client& cli, const std::string& nick, const s_Line& sline, const std::string& modeStringAndParams) const {
	std::string	channelName = sline.params[0];
	std::string numeric = " 324 ";
	std::string line = ":" + _serverName + numeric + nick + " " + channelName + " " + modeStringAndParams;
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplCreationTime(Client& cli, const std::string& nick, const s_Line& sline, const time_t& creationTime) const {
	std::string	channelName = sline.params[0];
	std::string numeric = " 329 ";
	std::ostringstream oss;
	oss << creationTime;
	std::string line = ":" + _serverName + numeric + nick + " " + channelName + " " + oss.str();
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUnknownMode(Client& cli, const std::string& nick, const s_Line& sline, const char mode) const {
	std::string	numeric = " 472 ";
	std::string line = ":" + _serverName + numeric + nick + " " + mode + " :is unknown mode char to me";
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUserNotInChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& userName) const {
	std::string	numeric = " 441 ";
	std::string chanName = sline.params[0];
	std::string line = ":" + _serverName + numeric + nick + " " + userName + " " + chanName + " :They aren't on that channel"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUserDontMatch(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 502 ";
	std::string line = ":" + _serverName + numeric + nick + " :Cant change mode for other users";
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_notifyMembersOfAllChannels(const std::vector<std::string>& cliChannels, int clientFd, const std::string& msg, const std::string& cmd) {
	std::set<int>				listMembersToNotify;
	// loop on the client's channel list to store the fd member to notify (unique broadcast per member)

	for (std::size_t i = 0; i < cliChannels.size(); ++i) {
		std::string			channelName = cliChannels[i];
		std::map<std::string, Channel>::iterator chanIt = _channelList.find(channelName);
		if (chanIt == _channelList.end()) {
			continue;
		}
		Channel&								channel = chanIt->second;
		const std::map<int, Channel::MemberState>& 	members = channel.getMembers();
		std::map<int, Channel::MemberState>::const_iterator it = members.begin();
		for (; it != members.end(); ++it) {
			if (it->first == clientFd && cmd == "QUIT") {
				continue;
			}
			if (listMembersToNotify.find(it->first) == listMembersToNotify.end()) {
				_sendToClient(it->first, msg);
				listMembersToNotify.insert(it->first);
			}
		}
	}
}

void	Server::_notifyMembersSingleChan(const std::vector<int>& memberList, int clientFd, const std::string& msg) {		
	for (std::size_t i = 0; i < memberList.size(); ++i) {
		if (memberList[i] == clientFd) {
			continue;
		}
		_sendToClient(memberList[i], msg);
	}
}

void	Server::_sendRplWelcome(const Client& cli) const {
	std::string	numeric = " 001 ";
	std::string	prefixServer = ":" + _serverName;
	std::string	nick = cli.getNickname();
	std::string	username = cli.getUsername();
	std::string	ipAddr	= cli.ipAddr;
	std::string prefixClient = nick + "!" + username + "@" + ipAddr;
	std::string subject	= " :Welcome to the localhost Network, ";
	std::string message = prefixServer + numeric + nick + subject + prefixClient; 
	
	_sendToClient(cli.fd, message);
}

void	Server::_sendRplYourHost(const Client& cli) const {
	std::string	numeric = " 002 ";
	std::string	prefixServer = ":" + _serverName;
	std::string	nick = cli.getNickname();
	std::string subject	= " :Your host is " + _serverName;
	std::string message = prefixServer + numeric + nick + subject; 
	
	_sendToClient(cli.fd, message);
}

void	Server::_sendRplCreated(const Client& cli) const {
	std::string	numeric = " 003 ";
	std::string	prefixServer = ":" + _serverName;
	std::string	nick = cli.getNickname();
	time_t	creation = getCreationTime();
	char* timeStr = ctime(&creation);
	std::string cleanTime(timeStr);

	if (!cleanTime.empty() && cleanTime[cleanTime.size() - 1] == '\n') {
    	cleanTime.erase(cleanTime.size() - 1);
	}
	std::string subject	= " :This server was created " + cleanTime;
	std::string message = prefixServer + numeric + nick + subject; 
	_sendToClient(cli.fd, message);
}

void	Server::_sendRplMyInfo(const Client& cli) const {
	std::string	numeric = " 004 ";
	std::string	nick = cli.getNickname();
	std::string modes	= " itkol are supported by this server";
	std::string message = ":" + _serverName + numeric + nick + " " + _serverName + modes; 

	_sendToClient(cli.fd, message);

}

void	Server::_sendRplUmodeIs(const Client& cli) const {
	std::string	numeric = " 221 ";
	std::string	nick = cli.getNickname();
	std::string modes	= " itkol are supported by this server";
	std::string message = ":" + _serverName + numeric + "+"; 

	_sendToClient(cli.fd, message);

}