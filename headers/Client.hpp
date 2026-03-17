#ifndef CLIENT_HPP
 #define CLIENT_HPP

 
 #include <string>

 class Client {
    private:
        int             _fd;
        std::string     _ipAddr;
    public:
        Client(void);
        ~Client(void);
        void            setClientFd(int fd);
        void            setClientIpAddr(const std::string& ipAddr);
        int             getClientFd(void);
        std::string&    getClientIpAddr(void);

 };

#endif // !CLIENT_HPP