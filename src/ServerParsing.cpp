#include "libs.hpp"
#include "Server.hpp"
#include "Client.hpp"

std::vector<std::string>	Server::_splitCRLF(int clientFd) {

	std::vector<std::string>	vLines;
	std::string					line;
	size_t						posCRLF = std::string::npos;
	Client*						client;

	client = &_clientList[clientFd];
	while ((posCRLF = client->bufferIn.find("\r\n")) != std::string::npos) {
			line = client->bufferIn.substr(0, posCRLF);
			vLines.push_back(line);
			client->bufferIn.erase(0, posCRLF + 2);
	}
	return (vLines);
}

std::string	Server::_spaceTrim(const std::string& str) const {
	std::string	trimmed = str.substr(0);
	std::size_t i = 0;

	for (; i < trimmed.size(); ++i) {
		if (trimmed[i] != ' ')
			break;
	}
	trimmed.erase(0, i);
	if (trimmed.empty())
		return (trimmed);
	for (i = (trimmed.size() - 1); i > 0; --i) {
		if (trimmed[i] != ' ') 
			break;
	}
	std::string	res = trimmed.substr(0, (i + 1));
	return (res);
}

std::string	Server::_handlePrefix(std::string& line) {
	std::string prefix;

	if (!line.empty() && line[0] == ':') {
		std::size_t	pos = line.find(' ');
		if (pos == std::string::npos) {
			prefix = line.substr(1);
			line.erase(0, line.size());
		}
		else {
			prefix = line.substr(0, pos);
			prefix.erase(0, 1);
			line.erase(0, pos);
		}
	}
	return (prefix);	
}

std::string	Server::_handleCommand(std::string& line) {
	std::string command;

	int start = 0;
	
	while (line[start] == ' ')
		++start;
	if (start > 0)
		line.erase(0, start);
	if (!line.empty()) {
		std::size_t	pos = line.find(' ');
		if (pos == std::string::npos) {
			command = line.substr();
			line.erase(0, command.size());
		}
		else {
			command = line.substr(0, pos);
			line.erase(0, pos);
		}
	}
	return (command);	
}

std::vector<std::string>	Server::_handleParams(std::string& line) {
	std::vector<std::string>	params;
	
	while (!line.empty()) {
		int start = 0;
		
		while (line[start] == ' ')
			++start;
		
		if (start > 0)
			line.erase(0, start);
		
		if (line.empty())
			break ;
		
		if (line[0] == ':') {
			start = 1;
			while (line[start] == ' ')
				++start;
			params.push_back(line.substr(start));
			break ;
		}
		
		std::size_t pos = line.find(' ');
		if (pos == std::string::npos) {
			params.push_back(line.substr(0, line.size()));
			break ;
		}
		
		params.push_back(line.substr(0, pos));
		line.erase(0, pos);
	}
	return (params);
}


Server::s_Line	Server::_parseLine(const std::string& line) {
	struct s_Line				sLine;
	if (line.empty())
		return (sLine);
	std::string					lineCpy = _spaceTrim(line.substr(0));
	
	sLine.raw = lineCpy;
	sLine.prefix = _handlePrefix(lineCpy);
	sLine.command = _handleCommand(lineCpy);
	sLine.params = _handleParams(lineCpy);
	return (sLine);
}
