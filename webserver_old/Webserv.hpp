/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/13 10:15:43 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/20 19:53:20 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef		WEBSERV_HPP
# define	WEBSERV_HPP

# include 	<iostream>
# include 	<vector>
# include 	<algorithm>
# include 	<set>
# include 	<sys/socket.h>
# include	<netinet/in.h>
# include	<sys/epoll.h>
# include 	<fcntl.h>

# include 	<cstring>
# include 	<cerrno>
# include	"settings.hpp"
# include 	"ServerConfig.hpp"
# include 	"HttpRequest.hpp"
# include 	"HttpResponse.hpp"
# include 	"RequestException.hpp"
# include 	"ConnectionController.hpp"
# include 	"ConfigParser.hpp"

class ServerConfig; 
class ConnectionController;

class Webserv
{
	private:
		
		std::vector<ServerConfig>	serverConfigs;
		std::string					configFile;
		std::vector<int>			serverSockets;
		ConnectionController		connectionController; 
				
		Webserv();
		Webserv(Webserv const &other);
		Webserv &operator=(Webserv const &other);

		size_t						parseConfig(std::string config_file);
		bool						setupSockets(ConnectionController &cc);
		bool						findServerFromRequest(HttpRequest &req, ServerConfig &sc);
		bool						isServerSocket(int fd);

	public:
		
		~Webserv();
		Webserv(std::string configFile);
		std::set<int> 				getListeningPorts();
		ConnectionController 		&getConnectionController();
		std::vector<ServerConfig> 	getServerConfigs();
		
		int 						run(void);
		
};

#endif
