#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "ConfigData.hpp"
# include "FileDescriptor.hpp"
# include "Socket.hpp"
# include "OptionalData.hpp"
# include "HttpRequest.hpp"
# include "../defined_value.hpp"

//

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
	void handleSocketEvents(std::map<int, s_webserv_event>::const_iterator& eventIt, std::map<int, s_webserv_event>& returnEvents);
	void checkSocketTimeOut();
	bool	checkCGIOUTSocketTimeOut(std::map<int, Socket>::iterator &socketIt, const time_t& currentTime);
	
public:
	WebServ(const std::string &configPath);
	void run();
};

#endif
