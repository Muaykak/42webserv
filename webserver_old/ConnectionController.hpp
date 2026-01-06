/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConnectionController.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/04 18:14:32 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/20 20:00:05 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef		CONNECTIONCONTROLLER__HPP
# define	CONNECTIONCONTROLLER__HPP
# include 	<map>
# include	<unistd.h>
# include	<string.h>
# include	<cstring>
# include 	"settings.hpp"
# include 	"Connection.hpp"
# include 	"ServerConfig.hpp"


class Connection;

class ConnectionController 
{
	private:

		std::map<int, Connection> 	connections; 
		std::map<int, ServerConfig> servers;
		std::vector<ServerConfig>	rawServers;

		ConnectionController(ConnectionController const &other);
		ConnectionController &operator=(ConnectionController const &other);

	public:
		ConnectionController();		
		~ConnectionController();

		static int					epollSocket; 

		Connection 					*findConnection(int fd);
		bool						closeConnection(int fd);
		int							openConnection(int fd, ServerConfig config);

		bool						handleRead(int clientSocket);
		bool						handleWrite(int clientSocket);

		int							addServer(int fd, ServerConfig server);
		ServerConfig				*getServer(int fd);
		std::map<int, ServerConfig> getServers(); 

		static void					setEpollSocket(int epollFd);
		static  int					getEpollSocket();

		size_t						purgeExpiredConnections();
		bool 						handleRequestException(RequestException &reqException,Connection &conn);
		void						debug();

		int							addRawServer(ServerConfig server);
		std::vector<ServerConfig>   getRawServers();

};

#endif
