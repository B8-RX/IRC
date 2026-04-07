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

bool    Channel::addMember(int memberFd, bool isChanOp) {
    if (isMember(memberFd))
        return (false);
    _members[memberFd] = MemberState(isChanOp);
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

const std::map<int, Channel::MemberState>&   Channel::getMembers(void) const {
    return (_members);
}
