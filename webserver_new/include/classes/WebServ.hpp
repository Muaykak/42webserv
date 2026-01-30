#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "ConfigData.hpp"
# include "FileDescriptor.hpp"
# include "Socket.hpp"


class WebServ
{
private:
	// the .conf file that we passed to the program
	const std::string _webservConfigPath;
	ConfigData _configData;
	FileDescriptor _epollFD;
	std::map<int, Socket> sockets;
	
public:
	WebServ(const std::string &configPath);
	void run();
};

#endif
