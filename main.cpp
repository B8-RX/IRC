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

int	main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <port>\n";
		return (1);
	}
	try {
	Server	myServer;
	std::istringstream iss(argv[1]);
	uint16_t	port;
	char	c;

	if (!(iss >> port) || iss.get(c)) {
		std::cerr << "Invalid port number: " << argv[1] << "\n";
		return (1);
	}

	myServer.init(port);
	myServer.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << "\n";
	}
	return (0);
}
