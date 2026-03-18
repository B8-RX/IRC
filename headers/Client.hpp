#ifndef CLIENT_HPP
 #define CLIENT_HPP

 
 #include <string>
 #include <sys/socket.h>
 #include <netinet/in.h>


 class Client {
     public:
        int             fd;
        std::string     ipAddr;
        uint16_t        port;
        sockaddr_in		address;
		socklen_t		addressLen;
        Client(void);
        ~Client(void);
 };

#endif // !CLIENT_HPP