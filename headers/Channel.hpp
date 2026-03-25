
#ifndef CHANNEL_HPP
 #define CHANNEL_HPP

#include <string>
#include "Client.hpp"

 class Channel {
     public:
        Channel(void);
        ~Channel(void);
        
        int                     fd;
        std::string             buffer_in;
        std::string             buffer_out;
 };

#endif // !CHANNEL_HPP
