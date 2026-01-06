/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConnectionController.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/04 18:23:14 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/21 09:32:57 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConnectionController.hpp"
#include "Logger.hpp"

int ConnectionController::epollSocket = 0; 

ConnectionController::ConnectionController(ConnectionController const &other)
{
	(void) other;
	
}
ConnectionController &ConnectionController::operator=(ConnectionController const &other)
{
	(void) other;
	return (*this);
}


ConnectionController::ConnectionController()
{
	
}

ConnectionController::~ConnectionController()
{
	Logger::log(LC_NOTE, "ConnectionController destructor has been called");
	for(std::map<int, Connection>::iterator it = connections.begin(); it!= connections.end(); ++it )
		closeConnection(it->first);
}


Connection *ConnectionController::findConnection(int fd)
{
	std::map<int,Connection>::iterator it = connections.find(fd);
	if(it != connections.end())
		return &(it->second);	
	return (NULL);
}
bool	ConnectionController::closeConnection(int clientSocket)
{		
	Logger::log(LC_MINOR_NOTE, "Closing client socket #%d, unregistererd from epoll", clientSocket);

	std::map<int,Connection>::iterator it = connections.find(clientSocket);
	epoll_ctl(epollSocket , EPOLL_CTL_DEL , clientSocket, NULL);
	close(clientSocket);	
	if(it != connections.end())
		connections.erase(clientSocket);		
	return (true);
}
int		ConnectionController::openConnection(int clientSocket, ServerConfig serverConfig)
{

	int flags = fcntl(clientSocket, F_GETFL, 0);
	if (flags < 0 || fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) < 0) {
		close(clientSocket);
		Logger::log(LC_ERROR, "Unable to set accepted socket to non-block mode.");
		return (-1);
	}

	connections[ clientSocket ] = Connection(clientSocket, serverConfig);
	connections[ clientSocket ].clear();

	// register to epoll
	epoll_event  event; 
	event.events = EPOLLIN;	
	event.data.fd = clientSocket;
	epoll_ctl(epollSocket,  EPOLL_CTL_ADD, clientSocket , &event);
	Logger::log(LC_MINOR_NOTE, "Accepting client connection #%d, reigistered into epoll", clientSocket);
	

	return connections.size();

}


bool	ConnectionController::handleRead(int clientSocket)
{
	Connection *conn = findConnection(clientSocket);
	size_t	bufferSize = CON_RECV_BUFFER_SIZE - 1;
	char	buffer[CON_RECV_BUFFER_SIZE];
	
	if(conn->getHeaderIsComplete() && conn->getRequestIsComplete())
		return (true);
	
	try {

		while(true)
			{
				try 
				{
					
					int  bytesRead = recv(conn->getSocket(), &buffer, bufferSize, 0 );
					Logger:: log(LC_MINOR_NOTE, " on socket#%d , bytesRead = %d" , conn->getSocket(), bytesRead);


					if(bytesRead == 0)
					{
						Logger::log(LC_MINOR_NOTE, "Connection disconnected from client on socket#%d" , conn->getSocket());
						closeConnection(conn->getSocket());
						return (false);
					}
						
					if (bytesRead == -1) {
						if(conn->shouldRetry())
						{
							Logger::log(LC_MINOR_NOTE, " EAGAIN or EWOULDBLOCK detected");
							return (false);	
						}
						Logger::log(LC_CON_FAIL, "Error recv() , forcing socket#%d to close." , conn->getSocket());
						closeConnection(conn->getSocket());
						return (false);
					}

				    
					if(conn->appendRequestBuffer(buffer , bytesRead, rawServers))
					{
						HttpRequest httpRequest;						
						conn->processRequest(httpRequest);
						Logger::log(LC_MINOR_NOTE, "processRequest() is done!");
						if(handleWrite(conn->getSocket()))
						{
							Logger::log(LC_MINOR_NOTE, " FRI - DONE RESPONDING");
							closeConnection(conn->getSocket());
							return true;
						}
					}
				}
				catch(RequestException &e)
				{
					Logger::log(LC_MINOR_NOTE, "RequestException was thrown in the main loop");
					handleRequestException(e, *conn);
					return (false);
				}
			}	
	} 
	catch (std::exception &e)
	{
		std::cout << "EXCEPTION" << e.what() << std::endl;	
	} 
	return (true);
}

bool	ConnectionController::handleWrite(int clientSocket )
{

	Connection *conn = findConnection(clientSocket);

	if(!conn->needsToWrite())
		return (true);

	size_t sendSize = WEBS_RESP_SEND_SIZE;

	while( conn->getResponseBuffer().length() > 0 )
	{
		conn->punchIn();
 		int bytesSent = send( clientSocket , conn->getResponseBuffer().c_str() ,sendSize , MSG_DONTWAIT);
		if (bytesSent <= 0)
		{
			Logger::log(LC_MINOR_NOTE, " bytesSent = %d" , bytesSent); 
			if( bytesSent == -1)
			{	
				
				if(conn->shouldRetry())
				{
					Logger::log(LC_MINOR_NOTE , "Minor Error: buffer full or would block!");
					return (false);
				}
				Logger::log(LC_MINOR_NOTE , "Done sending");
				closeConnection(clientSocket);

			}
			if (bytesSent == 0)
			{
				Logger::log(LC_MINOR_NOTE , " Done sending");
				closeConnection(clientSocket);
				return (true);
			}
			
			// catch all other errors
			Logger::log(LC_CON_FAIL, "Unrecoverable socket error, abort process");
			closeConnection(conn->getSocket());
		}
		conn->truncateResponseBuffer(static_cast<size_t>(bytesSent));
		
	}
	Logger::log(LC_MINOR_NOTE, " handleWrite closing connection");
	closeConnection(clientSocket);
	return (true);

}
int	ConnectionController::addServer(int fd, ServerConfig server)
{
	if(servers.find(fd) != servers.end())
		throw std::runtime_error("ERROR, try overwrites the existing sockets");
	servers[ fd] = server;
	return servers.size();
}
ServerConfig	*ConnectionController::getServer(int fd)
{
	for( std::map<int, ServerConfig>::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		if(fd == it->first)
		return &(it->second);
	}
	return NULL;
}
std::map<int, ServerConfig> ConnectionController::getServers()
{
	return servers;
}


void		ConnectionController::setEpollSocket(int epollFd)
{
	epollSocket = epollFd;
}

int		ConnectionController::getEpollSocket()
{

	return (epollSocket);
}

size_t	ConnectionController::purgeExpiredConnections()
{
	time_t		now = time(NULL);
	size_t 		count = 0;

	for ( std::map<int, Connection>::iterator it = connections.begin(); it != connections.end(); ++it)
	{
		if(it->second.isExpired(now) )
		{
			Logger::log(LC_MINOR_NOTE, "Connection #%d timeout, closing connection and remove from epoll", it->second.getSocket());
			closeConnection(it->second.getSocket());
			count ++;
		}
	}
	return (count);
}


bool ConnectionController::handleRequestException(RequestException &e, Connection &conn)
{
	HttpResponse httpResponse;
	int statusCode = e.getCode();
	httpResponse.setStatus(e.getCode());
	// handle the 300 series which require redirection 
	if(statusCode >= 300 && statusCode <= 399)
	{
		if(!e.getMessage().empty())
			httpResponse.setHeader("Location", e.getMessage());
	}
	
	conn.ready(httpResponse);
	epoll_event  event;
	memset( &event , 0 , sizeof(event));
	handleWrite(conn.getSocket());
	return (true);
}



void ConnectionController::debug()
{
	std::cout << "\n==========================\nConnectionController\n==========================" << std::endl;
	std::cout << " -rawServers " << std::endl;
	for( std::vector<ServerConfig>::iterator it = rawServers.begin(); it != rawServers.end(); ++it)
	{
		std::cout << " server port " << it->getPort() << std::endl; 
		std::cout << " server name " << it->getServerName() << std::endl; 
	}
}


int	ConnectionController::addRawServer(ServerConfig server)
{
	rawServers.push_back(server);
	return rawServers.size();
}
std::vector<ServerConfig>  ConnectionController::getRawServers()
{
	return rawServers;
}

