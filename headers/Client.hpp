#ifndef CLIENT_HPP
 #define CLIENT_HPP

 
#include <string>
#include <map>
#include "Channel.hpp"

class Channel;

class Client {
 public:
	Client(void);
	~Client(void);
	
	int                     fd;
	std::string             ipAddr;
	std::string             buffer_in;
	std::string             buffer_out;
	std::string             nickname;
	std::string             username;
	bool                    connected;
	std::map<int, Channel>  channels;
 };

#endif // !CLIENT_HPP
