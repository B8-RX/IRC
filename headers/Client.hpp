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
	std::string						getOldNickname(void) const;
	std::string						getUsername(void) const;
	std::string						getRealname(void) const;
	bool							getPassAccepted(void) const;
	bool							getRegirstered(void) const;
	bool							hasNick;
	bool							hasUser;

	const std::string&				setNickname(const std::string&);
	const std::string&				setOldNickname(const std::string&);
	const std::string&				setUsername(const std::string&);
	const std::string&				setRealname(const std::string&);
	bool							setPassAccepted(bool state);
	bool							setRegirstered(bool state);

	bool							addSubscriptionChan(const std::string& name);
	bool							isMemberChan(const std::string& name) const;
	const std::vector<std::string>&	getSubscribedChannels(void) const;
	bool							removeSubscribedChannel(const std::string& channel);

private:
	std::string             		_oldNickname;
	std::string             		_nickname;
	std::string             		_username;
	std::string						_realname;
	bool							_passAccepted;
	bool                    		_registered;
	std::vector<std::string>		_channels;
 };

#endif // !CLIENT_HPP
