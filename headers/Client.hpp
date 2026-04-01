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
	
private:
	std::string             		_nickname;
	bool							_hasNick;
	std::string             		_username;
	bool							_hasUser;
	bool							_passAccepted;
	bool                    		_registered;
	std::map<int, Channel>  		_channels;
 };

#endif // !CLIENT_HPP
