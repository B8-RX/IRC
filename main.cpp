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

int	main(void) {
	try {
	Server	myServer;
	myServer.init("IPV4", "TCP", 0, 8080);
	myServer.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << "\n";
	}
	return (0);
}
