#ifndef SOCKET_HPP
# define SOCKET_HPP

# include <vector>
# include "FileDescriptor.hpp"
# include <string>
# include <cstdlib>
# include <map>
# include "ServerConfig.hpp"
# include <fcntl.h>
# include <sys/socket.h>
# include "Logger.hpp"
# include <netinet/in.h>
# include <sys/epoll.h>
# include "../utility_function.hpp"
# include "Http.hpp"
# include "CgiProcess.hpp"

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/


enum e_socket_type
{
	NO_TYPE,
	CLIENT_SOCKET,
	SERVER_SOCKET,
	CGI_FD_STDIN,
	CGI_FD_STDOUT
};

// Use only after epoll_create() because we're gonna need
// that epoll_fd in this class
class Socket
{
private:
	FileDescriptor _epollFD;
	FileDescriptor _socketFD;
	e_socket_type _socketType;
	int _server_listen_port;
	const std::vector<ServerConfig> *_serversConfig;
	Http	http;

	// for CGI part
	std::map<int, Socket>	*_socketMap;

	// cgi target to parent
	Socket	*_parentSocket;
	

	CgiProcess	_cgiProcess;


public:

	Socket();
	Socket(const Socket &obj);
	Socket(int fd);
	Socket(const FileDescriptor& fd);
	Socket &operator=(const Socket &obj);
	~Socket();
	const FileDescriptor& getSocketFD() const;
	const FileDescriptor& getEpollFD() const;
	const std::vector<ServerConfig> *getServersConfigPtr() const;

	// Be sure that epollFD still available !
	bool setupSocket(e_socket_type socketType, const std::vector<ServerConfig> *_serversConfigVec,
		const FileDescriptor &epollFD, std::map<int, Socket>* socketMap);

	bool setupCGI_socket(e_socket_type cgiSocketType, const std::vector<ServerConfig> *_serversConfigVec, const FileDescriptor &epollFD,
		Socket* parentSocket, std::map<int, Socket>* socketMap, CgiProcess& cgiProcess);
	// return false means this Socket should be DESTROYED after handleEVENT
	bool handleEvent(const epoll_event &event);

};

#endif
