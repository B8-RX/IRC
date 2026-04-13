#include "Channel.hpp"

Channel::Channel(void) : _name("") {}
Channel::Channel(const std::string& name) : _name(name) {}
Channel::~Channel(void) {}

const std::string& Channel::getName(void) const {
    return (_name);
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

bool    Channel::isMode(const std::string& mode) const {
    return (_chanModeList.find(mode) != _chanModeList.end());
}

const std::string   Channel::getTopic(void) const {
    return (_t_Topic.topic);
}

const std::string   Channel::getTopicAuthor(void) const {
    return (_t_Topic.topicAuthor);
}

void    Channel::setTopic(const std::string& topic) {
    _t_Topic.topic = topic;
}

void    Channel::setTopicAuthor(const std::string& author) {
    _t_Topic.topicAuthor = author;
}

void    Channel::setTimestamp(void) {
    _t_Topic.time = std::time(0);
}

std::time_t    Channel::getTimestamp(void) const {
    return (_t_Topic.time);
}

// void    Channel::notifyQuit(int memberFd, const std::string& msg) const {
//     std::map<int, MemberState>::const_iterator  it = _members.begin();
//     for (; it != _members.end(); ++it) {
//         if (it->first == memberFd) {
//             continue;
//         }
        
//     }
// }
