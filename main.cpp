/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ssghioua <ssghioua@42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/01 23:26:13 by ssghioua          #+#    #+#             */
/*   Updated: 2026/03/01 23:26:14 by ssghioua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <exception>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <cstring>

int	main(int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>(optional)\n";
		return (1);
	}
/*!
 * @brief Set up signal handlers
 */
	struct sigaction act;
	act.sa_handler = &Server::sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
/*!
 * @brief Set up Ctrl+C signal handlers
 */
	if (sigaction(SIGINT, &act, NULL) == -1) {
		std::cerr << "Failed to set signal handler: " << std::strerror(errno) << "\n";
		return (1);
	}
/*!
 * @brief Set up Ctrl+\ signal handlers
 */
	if (sigaction(SIGQUIT, &act, NULL) == -1) {
		std::cerr << "Failed to set signal handler: " << std::strerror(errno) << "\n";
		return (1);
	}

	try {
	std::string	password = (argc == 3) ? argv[2] : ""; 
	std::istringstream iss(argv[1]);
	uint16_t	port;
	char	c;
	
	if (!(iss >> port) || iss >> c) {
		std::cerr << "Invalid port number: " << argv[1] << "\n";
		return (1);
	}
	
	Server	myServer(port, password);
	myServer.init();
	myServer.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << "\n";
	}
	std::cout << "Server Closed! Bye Bye!\n";
	return (0);
}
