
#ifndef CHANNEL_HPP
 #define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
#include <cstddef>

 class Channel {
     public:
        Channel(void);
        Channel(const std::string& name);
        ~Channel(void);
        struct MemberState {
            bool    isChanOp;
            MemberState(void) : isChanOp(false) {}
            MemberState(bool isOp) : isChanOp(isOp) {}
        };

        const std::string&  getName(void) const;
        bool                empty(void) const;
        std::size_t         memberCount(void) const;

        bool                addMember(int memberFd, bool isOp);
        bool                removeMember(int memberFd);
        
        bool                addInvite(int memberFd);
        bool                removeInvite(int memberFd);
        bool                isInvited(int memberFd);
        
        bool                isMode(const std::string& mode) const;

        bool                isMember(int memberFd) const;
        bool                isChanOp(int memberFd) const;
        std::map<int, MemberState>&                         getMembers(void);

    private:
        std::string                 _name;
        std::map<int, MemberState>  _members;
        std::set<int>               _inviteList;
        std::set<std::string>       _chanModeList; // i, t, k, o, l
 };

#endif // !CHANNEL_HPP
