#ifndef CLIENT_HPP
 #define CLIENT_HPP

 
 #include <string>
 #include <sys/socket.h>
 #include <netinet/in.h>


 class Channel;

 class Client {
     public:
        Client(void);
        ~Client(void);
        
        int                     fd;
        std::string             ipAddr;
        uint16_t                port;
        std::string             buffer_in;
        std::string             buffer_out;
        std::string             nickname;
        std::string             username;
        bool                    connected;
        std::map<int, Channel>  channels;
 };

#endif // !CLIENT_HPP