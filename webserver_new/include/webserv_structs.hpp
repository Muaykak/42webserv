#ifndef WEBSERV_STRUCTS
# define WEBSERV_STRUCTS

#include <map>
#include <string>
#include "./classes/OptionalData.hpp"
#include <sys/epoll.h>
#include "./classes/FileDescriptor.hpp"

typedef std::map<std::string, std::vector<std::string> > t_config_map;
typedef std::map<std::string, const t_config_map> t_location_map;

class HttpRequest;

struct s_response_buff
{
	std::vector<char> buffer;
	size_t	currentIndex;
};

struct s_webserv_custom_event
{
	OptionalData<HttpRequest> httpRequestData;
	OptionalData<bool> clientSocketManualDisconnect;
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

#endif