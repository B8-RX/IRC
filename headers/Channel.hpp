
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
		
		// general infos
		bool						empty(void) const;
		std::size_t					memberCount(void) const;
		const std::string&			getName(void) const;
		const time_t&				getCreationTime(void) const;
		std::size_t					getLimitChannel(void);

		// invited
		bool						addInvite(int memberFd);
		bool						removeInvite(int memberFd);
		bool						isInvited(int memberFd);
		
		
		// members/Operator		
		struct MemberState {
			//! bool	isFounder;
			bool    isChanOp;
			MemberState(void) : isChanOp(false) {}
			MemberState(bool isOp) : isChanOp(isOp) {}
		};
		bool							isChanOp(int memberFd) const;
		bool							updateMemberState(int memberFd, bool isOp);
		bool							addMember(int memberFd, bool isOp);
		bool							removeMember(int memberFd);
		bool							isMember(int memberFd) const;
		std::map<int, MemberState>& 	getMembers(void);


		// topic
		struct s_Topic {
			std::string	topic;
			std::string	topicAuthor;
			std::time_t	time;
		};
		const s_Topic&				getTopicStruct(void) const;
		const std::string			getTopic(void) const;
		const std::string			getTopicAuthor(void) const;
		void						setTopic(const std::string& topic);
		void						setTopicAuthor(const std::string& author);
		void						setTimestamp(void);
		std::time_t					getTimestamp(void) const;


		// channel modes
		bool						isMode(const std::string& mode) const;
		void						setInviteOnly(bool);
		void						setTopiRestricted(bool);
		void						setKey(const std::string& key);
		const std::string&			getKey(void) const;
		void						unsetKey(void);
		void						setLimit(std::size_t limit);
		void						unsetLimit(void);
		std::string					buildModeString(void) const;
		void						handleSingleMode(const char mode, bool add, const std::string& param);
	
		private:
		std::string                 _name;
		const time_t				_creationTime;
		std::map<int, MemberState>  _members;
		std::set<int>               _inviteList;
		
		s_Topic						_topic;

		// channel modes
		bool						_inviteOnly;
		bool						_topicRestricted;
		bool						_limitEnabled;
		std::size_t					_userLimit;
		bool						_keyEnabled;
		std::string					_key;

 };

#endif // !CHANNEL_HPP
