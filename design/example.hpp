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

class FileDescriptor{
/*
	This is a wrapper class
	Normally we need to close our fd by close() after we finished using it

	we can use destructor of c++ to handle the close()
*/

	private:
		int	_fd;
		// the actual int that store file descriptor number
	public:
		FileDescriptor(int fd) : _fd(fd){
			if (_fd < 0 || _fd > MAX_FD - 1){
				throw FileDescriptor::OutOfRangeException();
			}
		};

		~FileDescriptor(){
			close(_fd);
		};

		//operator overload
		bool operator==(const FileDescriptor& rhs){
			return _fd == rhs._fd;
		}
		bool operator!=(const FileDescriptor& rhs){
			return _fd != rhs._fd;
		}

		int	getFd(void) const{
			return _fd;
		}

		// Exception
		class OutOfRangeException : public std::exception {
			virtual const char	*what() const throw(){
				return ("fd is out of range!");
			}
		};
};

std::ostream& operator<<(std::ostream &os, const FileDescriptor& obj){
	os << obj.getFd();
}

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

class ConfigData {
	// Required
	const unsigned int 	clientMaxBodySize;

	// Optional
	std::map<int, std::string>	errorPages;
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class ServerLocationConfigData : {

};


/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class ServerConfigData {
	public:

		// Required
		const std::string	listeningPort;
		const std::string	hostAddress;
		const std::string	serverName;
		const unsigned int 	clientMaxBodySize;
		std::vector<ServerLocationConfigData> locationDatas;

		// Optional
		std::map<int, std::string>	errorPages;


		ServerConfigData(const std::string& listen_port, const std::string& host_ip,
		const std::string& server_name, const unsigned int& client_max_body_size) :
			listeningPort(listen_port), hostAddress(host_ip), serverName(server_name),
			clientMaxBodySize(client_max_body_size)
		{
		}
		virtual ~ServerConfigData()
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