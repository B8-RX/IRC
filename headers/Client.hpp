#ifndef CLIENT_HPP
 #define CLIENT_HPP

 
#include <queue>
#include <string>
#include <map>
#include "Channel.hpp"

class Channel;

class Client {
 public:
	Client(void);
	~Client(void);
	
	int             		        fd;
	std::string     		        ipAddr;
	std::string     		        bufferIn;
	std::string     		        bufferOut;
	std::string             		nickname;
	std::string             		username;
	bool                    		connected;
	std::map<int, Channel>  		channels;
 };

#endif // !CLIENT_HPP
