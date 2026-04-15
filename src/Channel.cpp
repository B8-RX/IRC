#include "Channel.hpp"

Channel::Channel(void) : _name(""), _creationTime(std::time(0)), _inviteOnly(false), _topicRestricted(false), _limitEnabled(false), _keyEnabled(false) {}
Channel::Channel(const std::string& name) : _name(name), _creationTime(std::time(0)), _inviteOnly(false), _topicRestricted(false), _limitEnabled(false), _keyEnabled(false) {}
Channel::~Channel(void) {}

const std::string& Channel::getName(void) const {
	return (_name);
}

const time_t& Channel::getCreationTime(void) const {
	return (_creationTime);
}

bool    Channel::empty(void) const {
	return (_members.empty());
}

std::size_t Channel::memberCount(void) const {
	return (_members.size());
}

bool    Channel::isMember(int memberFd) const {
	return (_members.find(memberFd) != _members.end());
}

bool    Channel::addMember(int memberFd, bool isOp) {
	if (isMember(memberFd))
		return (false);
	_members[memberFd] = MemberState(isOp);
	return (true);
}

bool    Channel::removeMember(int memberFd) {
	std::map<int, MemberState>::iterator it = _members.find(memberFd);

	if (it == _members.end())
		return (false);
	_inviteList.erase(memberFd);
	_members.erase(it);
	return (true);
}

bool    Channel::isChanOp(int memberFd) const {
	std::map<int, MemberState>::const_iterator it = _members.find(memberFd);

	if (it == _members.end())
		return (false);
	return (it->second.isChanOp);
}

std::map<int, Channel::MemberState>&   Channel::getMembers(void) {
	return (_members);
}

bool    Channel::addInvite(int memberFd) {
	return (_inviteList.insert(memberFd).second);
}

bool    Channel::removeInvite(int memberFd) {
	return (_inviteList.erase(memberFd));
}

bool    Channel::isInvited(int memberFd) {
	return (_inviteList.find(memberFd) != _inviteList.end());
}

const Channel::s_Topic&   Channel::getTopicStruct(void) const {
	return (_topic);
}

const std::string   Channel::getTopic(void) const {
	return (_topic.topic);
}

const std::string   Channel::getTopicAuthor(void) const {
	return (_topic.topicAuthor);
}

void    Channel::setTopic(const std::string& topic) {
	if (topic.empty()) {
		_topic.topic = "";
	}
	else {
		_topic.topic = topic;
	}
}

void    Channel::setTopicAuthor(const std::string& author) {
	_topic.topicAuthor = author;
}

void    Channel::setTopicTimestamp(void) {
	_topic.time = std::time(0);
}

bool    Channel::isMode(const std::string& mode) const {
	if (mode == "i") {
		return (_inviteOnly);
	}
	if (mode == "t") {
		return (_topicRestricted);
	}
	if (mode == "k") {
		return (_keyEnabled);
	}
	if (mode == "l") {
		return (_limitEnabled);
	}
	return (false);
}

void	Channel::setInviteOnly(bool inviteOnly) {
	_inviteOnly = inviteOnly;
}

void	Channel::setTopicRestricted(bool topicRestricted) {
	_topicRestricted = topicRestricted;
}

void	Channel::setKey(const std::string& key) {
	_key = key;
	_keyEnabled = true;
}

const std::string&	Channel::getKey(void) const{
	return (_key);
}

void	Channel::unsetKey(void) {
	_key.clear();
	_keyEnabled = false;
}

void    Channel::setLimit(std::size_t limit) {
	_userLimit = limit;
	_limitEnabled = true;
}

void    Channel::unsetLimit(void) {
	_limitEnabled = false;
}

std::size_t    Channel::getLimitChannel() {
	return (_userLimit);
}

std::string	Channel::buildModeString(void) const {
	std::string modeString = "";

	if (_inviteOnly) {
		modeString += "i";
	}
	if (_topicRestricted) {
		modeString += "t";
	}
	if (_keyEnabled) {
		modeString += "k";
	}
	if (_limitEnabled) {
		modeString += "l";
	}
	if (!modeString.empty()) {
		modeString = "+" + modeString;
	}
	return (modeString);
}

bool	Channel::updateMemberState(int memberFd, bool isOp) {
	std::map<int, MemberState>::iterator member = _members.find(memberFd);
	if (member == _members.end()) {
		return (false);
	}
	member->second.isChanOp = isOp;
	return (true);
}
