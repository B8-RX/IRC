#include "Client.hpp"
#include <iostream>
#include <string>

Client::Client(void) {}
Client::~Client(void) {}

void            Client::setClientFd(int fd) {
    _fd = fd;
}

void            Client::setClientIpAddr(const std::string& ip) {
    _ipAddr = ip;
}

int             Client::getClientFd(void) {
    return (_fd);
}

std::string&    Client::getClientIpAddr(void) {
    return (_ipAddr);
}