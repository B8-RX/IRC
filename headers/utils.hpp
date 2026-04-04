#ifndef UTILS_HPP
# define UTILS_HPP

#include "Server.hpp"
#include "Client.hpp"

void    printLine(const Server::s_Line& sLine);
void    printClient(const Client& client);
bool    isValidNick(const std::string& nick);
bool	isUsedNick(std::map<int, Client>& ClientsList, const std::string& nick, int clientFd);

#endif // !UTILS_HPP