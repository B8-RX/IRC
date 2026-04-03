#include "Client.hpp"
#include <iostream>
#include <string>

Client::Client(void) : hasNick(false), hasUser(false), _passAccepted(false), _registered(false) {}
Client::~Client(void) {}

std::string Client::getNickname(void) const {
    return (_nickname);
}

std::string Client::getUsername(void) const {
    return (_username);
}

bool    Client::getPassAccepted(void) const {
    return (_passAccepted);
}

bool    Client::getRegirstered(void) const {
    return (_registered);
}

std::string Client::setNickname(const std::string& nick) {
    _nickname = nick;
    hasNick = true;
    return (_nickname);
}

std::string Client::setUsername(const std::string& user) {
    _username = user;
    hasUser = true;
    return (_username);
}

bool    Client::setPassAccepted(bool state) {
    _passAccepted = state;
    return (_passAccepted);
}

bool    Client::setRegirstered(bool state) {
    _registered = state;
    return (_registered);
}
