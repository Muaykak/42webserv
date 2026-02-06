#include "../../include/classes/Socket.hpp"

Socket::Socket() :
_socketType(NO_TYPE),
_serversConfig(NULL),
_server_listen_port(-1),
_socketMap(NULL),
_parentSocket(NULL)
{}
Socket::Socket(const Socket &obj) :
_socketFD(obj._socketFD),
_epollFD(obj._epollFD),
_socketType(obj._socketType),
_serversConfig(obj._serversConfig),
_server_listen_port(obj._server_listen_port),
_socketMap(obj._socketMap),
_parentSocket(obj._parentSocket)
{
}
Socket::Socket(int fd) :
_socketFD(fd),
_socketType(NO_TYPE),
_serversConfig(NULL),
_server_listen_port(-1),
_socketMap(NULL),
_parentSocket(NULL)
{

}
Socket::Socket(const FileDescriptor& fd) :
_socketFD(fd),
_socketType(NO_TYPE),
_serversConfig(NULL),
_server_listen_port(-1),
_socketMap(NULL),
_parentSocket(NULL)
{
}
Socket& Socket::operator=(const Socket &obj)
{
	if (this != &obj)
	{
		_epollFD = obj._epollFD;
		_socketFD = obj._socketFD;
		_socketType = obj._socketType;
		_server_listen_port = obj._server_listen_port;
		_serversConfig = obj._serversConfig;
		_socketMap = obj._socketMap;
		_parentSocket = obj._parentSocket;
	}
	return (*this);
}
Socket::~Socket()
{

}
const FileDescriptor& Socket::getSocketFD() const
{
	return (_socketFD);
}
const FileDescriptor& Socket::getEpollFD() const
{
	return (_epollFD);
}

bool Socket::setupCGI_socket(e_socket_type cgiSocketType, const std::vector<ServerConfig> *_serversConfigVec, const FileDescriptor &epollFD,
	Socket* parentSocket, std::map<int, Socket>* socketMap, CgiProcess& cgiProcess)
{
	if (cgiSocketType != CGI_FD_STDIN && cgiSocketType != CGI_FD_STDOUT)
	{
		Logger::log(LC_RED, "ERROR::Socket::setupCGISocket::INVALID SOCKET TYPE!");
		return (false);
	}

	if (socketMap == NULL)
	{
		Logger::log(LC_RED, "ERROR::Socket::setupSocket::socketMap should not be null!");
		return (false);
	}
	if (parentSocket == NULL)
	{
		Logger::log(LC_RED, "ERROR::Socket::setupSocket::parenSocket should not be null!");
		return (false);
	}
	if (_socketFD.getFd() < 0){
		throw WebservException("Socket::setupSocket::_socketFD cannot < 0");
	}

	_epollFD = epollFD;
	_serversConfig = _serversConfigVec;
	_socketType = cgiSocketType;
	_socketMap = socketMap;
	_parentSocket = parentSocket;

	if (fcntl(_socketFD.getFd(), F_SETFL, O_NONBLOCK) != 0)
	{
		std::string errorMsg = "Socket::fcntl() O_NONBLOCK Error::";
		errorMsg += std::strerror(errno);
		throw WebservException(errorMsg);
	}

	// Add the server socket to epoll monitoring event
	epoll_event event;
	std::memset(&event, 0, sizeof(event));
	event.events = _socketType == CGI_FD_STDIN ? EPOLLOUT : EPOLLIN;
	event.data.fd = _socketFD.getFd();
	if (epoll_ctl(_epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
	{
		std::string errorMsg = "Socket::setupSocket::CLIENT_SCOKET::epoll_ctl() Error::";
		errorMsg += std::strerror(errno);
		throw(WebservException(errorMsg));
	}
	return (true);
}

// Be sure that epollFD still available !
bool Socket::setupSocket(e_socket_type socketType, const std::vector<ServerConfig> *_serversConfigVec,
	const FileDescriptor &epollFD, std::map<int, Socket>* socketMap)
{
	if (_socketType != NO_TYPE)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::SET NEW TYPE TO THE ALREADY SET ONE");
		return true;
	}
	if (socketType == NO_TYPE)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::NO_TYPE MUST NOT SETUP");
		return true;
	}
	if (socketType == CGI_FD_STDIN || socketType == CGI_FD_STDOUT)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::WRONG FUNCTION TO SETUP CGI SOCKET");
		return true;
	}
	if (socketMap == NULL)
	{
		Logger::log(LC_RED, "ERROR::Socket::setupSocket::socketMap should not be null!");
		return (true);
	}
	if (_socketFD.getFd() < 0){
		throw WebservException("Socket::setupSocket::_socketFD cannot < 0");
	}


	_epollFD = epollFD;
	_serversConfig = _serversConfigVec;
	_socketType = socketType;
	_socketMap = socketMap;
	switch (socketType)
	{
		case SERVER_SOCKET:
		{
			if (!_serversConfig)
			{
				throw WebservException("Socket::setup ServerSocket needs _serversConfig!");
			}
		
			if (fcntl(_socketFD.getFd(), F_SETFL, O_NONBLOCK) != 0)
			{
				std::string errorMsg = "Socket::fcntl() O_NONBLOCK Error::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			// reusable socket if the server was restart before port allocation timeout
			int opt = 1;
			if (setsockopt(_socketFD.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
				Logger::log(LC_NOTE, "Fail to set socket#%d for reuse socket", _socketFD.getFd());
		
			sockaddr_in sv_addr;
			std::memset(&sv_addr, 0, sizeof(sockaddr));
			sv_addr.sin_family = AF_INET;
			sv_addr.sin_addr.s_addr = INADDR_ANY;
			/*################### NEED TO FIX THIS */
			_server_listen_port = (*_serversConfig)[0].getPort();
			sv_addr.sin_port = htons(_server_listen_port);
		
			if (bind(_socketFD.getFd(), (struct sockaddr *)&sv_addr, sizeof(sv_addr)) != 0)
			{
				std::string errorMsg = "Socket::bind() failed::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			if (listen(_socketFD.getFd(), MAX_LISTENSOCKET_CONNECTION) != 0)
			{
				std::string errorMsg = "Socket::listen() failed::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			// Add the server socket to epoll monitoring event
			epoll_event event;
			std::memset(&event, 0, sizeof(event));
			event.events = EPOLLIN;
			event.data.fd = _socketFD.getFd();
			if (epoll_ctl(_epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
			{
				std::string errorMsg = "Socket::setupSocket::SERVER_SCOKET::epoll_ctl() Error::";
				errorMsg += std::strerror(errno);
				throw(WebservException(errorMsg));
			}
		
			// print Success
			// Could have better solution
			size_t	_serversConfigIndex = 0;
			size_t	serverNameVecIndex = 0;
			const std::vector<std::string> * tempServerNameVec = NULL;
			std::stringstream ss;
			ss << _server_listen_port;
			std::string portStr;
			ss >> portStr;
			while (_serversConfigIndex < _serversConfig->size())
			{
				tempServerNameVec = (*_serversConfig)[_serversConfigIndex].getServerNameVec();
				if (tempServerNameVec == NULL)
					Logger::log(LC_SYSTEM, "Server is listening on port " + portStr);
				else {
					std::string serverNameString;
					serverNameVecIndex = 0;
					while (serverNameVecIndex < tempServerNameVec->size()){
						if (serverNameVecIndex > 0)
							serverNameString += ", ";
						serverNameString += (*tempServerNameVec)[serverNameVecIndex];
						++serverNameVecIndex;
					}
					Logger::log(LC_SYSTEM, "Server <%s> is listening on port " + portStr + " (fd %d)", serverNameString.c_str(), _socketFD.getFd());
				}
				++_serversConfigIndex;
			}
		
			break;
		}
		case CLIENT_SOCKET:
		{
			_server_listen_port = (*_serversConfig)[0].getPort();
			if (fcntl(_socketFD.getFd(), F_SETFL, O_NONBLOCK) != 0)
			{
				std::string errorMsg = "Socket::fcntl() O_NONBLOCK Error::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
			if (socketMap == NULL)
			{
				Logger::log(LC_RED, "ERROR::Socket::setupSocket::socketMap should not be null!");
				return (true);
			}

			// Add the server socket to epoll monitoring event
			epoll_event event;
			std::memset(&event, 0, sizeof(event));
			event.events = EPOLLIN;
			event.data.fd = _socketFD.getFd();
			if (epoll_ctl(_epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
			{
				std::string errorMsg = "Socket::setupSocket::CLIENT_SCOKET::epoll_ctl() Error::";
				errorMsg += std::strerror(errno);
				throw(WebservException(errorMsg));
			}
		}
		default: {

			break;
		}
		return (true);
	}
	return (true);
}
// return false means this Socket should be DESTROYED after handleEVENT
bool Socket::handleEvent(const epoll_event &event)
{
	switch (_socketType)
	{
		case SERVER_SOCKET:
		{

			// Error Handling
			if ((event.events & EPOLLRDHUP) || (event.events & EPOLLHUP) || (event.events & EPOLLERR))
			{
				int error_code;
				socklen_t len = sizeof(error_code);
				if (getsockopt(_socketFD.getFd(), SOL_SOCKET, SO_ERROR, &error_code, &len) != 0)
				{
					std::string errMsg = "Socket#" + toString(_socketFD.getFd()) + "Error::getsockopt()::";
					errMsg += std::strerror(errno);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				else
				{
					std::string errMsg = "Socket#" + toString(_socketFD.getFd()) + "Error::";
					errMsg += std::strerror(error_code);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				return true;
			}
			// Request new connection from client
			else if (event.events & EPOLLIN)
			{
				sockaddr_in client_address;
				socklen_t len = sizeof(client_address);
				int client_socket;
				while (true)
				{
					client_socket = accept(event.data.fd, (sockaddr *)&client_address, &len);
					if (client_socket > 0)
					{

						Logger::log(LC_CONN_LOG, "Port %d Establishing connection from client#%d", _server_listen_port, client_socket);

						// set up client Socket
						_socketMap->insert(std::make_pair(client_socket, Socket(client_socket)));
						(*_socketMap)[client_socket].setupSocket(CLIENT_SOCKET, _serversConfig, _epollFD, _socketMap);
						continue;
					}
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						return (true);
					else if (errno == EMFILE || errno == ENFILE)
					{
						Logger::log(LC_RED, "ERROR::ServerSocker#%d::fd limit reached!", static_cast<int>(_epollFD.getFd()));
						return (true);
					}
					else if (errno == EINTR || errno == ECONNABORTED || errno == EPROTO 
					|| errno == ENETDOWN || errno == EHOSTDOWN || errno == ENONET
					|| errno == EHOSTUNREACH || errno == EOPNOTSUPP)
					{
						continue;
					}
					else
					{
						std::string errorMsg = "ServerSocket#" + toString(_socketFD.getFd()) + "::Fatal Error::";
						errorMsg += std::strerror(errno);
						throw(WebservException(errorMsg));
					}
				}
			}
			return (true);
			break;
		}
		case CLIENT_SOCKET: {

			if ((event.events & EPOLLRDHUP) || (event.events & EPOLLHUP) || (event.events & EPOLLERR))
			{
				int error_code;
				socklen_t len = sizeof(error_code);
				if (getsockopt(_socketFD.getFd(), SOL_SOCKET, SO_ERROR, &error_code, &len) != 0)
				{
					std::string errMsg = "Closing ClientSocket#" + toString(_socketFD.getFd()) + "Error::getsockopt()::";
					errMsg += std::strerror(errno);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				else
				{
					std::string errMsg = "Closing ClientSocket#" + toString(_socketFD.getFd()) + "Error::";
					errMsg += std::strerror(error_code);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				// return false to signal to remove from socket map
				return false;
			}
			if (event.events & EPOLLOUT){
				http.writeToClient(*this, *_socketMap);
				// Handle http response
			}
			if (event.events & EPOLLIN){
				http.readFromClient(*this, *_socketMap);
				// Handle http request

			}
			return http.isKeepConnection();
		}
		case CGI_FD_STDIN: {
			return true;
		}
		default:
		{
			std::string errorMsg = "Socket#" + toString(_socketFD.getFd()) + "::handleEvent()::NO_TYPE can't handle event!";
			throw(WebservException(errorMsg));
			break;
		}
	}
	return (true);
}

const std::vector<ServerConfig> *Socket::getServersConfigPtr() const
{
	return (_serversConfig);
}

int	Socket::getServerListenPort() const
{
	return (_server_listen_port);
}
