// This file serves as an example outline to webserv project
#ifndef EXAMPLE_HPP
# define EXAMPLE_HPP

// small RULES for this project
/*
	1. make use of c++ conventions; like create a wrapper class, throw exceptions, make use of STL
	2. The commenting format should look like this

	<previous line of code>

	// the comment we want to comment
	<line of code we want to comment>

	<next line of code>

	or 

	<previous line of code>

	<line of code we want to comment>
	// when you want to
	// comment more than one line of code
	// it easier to look!

	<next line of code>
	


	- i think it looks better this way so i think we can try that

	3. try to name variables and functions that is easily to understand.
*/


# include <exception>
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <cstring>
# include <cstdlib>
# include <fcntl.h>
# include <iostream>
# include <sstream>
# include <string>
# include <map>
# include <vector>
# include <set>


// CONSTANT VALUE

//----------- LIMITS
// Soft limit of file descriptor allowed in this project
# define MAX_FD 1024

// Color Text
# define  	LC_RED  				"\033[31m"
# define  	LC_YELLOW  				"\033[33m"
# define  	LC_BOLD  				"\033[1m"
# define  	LC_RESET  				"\033[0m"
# define  	LC_ERROR  				"\033[31m\033[1m"
# define  	LC_DEBUG  				"\033[1m\033[34m"

# define  	LC_SYSTEM  				"\033[34m\033[1m"
# define  	LC_MINOR_SYSTEM  		"\033[34m\033[1m" 
# define  	LC_INFO  				"\033[36m"
# define  	LC_NOTE  				"\033[33m"
# define  	LC_MINOR_NOTE  			"\033[30m "

# define  	LC_REQ_LOG  			"\033[33m"
# define  	LC_RES_OK_LOG  			"\033[32m"
# define  	LC_RES_NOK_LOG  		"\033[31m"
# define  	LC_RES_INT_LOG  		"\033[36m"
# define  	LC_RES_FND_LOG  		"\033[34m"
# define  	LC_CON_FAIL  			"\033[31m"
# define  	LC_CONN_LOG  			"\033[35m"

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

// exception class
class WebservException : public std::exception {
	protected:
		std::string _message;
	public:
		WebservException(const std::string& message) throw() : _message(message){
		};
		virtual ~WebservException() throw(){
		};

		virtual const char *what() const throw(){
			return (_message.c_str());
		};
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/


/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class FileFD{
/*
	This is a wrapper class
	Normally we need to close our fd by close() after we finished using it

	we can use destructor of c++ to handle the close()
*/

	private:
		const int	_fd;
		// the actual int that store file descriptor number
	public:
		FileFD(int fd) : _fd(fd){
			if (_fd < 0 || _fd > MAX_FD - 1){
				throw WebservException("FileDesctiptor::fd is out of range!");
			}
		};

		~FileFD(){
			close(_fd);
		};

		//operator overload
		bool operator==(const FileFD& rhs){
			return _fd == rhs._fd;
		}
		bool operator!=(const FileFD& rhs){
			return _fd != rhs._fd;
		}
		bool operator==(const int& rhs){
			return _fd == rhs;
		}
		bool operator!=(const int& rhs){
			return _fd != rhs;
		}


		int	getFd(void) const{
			return _fd;
		}

};

std::ostream& operator<<(std::ostream &os, const FileFD& obj){
	os << obj.getFd();
}

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

# include <ctime>
# include <cstdarg>

class Logger
{
    private:
		Logger();
		Logger(Logger const &other);
		Logger &operator=(Logger const &other);
		~Logger();

	public:
		//void log(int output, std::string color, std::string message);
		static void log(std::string color, std::string  message, ...);
		static std::string getTimestamp();
};

Logger::Logger()
{

}
Logger::Logger(Logger const &other)
{
	if(this != &other)
		*this = other;

}
Logger &Logger::operator=(Logger const &other)
{
	if(this != &other)
		*this = other;
	return (*this);
}

Logger::~Logger()
{

}
void Logger::log(std::string color, std::string message, ...)
{

	if(color == LC_MINOR_NOTE)
		return ; 

	std::stringstream 	ss;
	va_list		args;
	va_start(args, message);
	char	buff[500];
	vsprintf(buff, message.c_str() , args);

	std::cout << color << getTimestamp() << std::string(buff) << LC_RESET << std::endl; 

}
std::string Logger::getTimestamp()
{
	time_t	t; 
	char	buff[24];

	t = time(0);
	strftime(buff, sizeof(buff) , "[%Y-%m-%d %H:%M:%S] " , localtime(&t));
	return std::string(buff);
}

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

typedef	std::map<std::string, std::string>	t_config_map;
typedef	std::map<std::string, t_config_map>	t_location_map;

class ServerConfigData {
	private:
		const t_config_map	serverConfig;
		const t_location_map locationConfigs;
		const int			listenPort;

	public:
	
		ServerConfigData(const t_config_map& configServer,const t_location_map& configLocations) :
			serverConfig(configServer), locationConfigs(configLocations),
		{
			// Check if got all required things
		};
		ServerConfigData(const ServerConfigData& obj): serverConfig(obj.serverConfig), locationConfigs(obj.locationConfigs), listenPort(obj.listenPort)
		{
			// Check if got all required things
		}
		~ServerConfigData(){}


		std::string	getLocationData(const std::string& locationPath,  const std::string& keyToFind) const {
			const t_location_map::const_iterator currentLocationConfig = locationConfigs.find(locationPath);
			if (currentLocationConfig == locationConfigs.end()){
				return ("");
			}
			const t_config_map::const_iterator keyFound = currentLocationConfig->second.find(keyToFind);
			if (keyFound != currentLocationConfig->second.end()){
				return (keyFound->second);
			}
			const t_config_map::const_iterator serverFoundKey = serverConfig.find(keyToFind);
			if (serverFoundKey != serverConfig.end()){
				return (serverFoundKey->second);
			}
			return ("");
		};

		std::string	getServerData(const std::string& keyToFind) const {
			const t_config_map::const_iterator keyFound = serverConfig.find(keyToFind);
			if (keyFound != serverConfig.end()){
				return (keyFound->second);
			}
			return ("");
		}
		
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class ServerSockets {
	private:
		ServerConfigData	configData;

		std::string	serverName;
		int			port;
		FileDescriptor		socketFD;

		void createServerSocket(const ServerConfigData& servConfData) {

			std::string server_name = servConfData.getServerData("server_name");

			// AF_INET   is  ipv4  
			// SOCK_STREAM is connection with no data loss (TCP)
			int socket_fd = socket(AF_INET, SOCK_STREAM, 0);


			if (socket_fd < 0){
				throw WebservException("Server[" + server_name + "]::createListeningSocket::socket() failed");
			}

			// Non blocking mode need to be enable to prevent freezing
			if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) == -1) {
				close(socket_fd);
				throw WebservException("Server[" + server_name + "]::createListeningSocket::fcntl() failed");
			}

			int opt = 1;
			if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
				Logger::log(LC_NOTE, "Fail to set socket#%d for reuse socket", socket_fd);
			S
			

			sockaddr_in sv_addr;
			std::memset(&sv_addr, 0, sizeof(sockaddr_in));
			sv_addr.sin_family = AF_INET;
			sv_addr.sin_addr.s_addr = INADDR_ANY;
			sv_addr.sin_port = htons(std::atoi(servConfData.getServerData("listen").c_str()));
		};
		static int	getPort(const ServerConfigData& servConfData){
			std::string	portString = servConfData.getServerData("listen");
			
			std::stringstream ss(portString);
			if (!(ss >> portString)){
				throw WebservException("Server");
			}
		}
		static std::string getServerName(const ServerConfigData& servConfData){
			std::string server_name = servConfData.getServerData("server_name");
			if (server_name.empty()){
				throw WebservException("Server::getServerName() failed");
			}
			return (server_name);
		}
	public:

		Server(const ServerConfigData& serverConfigData) : configData(serverConfigData)
		{

		}


};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class WebServ {
	private:
		// the .conf file that we passed to the program
		const std::string _webservConfigPath;

		std::map<int, std::vector<ServerConfigData>> _serversByPort;


		// loads all config data into proper structure
		void configFileParser(){


			// throw SomeExceptions Error when something wrong
		};

	public:
		WebServ(const std::string& configPath) : _webservConfigPath(configPath)
		{
			// Handle ConfigPath here or
			configFileParser();
		}
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

#endif