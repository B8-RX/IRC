#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sstream>

void    Server::_printLine(const Server::s_Line& sLine) const {
	std::cout << "raw line: [" << sLine.raw << "]\n";
	std::cout << "prefix: [" << (sLine.prefix.empty() ? "" : sLine.prefix) << "]\n";
	std::cout << "command: [" << (sLine.command.empty() ? "" : sLine.command) << "]\n";		
	for (std::size_t i = 0; i < sLine.params.size(); ++i) {
		std::cout << "param:[" << sLine.params[i] << "]\n";
	}		
	std::cout << "\n";

}

void	Server::_printLogServer(const std::string& type, const Client& cli, const s_Line& sline, const std::string& info) const {
	std::string message;
	std::string color = (type == "INFO") ? GREEN : (type == "ERROR") ? RED : RESET;
	if (type == "INFO") {
		message = _generateLogInfo(sline, cli, info);
	}
	else {
		message = info;
	}
	if (sline.command == "PRIVMSG" && type == "INFO") {
		color = BLUE;
	}
	std::cout << color << "[" << type << "]: " << message << " [END]" << RESET << "\n";
}

std::string	Server::_generateLogInfo(const s_Line& sline, const Client& cli, const std::string& info) const{
	std::string message;

	std::string nickName = cli.getNickname();

	if (nickName.empty()) {
		nickName = "*";
	}

	if (sline.command == "PASS") {
		message = "password accepted !";
	}
	else if (sline.command == "NICK") {
		if (cli.getOldNickname().empty()) {
			message = "nickname " + std::string(YELLOW) + nickName + std::string(GREEN) + " was created"; 
		} 
		else {
			message = std::string(YELLOW) + cli.getOldNickname() + std::string(GREEN) + " changed her nickname to " + std::string(YELLOW) + sline.params[0] + std::string(GREEN);
		}
	}
	else if (sline.command == "USER") {
		if (cli.hasNick) {
			message = std::string(YELLOW) + nickName + " added her username " + std::string(YELLOW) + sline.params[0] + std::string(GREEN); 
		} 
		else {
			message = "username " + std::string(YELLOW) + sline.params[0] + std::string(GREEN) + " was created"; 
		}
	}
	else if (sline.command == "PING") {
			message = std::string(YELLOW) + nickName + std::string(GREEN) + "@" + cli.ipAddr + " send PING" + std::string(GREEN) ; 
	}
	else if (sline.command == "JOIN") {
		const std::map<std::string, Channel>::const_iterator chanIt = _channelList.find(info);
		std::string sujet = " join channel ";
		if (chanIt != _channelList.end() && (chanIt->second.memberCount() == 1)) {
			sujet = " created channel ";
		}
		message =  std::string(YELLOW) + nickName + std::string(GREEN) + sujet + std::string(YELLOW)  + info + std::string(GREEN); 
	}
	else if (sline.command == "QUIT") {
		message = std::string(RESET) + nickName + std::string(GREEN) + " disconnected" + std::string(GREEN); 
	}
	else if (sline.command == "PART") {
		message = std::string(YELLOW) + nickName + std::string(GREEN) + " unsubscribed from channel " + std::string(YELLOW) + info + std::string(GREEN); 
	}
	else if (sline.command == "PRIVMSG") {
		message = nickName + " send message to " + info; 
	}
	else if (sline.command == "KICK") {
		std::string reason = (sline.params.size() >= 3 ? sline.params[2] : "");
		message = std::string(YELLOW) + nickName + std::string(GREEN) + " kicked " + std::string(RED) + info + std::string(GREEN) + " from channel " +  std::string(YELLOW) + sline.params[0] + std::string(GREEN);
		if (!reason.empty()) {
			message += " reason:" + std::string(RED) + "(" + reason + ")" + std::string(GREEN);
		}
	}	
	else if (sline.command == "INVITE") {
		std::string invitedUser = sline.params[0];
		std::string chanName = sline.params[1];
		message = std::string(YELLOW) + nickName + std::string(GREEN) + " invited " + std::string(YELLOW) + invitedUser + std::string(GREEN) + " to channel " + std::string(YELLOW) + chanName + std::string(GREEN); 
	}
	else if (sline.command == "TOPIC") {
		std::string chanName = info;
		std::string sujet = " requested topic info of the channel ";
		if (sline.params.size() > 1) {
			sujet = " edit the topic of the channel ";
		}
		message = std::string(YELLOW) + nickName + std::string(GREEN) + sujet + std::string(YELLOW) + chanName + std::string(GREEN);  
	}
	return (message);
}

void    Server::_printClient(const Client& client) const {
	std::cout << "|| Client Info ||\n";
	std::cout << "client fd: [" << client.fd << "]\n";
	std::cout << "client nickname: [" << (client.getNickname().empty() ? "" : client.getNickname()) << "]\n";
	std::cout << "client username: [" << (client.getUsername().empty() ? "" : client.getUsername()) << "]\n";
	std::cout << "client realname: [" << (client.getRealname().empty() ? "" : client.getRealname()) << "]\n";
	std::cout << "client pass accepted: [" << (client.getPassAccepted() ? "true" : "false") << "]\n";
	std::cout << "client registered: [" << (client.getRegirstered() ? "true" : "false") << "]\n";
	std::cout << "Channels subscription: [";
	if (!client.getSubscribedChannels().empty()) {
		std::vector<std::string> channels = client.getSubscribedChannels();
		for (std::size_t i = 0; i < channels.size(); ++i)
		std::cout << channels[i] << (i + 1 < channels.size() ? ", " : "");
	}
	std::cout << "]\n";
	std::cout << "buffer in: [" << client.bufferIn << "]\n";
	std::cout << "|| || || ||\n";
}

void	Server::_printChannel(Channel& channel) const {
	std::cout << "\n|| Channel Info ||\n";
	std::cout << "channel name: [" << (channel.getName().empty() ? "" : channel.getName()) << "]\n";
	std::cout << "Channels members: [";
	std::map<int, Channel::MemberState>& members = channel.getMembers();
	std::map<int, Channel::MemberState>::const_iterator it = members.begin();
	for (std::size_t i = 0; it != members.end(); ++it, ++i) {
		std::map<int, Client>::const_iterator itc = _clientList.find(it->first);
		std::cout << ( itc != _clientList.end() ? itc->second.getNickname() : "");
		std::cout << (i + 1 < channel.memberCount() ? ", " : "");
	}
	std::cout << "]\n";
	std::cout << "|| || || ||\n\n";
}

void	Server::_printServerInfo(void) const {
	std::map<int, Client>::const_iterator			 cliIt = _clientList.begin(); 
	std::map<std::string, Channel>::const_iterator chanIt = _channelList.begin();
		std::cout << "\n|| Server Info ||\n";
	std::cout << "Server name = [" << _serverName << "]\n";
	std::cout << "listenning on port = [" << _port << "]\n";
	std::cout << "Client list = [";
	for (std::size_t i = 0; i < _clientList.size() ; ++i, ++cliIt) {
		std::cout << cliIt->second.getNickname() << ((i + 1) < _clientList.size() ? ", " : ""); 
	}
	std::cout << "]\n";
	std::cout << "Channel list = [";
	for (std::size_t i = 0; i < _channelList.size() ; ++i, ++chanIt) {
		std::cout << chanIt->first << ((i + 1) < _channelList.size() ? ", " : ""); 
	}
	std::cout << "]\n";
	std::cout << "|| || || ||\n\n";
}


bool    Server::_isValidNick(const std::string& nick) const {
	std::string	disallowedFirstChar = "0123456789:#"; 
	std::string allowedChar = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}|\\-_.";
	if (disallowedFirstChar.find(nick[0]) != std::string::npos)
		return (false);
	for (std::size_t i = 1; i < nick.size(); ++i) {
		if (allowedChar.find(nick[i]) == std::string::npos)
			return (false);
	}
	return (true);
}

bool    Server::_isUsedNick(std::map<int, Client>& ClientsList, const std::string& nick, int clientFd) const {
	std::map<int, Client>::iterator it = ClientsList.begin();
	std::map<int, Client>::iterator itEnd = ClientsList.end();

	for (; it != itEnd; ++it) {
		if (clientFd != it->first && it->second.hasNick && it->second.getNickname() == nick)
			return (true);
	}
	return (false);
}
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
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + cmd + " :Not enough parameters" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrAlreadyRegistered(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 462 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " :You may not reregister" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}	

void	Server::_sendErrNoNickNameGiven(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 431 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " :No nickname given" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrOnUseNickName(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetNick)  const {
	std::string	numeric = " 432 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + targetNick + " :Erroneus nickname" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNickNameInUse(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetNick)  const {
	std::string	numeric = " 433 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + targetNick + " :Nickname is already in use" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrBadChanMask(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 476 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + chanName + " :Bad Channel Mask" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUnknownCommand(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const {
	std::string	numeric = " 421 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + cmd + " :Unknown command" + std::string(RESET);
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}


void	Server::_sendErrUnregistered(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 451 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " :You have not registered" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrPassMisMatch(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 464 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " :Password incorrect" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNotOnChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 442 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + chanName + " :You're not on that channel" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrUserOnChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 443 ";
	std::string targetUser = sline.params[0];
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + targetUser + " " + chanName + " :is already on channel" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoSuchChannel(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 403 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + chanName + " :No such channel" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoRecipient(Client& cli, const std::string& nick, const s_Line& sline, const std::string& cmd) const {
	std::string	numeric = " 411 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " :No recipient given (" + cmd + ")" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoTextToSend(Client& cli, const std::string& nick, const s_Line& sline, const std::string& placeholder) const {
	(void)placeholder;
	std::string	numeric = " 412 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " :No text to send" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrNoSuchNick(Client& cli, const std::string& nick, const s_Line& sline, const std::string& targetName) const {
	std::string	numeric = " 401 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + targetName + " :No such nick/channel" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrCannotSendToChan(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 404 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + chanName + " :Cannot send to channel" + std::string(RESET); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendErrChaNoPrivsNeeded(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	std::string	numeric = " 482 ";
	std::string line = std::string(RED) + ":" + _serverName + numeric + nick + " " + chanName + " :You're not channel operator" + std::string(RESET); 
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
	std::string line = ":" + _serverName + numeric +  nick + " " + invitedUser + " " + chanName; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplNoTopic(Client& cli, const std::string& nick, const s_Line& sline, const std::string& chanName) const {
	(void)nick;
	std::string	numeric = " 331 ";
	std::string line = ":" + _serverName + numeric + chanName + " :No topic is set"; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplTopic(Client& cli, const std::string& nick, const s_Line& sline, const std::string& topic) const {
	(void)nick;
	std::string numeric = " 332 ";
	std::string line = ":" + _serverName + numeric + sline.params[0] + " :" + topic; 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}

void	Server::_sendRplWhoTime(Client& cli, const std::string& nick, const s_Line& sline, std::time_t time) const {
	(void)nick;
	std::string numeric = " 333 ";
	std::ostringstream oss;
	oss << time;
	std::string line = ":" + _serverName + numeric + sline.params[0] + " " + nick + " :" + oss.str(); 
	_printLogServer("DEBUG", cli, sline, line);
	_sendToClient(cli.fd, line);
}