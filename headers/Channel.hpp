
#ifndef CHANNEL_HPP
 #define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
#include <cstddef>
#include <ctime>


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

		bool						empty(void) const;
		std::size_t					memberCount(void) const;
		const std::string&			getName(void) const;

		bool						addMember(int memberFd, bool isOp);
		bool						removeMember(int memberFd);
		
		bool						addInvite(int memberFd);
		bool						removeInvite(int memberFd);
		bool						isInvited(int memberFd);
		
		bool						isMode(const std::string& mode) const;

		bool						isChanOp(int memberFd) const;
		bool						isMember(int memberFd) const;
		std::map<int, MemberState>& getMembers(void);

		const std::string			getTopic(void) const;
		const std::string			getTopicAuthor(void) const;
		void						setTopic(const std::string& topic);
		void						setTopicAuthor(const std::string& author);
		void						setTimestamp(void);
		std::time_t					getTimestamp(void) const;
	private:
		std::string                 _name;
		std::map<int, MemberState>  _members;
		std::set<int>               _inviteList;
		std::set<std::string>       _chanModeList; // i, t, k, o, l
		struct s_Topic {
			std::string	topic;
			std::string	topicAuthor;
			std::time_t	time;
		} _t_Topic;
 };

#endif // !CHANNEL_HPP
