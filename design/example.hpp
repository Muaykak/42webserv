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
# include <sys/stat.h>
# include <cerrno>
# include <cstring>
# include <cstdlib>
# include <fcntl.h>
# include <iostream>
# include <fstream>
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

// Utility Functions

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
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

typedef std::map<std::string, std::string>	t_config_map;
typedef std::map<std::string, t_config_map>	t_location_map;

class ServerConfig {
	private:
		t_config_map	_serverConfig;
		t_location_map	_locationConfig;
};

class ConfigData {
	private:
		std::map<std::string, ServerConfig>	_serversConfigs;

		std::vector<std::string>	splitToken(std::string& readBuffer)
		{
			std::vector<std::string>	tokens;

			size_t	indexFound;
			while (!readBuffer.empty()){

			indexFound = readBuffer.find_first_not_of(" \f\n\r\t\v#;");

			}
		}
	public:
		ConfigData(const std::string& configPath)
		{
			// Checking File extention (.conf)
			if (configPath.empty() || configPath.substr(configPath.size() - 5) != ".conf"){
				throw WebservException("Wrong file extension!");
			}
			// Check file type
			{
				struct stat statbuf;
				std::string errorMsg = "Checking file type <" + configPath + ">";
				if (stat(configPath.c_str(), &statbuf) == -1){
					errorMsg += "::stat() failed";
					errorMsg += std::strerror(errno);
					throw WebservException(errorMsg);
				}
				if (S_ISDIR(statbuf.st_mode) == true){
					errorMsg += "::Is A Directory";
					throw WebservException(errorMsg);
				}
			}

			std::ifstream	configFile(configPath.c_str());
			if (!configFile.is_open()){
				std::string errorMsg = "Cannot Open Config File!::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}

			// The actual read 
			{
				std::string readBuffer;

				while (configFile.good()){
					std::getline(configFile, readBuffer);
					std::cout << readBuffer;
				}
			}
			// throw SomeExceptions Error when something wrong
		}
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class WebServ {
	private:
		// the .conf file that we passed to the program
		const std::string	_webservConfigPath;
		ConfigData			_configData;

	public:
		WebServ(const std::string& configPath) : _webservConfigPath(configPath), _configData(_webservConfigPath)
		{
			// Handle ConfigPath here or
		}
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

#endif