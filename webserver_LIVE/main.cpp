/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/13 10:24:58 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/20 19:50:59 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <exception>
#include <signal.h>
#include "settings.hpp"
#include "Webserv.hpp"

int main(int argc, char **argv)
{
	try {
		if (argc > 2)
			throw std::invalid_argument("Invalid arguments, use webserv [configuration_file]");
		std::string config = (argc == 1) ? WEBS_DEF_CONFIG_FILE : std::string(argv[1]);

		signal(SIGPIPE, SIG_IGN);
		Webserv webserv(config);
		webserv.run();
	}
	catch(std::exception &e)
	{
		std::cout << LC_ERROR << "Error: " << e.what() << LC_RESET << std::endl;
	}
	return (0);
}
