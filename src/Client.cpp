#include "Client.hpp"

Client::Client(void) : hasNick(false), hasUser(false), _passAccepted(false), _registered(false) {}
Client::~Client(void) {}

std::string Client::getNickname(void) const {
    return (_nickname);
}

std::string Client::getUsername(void) const {
    return (_username);
}

std::string Client::getRealname(void) const {
    return (_realname);
}

bool    Client::getPassAccepted(void) const {
    return (_passAccepted);
}

bool    Client::getRegirstered(void) const {
    return (_registered);
}

const std::string& Client::setNickname(const std::string& nick) {
    _nickname = nick;
    hasNick = true;
    return (_nickname);
}

const std::string& Client::setUsername(const std::string& user) {
    _username = user;
    hasUser = true;
    return (_username);
}

const std::string& Client::setRealname(const std::string& name) {
    _realname = name;
    return (_realname);
}

bool    Client::setPassAccepted(bool state) {
    _passAccepted = state;
    return (_passAccepted);
}

bool    Client::setRegirstered(bool state) {
    _registered = state;
    return (_registered);
}

bool    Client::addMemberChan(const std::string& name) {
    _channels.push_back(name); 
    return (true);
}

bool    Client::isMemberChan(const std::string& name) const {
    for (std::size_t i = 0; i < _channels.size(); ++i) {
        if (_channels[i] == name)
            return (true);
    }
    return (false);
}
const std::vector<std::string>	Client::getSubscribedChannels(void) const {
    return (_channels);
}

bool    Client::removeSubscribedChannel(const std::string& target) {
    std::vector<std::string>::iterator it = _channels.begin();

    for (; it != _channels.end(); ++it) {
        if (*it == target) {
            break;
        }
    }
    if (it == _channels.end()) {
        return (false);
    }
    _channels.erase(it);
    return (true);
}