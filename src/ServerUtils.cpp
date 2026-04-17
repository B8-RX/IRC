#include "libs.hpp"
#include "Server.hpp"

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
	std::cout << color << "[" << type << "]" << message << " [END]" << RESET << "\n";
}

std::string	Server::_generateLogInfo(const s_Line& sline, const Client& cli, const std::string& info) const{
	std::string message;

	std::string nickName = cli.getNickname();

	if (nickName.empty()) {
		nickName = "*";
	}
	
	if (info == "registered") {
		message = " " + nickName + std::string(YELLOW) + " registration was successful" + std::string(GREEN);  
	}
	else if (sline.command == "PASS") {
		message = "password accepted !";
	}
	else if (sline.command == "NICK") {
		if (cli.getOldNickname().empty()) {
			message = " nickname " + nickName + std::string(YELLOW) + " was created" + std::string(GREEN) ; 
		} 
		else {
			message = " " + cli.getOldNickname() + std::string(YELLOW) + " changed her nickname to " + std::string(GREEN)  + sline.params[0];
		}
	}
	else if (sline.command == "USER") {
		if (cli.hasNick) {
			message = " " + nickName + std::string(YELLOW) + " added her username "  + std::string(GREEN) + sline.params[0]; 
		} 
		else {
			message = " username " + sline.params[0] + std::string(YELLOW) + " was created" + std::string(GREEN) ; 
		}
	}
	else if (sline.command == "PING") {
			message = " " + nickName + "@" + cli.ipAddr + std::string(YELLOW) +  " send PING" + std::string(GREEN) ; 
	}
	else if (sline.command == "JOIN") {
		const std::map<std::string, Channel>::const_iterator chanIt = _channelList.find(info);
		std::string sujet = " join channel ";
		if (chanIt != _channelList.end() && (chanIt->second.memberCount() == 1)) {
			sujet = " created channel ";
		}
		message =  " " + nickName + std::string(YELLOW) + sujet  + std::string(GREEN) + info; 
	}
	else if (sline.command == "QUIT") {
		message = " " + nickName + std::string(YELLOW) + " disconnected" + std::string(GREEN); 
	}
	else if (sline.command == "PART") {
		message = " " + nickName + std::string(YELLOW) + " unsubscribed from channel " + std::string(GREEN) + info; 
	}
	else if (sline.command == "PRIVMSG") {
		message = " " + nickName + std::string(YELLOW) + " send message to " + std::string(GREEN) + info; 
	}
	else if (sline.command == "KICK") {
		std::string reason = (sline.params.size() >= 3 ? sline.params[2] : "");
		message = " " + nickName + std::string(YELLOW) + " kicked " + std::string(GREEN) + info + " from channel " + sline.params[0];
		if (!reason.empty()) {
			message += " reason:" + std::string(RED) + "(" + reason + ")" + std::string(GREEN);
		}
	}	
	else if (sline.command == "INVITE") {
		std::string invitedUser = sline.params[0];
		std::string chanName = sline.params[1];
		message = " " + nickName + std::string(YELLOW) + " invited " + std::string(GREEN) + invitedUser+ " to channel " + chanName; 
	}
	else if (sline.command == "TOPIC") {
		std::string chanName = info;
		std::string sujet = " requested topic info of the channel ";
		if (sline.params.size() > 1) {
			sujet = " edit the topic of the channel ";
		}
		message = " " + nickName + std::string(YELLOW) + sujet + std::string(GREEN) + chanName;  
	}
	else if (sline.command == "MODE") {
		std::string chanName = sline.params[0];
		message = " " + nickName + std::string(YELLOW) + " change mode on channel " + std::string(GREEN) + chanName + " (" + info + ")";  

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
	if (nick.size() > 9) {
		return (false);
	}
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

bool    Server::_isValidChannelName(const std::string& name) const {
	if (name.size() > 50) {
		return (false);
	}
	std::string	disallowedChar = ":# ,\r\n"; 
	if (name.size() <= 1 || name[0] != '#')
		return (false);
	for (std::size_t i = 1; i < name.size(); ++i) {
		if (disallowedChar.find(name[i]) != std::string::npos)
			return (false);
	}
	return (true);
}