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
	
	std::string						getNickname(void) const;
	std::string						getUsername(void) const;
	bool							getPassAccepted(void) const;
	bool							getRegirstered(void) const;

	std::string						setNickname(const std::string&);
	std::string						setUsername(const std::string&);
	bool							setPassAccepted(bool state);
	bool							setRegirstered(bool state);

private:
	std::string             		_nickname;
	bool							_hasNick;
	std::string             		_username;
	std::string						_realName;
	bool							_hasUser;
	bool							_passAccepted;
	bool                    		_registered;
	std::map<int, Channel>  		_channels;
 };

#endif // !CLIENT_HPP
