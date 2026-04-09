
#ifndef CHANNEL_HPP
 #define CHANNEL_HPP

#include <string>
#include <map>
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

        const std::string&                                  getName(void) const;
        bool                                                empty(void) const;
        std::size_t                                         memberCount(void) const;

        bool                                                addMember(int memberFd, bool isChanOp);
        bool                                                removeMember(int memberFd);

        bool                                                isMember(int memberFd) const;
        bool                                                isChanOp(int memberFd) const;
        std::map<int, MemberState>&                         getMembers(void);
        // void                                                notifyQuit(int memberfd, const std::string& msg) const;

    private:
        std::string                   _name;
        std::map<int, MemberState>    _members;
 };

#endif // !CHANNEL_HPP
