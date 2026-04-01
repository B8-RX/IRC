#include "Client.hpp"
#include <iostream>
#include <string>

Client::Client(void) : _hasNick(false), _hasUser(false), _passAccepted(false), _registered(false) {}
Client::~Client(void) {}