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
# define MAX_LISTENSOCKET_CONNECTION 100

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

// Usage: like int fd = somegetfdfunction()
// The main difference is you should not assign fd that always need to open
// like 0, 1, 2
// FileDescriptor a = open(file)   and toss this class around is fine.
class FileDescriptor {
	private:
		int		_fd;
		size_t*	_reference_counter;

		void	release(){
			if (_reference_counter){
				(*_reference_counter)--;
				if (*_reference_counter == 0){
					if (close(_fd) != 0)
						std::cerr << "FileDescriptor::close() failed" << std::strerror(errno) << std::endl;
					delete _reference_counter;
					_reference_counter = NULL;
				}
			}
		}
	public:
		FileDescriptor() : _fd(-1), _reference_counter(NULL){
		}
		FileDescriptor(int fd) : _fd(fd), _reference_counter(NULL){
			if (_fd < 0 || _fd >= MAX_FD){
				std::string errMsg = "FileDescriptor::fd out of range::";
				throw WebservException(errMsg);
			}
			try {
				_reference_counter = new size_t(1);
			} catch (std::exception &e){
				if (close(_fd) != 0)
					std::cerr << "FileDescriptor::close() failed" << std::strerror(errno) << std::endl;
				throw;
			}
		};
		FileDescriptor(const FileDescriptor& obj) : _fd(obj._fd), _reference_counter(obj._reference_counter){
			if (_reference_counter)
				(*_reference_counter)++;
		}
		FileDescriptor&	operator=(const FileDescriptor &obj){
			if (this != &obj){
				release();
				_reference_counter = obj._reference_counter;
				if (_reference_counter)
					(*_reference_counter)++;
				_fd = obj._fd;
			}
			return (*this);
		}
		FileDescriptor& operator=(int fd){
			if (fd < 0 || fd >= MAX_FD){
				std::string errMsg = "FileDescriptor::fd out of range::";
				throw WebservException(errMsg);
			}
			if (_fd != fd){
				size_t	*new_reference_counter = NULL;
				try {
					new_reference_counter = new size_t(1);
				} catch (std::exception &e){
					if (close(fd) != 0)
						std::cerr << "FileDescriptor::close() failed" << std::strerror(errno) << std::endl;
					throw;
				}
				release();
				_fd = fd;
				_reference_counter = new_reference_counter;
			}
			return (*this);
		}
		~FileDescriptor(){
			release();
		}

		operator int() const {
			return _fd;
		};
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/


/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

typedef std::map<std::string, const std::string>	t_config_map;
typedef std::map<std::string, const t_config_map>	t_location_map;

class ServerConfig {
	private:
		t_config_map	_serverConfig;
		t_location_map	_locationsConfig;

		static void resolveLocationPath(std::string locationPath) {
		}
		const t_config_map* longestPrefixMatch(std::string locationPath) const {
		    while (true) {
			
		        // 2. Use map::find (Fast lookup)
		        t_location_map::const_iterator it = _locationsConfig.find(locationPath);
			
		        // 3. If found, this is inevitably the "longest" match 
		        // because we started with the longest possible string.
		        if (it != _locationsConfig.end()) {
		            return &(it->second);
		        }
			
		        // 4. If locationPath is empty or just root, and we still haven't found it, stop.
		        if (locationPath.empty() || locationPath == "/") {
		            return (NULL);
		        }
			
		        // 5. Trim logic: Remove everything after the last slash
		        size_t lastSlash = locationPath.find_last_of('/');
			
		        if (lastSlash == std::string::npos) {
		            locationPath = ""; // No slashes left? Clear it.
		        } else if (lastSlash == 0) {
		            locationPath = "/"; // Only slash was at start? Make it root.
		        } else {
		            locationPath = locationPath.substr(0, lastSlash); // Cut off the last segment
		        }
		    }
		}
	public:
		ServerConfig(){}
		ServerConfig(const ServerConfig& obj): _serverConfig(obj._serverConfig), _locationsConfig(obj._locationsConfig){
		}
		ServerConfig& operator=(const ServerConfig& obj){
			if (this != &obj){
				_serverConfig = obj._serverConfig;
				_locationsConfig = obj._locationsConfig;
			}
			return (*this);
		}
		~ServerConfig();

		std::string getServerData(const std::string& keyToFind) const {
			t_config_map::const_iterator FoundKey = _serverConfig.find(keyToFind);
			if (FoundKey == _serverConfig.end())
				return ("");
			return (FoundKey->second);
		}
		std::string	getLocationData(const std::string& locationPath, const std::string& keytoFind) const {
			const t_config_map* locationConfig = longestPrefixMatch(locationPath);
			t_config_map::const_iterator foundKey;
			if (locationConfig){
				foundKey = locationConfig->find(keytoFind);
					if (foundKey != locationConfig->end()){
						return (foundKey->second);
					}
			}
			foundKey = _serverConfig.find(keytoFind);
			if (foundKey != _serverConfig.end()){
				return (foundKey->second);
			}
			return ("");
		}
};

class ConfigData {
	private:
		// search server config by its port
		std::map<int, std::vector<ServerConfig>>	_serversConfigs;

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

class HttpRequest {

};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class HttpResponse {

};


/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class	ClientConnection {

};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

enum e_socket_type {
	NO_TYPE,
	CLIENT_SOCKET,
	SERVER_SOCKET,
	CGI_SOCKET
};

class Socket{
	private:

		FileDescriptor	_socketFD;
		e_socket_type	_socketType;
		int				_server_listen_port;

	public:
		const std::map<std::string, const ServerConfig>	*serversConfig;

		Socket() : _socketType(NO_TYPE), serversConfig(NULL), _server_listen_port(-1){};
		Socket(const Socket& obj) :  _socketFD(obj._socketFD), _socketType(obj._socketType),
		serversConfig(obj.serversConfig), _server_listen_port(obj._server_listen_port){

		}
		Socket(int fd) : _socketFD(fd), _socketType(NO_TYPE), serversConfig(NULL) {
		}
		Socket&	operator=(const Socket& obj){
			if (this != &obj){
				_socketFD = obj._socketFD;
				_socketType = obj._socketType;
				serversConfig = obj.serversConfig;
			}
			return (*this);
		}
		~Socket(){

		}

		void setupSocket(e_socket_type socketType){
			if (_socketType != NO_TYPE){
				Logger::log(LC_RED, "ERROR::Socket::setSocketType::SET NEW TYPE TO ALREADY SET ONE");
				return ;
			}
			switch (socketType) {
				case SERVER_SOCKET:
					if (!serversConfig){
						throw WebservException("Socket::setup ServerSocket needs serversConfig!");
					}
					if (_socketFD < 0){
						int temp_fd = socket(AF_INET, SOCK_STREAM, 0);
						if (temp_fd < 0){
							std::string errorMsg = "::Socket::setSocketType::socket() failed::";
							errorMsg += std::strerror(errno);
							throw WebservException(errorMsg);
						}
						_socketFD = temp_fd;
					}

					if (fcntl(_socketFD, F_SETFL, O_NONBLOCK) != 0)
					{
						std::string errorMsg = "Socket::fcntl() O_NONBLOCK Error::";
						errorMsg += std::strerror(errno);
						throw WebservException(errorMsg);
					}
					
					// reusable socket if the server was restart before port allocation timeout
					int opt = 1;
					if(setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
						Logger::log(LC_NOTE, "Fail to set socket#%d for reuse socket", _socketFD);
					
					sockaddr_in sv_addr;
					std::memset(&sv_addr, 0, sizeof(sockaddr));
					sv_addr.sin_family = AF_INET;
					sv_addr.sin_addr.s_addr = INADDR_ANY;
					/*################### NEED TO FIX THIS */
					_server_listen_port = 2345;
					sv_addr.sin_port = htons(_server_listen_port);

					if (bind(_socketFD, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) != 0){
						std::string errorMsg = "Socket::bind() failed::";
						errorMsg += std::strerror(errno);
						throw WebservException(errorMsg);
					}

					if (listen(_socketFD, MAX_LISTENSOCKET_CONNECTION) != 0){
						std::string errorMsg = "Socket::listen() failed::";
						errorMsg += std::strerror(errno);
						throw WebservException(errorMsg);
					}
					std::map<std::string, const ServerConfig>::const_iterator it = serversConfig->begin();

					std::stringstream ss;
					ss << _server_listen_port;
					std::string portStr;
					ss >> portStr;
					while (it != serversConfig->end()){
						Logger::log(LC_SYSTEM, "Server <" + it->first + "> is listening on port " + portStr);
						++it;
					}
					break;
			}
		}
};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

class WebServ {
	private:
		// the .conf file that we passed to the program
		const std::string	_webservConfigPath;
		ConfigData			_configData;
		FileDescriptor		_epollFD;

	public:
		WebServ(const std::string& configPath) : _webservConfigPath(configPath), _configData(_webservConfigPath)
		{
			// Handle ConfigPath here or
		}

		void	run();

};

/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
/*||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

#endif