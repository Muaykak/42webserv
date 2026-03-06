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
# include "HttpCgi.hpp"

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

struct s_cgidata {
	std::map<std::string, std::string> modifyEnvpMap;
	const std::map<std::string, std::set<std::string> >* headerFieldPtr;
	bool *isCgiProcessOpen;
};


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
	time_t	_lastEventTime;

	FileDescriptor _epollFD;
	FileDescriptor _socketFD;
	e_socket_type _socketType;
	const std::vector<ServerConfig>* _serversConfig;
	int _server_listen_port;
	std::set<in_addr_t>	_server_ip_host;
	std::vector<Http>	http;
	std::vector<HttpCgi> httpCgi;			

	// for CGI part
	std::map<int, Socket>	*_socketMap;

public:
	in_addr_t	_client_addr_in;

	Socket();
	Socket(const Socket &obj);
	Socket(int fd);
	Socket(const FileDescriptor& fd);
	Socket &operator=(const Socket &obj);
	~Socket();
	const FileDescriptor& getSocketFD() const;
	const FileDescriptor& getEpollFD() const;
	const std::vector<ServerConfig> *getServersConfigPtr() const;
	int getServerListenPort() const;

	bool setupServerSocket(const std::vector<ServerConfig> *_serversConfigVec,
		const FileDescriptor &epollFD, std::map<int, Socket>* socketMap);
	bool setupClientSocket(const std::vector<ServerConfig> *_serversConfigVec,
		const FileDescriptor &epollFD, std::map<int, Socket>* socketMap);
	bool setupCGIINSocket(const std::vector<ServerConfig> *_serversConfigVec,
		const FileDescriptor &epollFD, std::map<int, Socket>* socketMap);
	bool setupCGIOUTSocket(const std::vector<ServerConfig> *serversConfig,
		const FileDescriptor &epollFD, std::map<int, Socket>* socketMap, const HttpCgi& cgiData);

	// Be sure that epollFD still available !

	// return false means this Socket should be DESTROYED after handleEVENT
	bool handleEvent(const epoll_event &event);

	void setServerIpHost(const std::set<in_addr_t>& obj);
	const std::set<in_addr_t>& getServerIpHost() const;

	time_t	getLastEventTime() const;
	e_socket_type	getServerSockerType() const;

};

#endif
