#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "ConfigData.hpp"
# include "FileDescriptor.hpp"
# include "Socket.hpp"
# include "OptionalData.hpp"
# include "HttpRequest.hpp"

//
struct s_webserv_custom_event
{
	OptionalData<HttpRequest> httpRequestData;
};



struct s_webserv_event
{
	int eventFd;
	OptionalData<epoll_event *> epollEventData;
	OptionalData<s_webserv_custom_event> customEventData;
};

struct s_webserv_event_controller
{
	FileDescriptor epollFD;

	/* int is the socket FD int that you want to trigger
	the event */
	std::map<int, s_webserv_custom_event> *customEventMap;
};

class WebServ
{
private:
	// the .conf file that we passed to the program
	const std::string _webservConfigPath;
	ConfigData _configData;
	FileDescriptor _epollFD;
	std::map<int, Socket> sockets;

	epoll_event _epollEvents[MAX_EPOLL_EVENT];
	std::map<int, s_webserv_custom_event> _customEventMap;

	int webservCheckEvent(std::map<int, s_webserv_event>& returnEvents);
	
public:
	WebServ(const std::string &configPath);
	void run();
};

#endif
