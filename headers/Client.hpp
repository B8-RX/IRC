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
	std::string     		        buffer_in;
	std::string     		        buffer_out;
	struct s_Line {
		std::string					raw;
		std::string					prefix; // optional,may not be present 
		std::string					command;
		std::vector<std::string>	params; // strip ':' if trailing
		// use MAX_SIZE_MSG to check/ERR_INPUTTOOLONG
	};
	std::queue<struct s_Line>		_queue;
	std::string             		nickname;
	std::string             		username;
	bool                    		connected;
	std::map<int, Channel>  		channels;
 };

#endif // !CLIENT_HPP
