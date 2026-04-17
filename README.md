*This project has been created as a part of the 42 curriculum by ssghioua, cfakoube*

#IRC Server (ft_irc)

## Description

This project is about creating an IRC server in C++98. The server must follow the IRC protocol rules.

The aim is to learn about low-level network programming. This includes managing sockets, handling input/output with poll and communication between the server and clients. The server needs to handle clients at the same time. It should support IRC features like:

- Client registration with PASS, NICK and USER commands

- Managing channels with JOIN, PART and QUIT commands

- Sending messages with PRIVMSG

- Channel operations using MODE, INVITE, TOPIC and KICK

The server talks using text messages that end with `\r\n`. It follows the IRC protocol, for communication.

## Instructions

### Compilation
```bash
make

- Execution
./irc <port> <password>

- Example:
    ./irc 6667 mypassword

Connecting with a client
    Using nc:
        nc -C 127.0.0.1 6667

Using irssi:
    /connect 127.0.0.1 6667 mypassword nickname

```
### Features

The program can handle clients at the same time using poll.

It can also handle messages that're not complete by storing them in a buffer and putting them back together again.

The program understands IRC commands.

It can manage channels. It has a system for operators.

The program supports these things:

* PASS

* NICK

* USER

* JOIN

* PART

* QUIT

* PRIVMSG

* INVITE

* TOPIC

* MODE. This includes channel modes like i, t, k l o.

* Basic IRC numeric replies, like 001, 002 003 004 and error messages.

It works with IRC clients like irssi.

Technical Choices

The language used is C++98.

The program uses TCP sockets for the network.

It uses poll for I/O multiplexing.

Here is how it is set up:

The server is, in charge of the clients and the channels.

Each client has its place to store input.

The program only processes commands when it gets a line, which is marked by \r\n.


### Resources

We are using the IRC protocol you can find the version at https://modern.ircdocs.horse/.

If you want to look at the old version it is at https://datatracker.ietf.org/doc/html/rfc1459 as a historical reference.

Beej’s Guide to Network Programming is also helpful it is at https://beej.us/guide/bgnet/.

Tutorials and references are important.

We need to know about socket programming, which's the basics of TCP/IP.

The poll() function and non-blocking I/O documentation are also necessary.

* Artificial Intelligence tools, like ChatGPT were used to understand how the IRC protocol works and what might go wrong.

They helped us fix some issues like when we read part of a message or when the message was not formatted correctly.

They also helped us make our code better and more robust.

They clarified what the IRC commands and numeric replies are supposed to do.

We wrote all the code ourselves and we tested it. Made sure it works.

* Notes

This is a version of an IRC server it is meant for people who want to learn.

It does not do everything that a real IRC server does. It has the basic features that we need for our project.

The IRC server implementation is not meant to follow all the rules. It has the main things that we need.