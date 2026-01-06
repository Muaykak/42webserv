#include "../../include/classes/Socket.hpp"

Socket::Socket() : _socketType(NO_TYPE), _serversConfig(NULL), _server_listen_port(-1) {}
Socket::Socket(const Socket &obj) : _socketFD(obj._socketFD), _epollFD(obj._epollFD), _socketType(obj._socketType),
							_serversConfig(obj._serversConfig), _server_listen_port(obj._server_listen_port)
{
}
Socket::Socket(int fd) : _socketFD(fd), _socketType(NO_TYPE), _serversConfig(NULL), _server_listen_port(-1)
{
}
Socket::Socket(const FileDescriptor& fd) : _socketFD(fd), _socketType(NO_TYPE), _serversConfig(NULL), _server_listen_port(-1)
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

// Be sure that epollFD still available !
bool Socket::setupSocket(e_socket_type socketType, const std::vector<ServerConfig> *_serversConfigVec, const FileDescriptor &epollFD)
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
	if (_socketFD < 0){
		throw WebservException("Socket::setupSocket::_socketFD cannot < 0");
	}


	_epollFD = epollFD;
	_serversConfig = _serversConfigVec;
	_socketType = socketType;
	switch (socketType)
	{
		case SERVER_SOCKET:
		{
			if (!_serversConfig)
			{
				throw WebservException("Socket::setup ServerSocket needs _serversConfig!");
			}
		
			if (fcntl(_socketFD, F_SETFL, O_NONBLOCK) != 0)
			{
				std::string errorMsg = "Socket::fcntl() O_NONBLOCK Error::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			// reusable socket if the server was restart before port allocation timeout
			int opt = 1;
			if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
				Logger::log(LC_NOTE, "Fail to set socket#%d for reuse socket", _socketFD);
		
			sockaddr_in sv_addr;
			std::memset(&sv_addr, 0, sizeof(sockaddr));
			sv_addr.sin_family = AF_INET;
			sv_addr.sin_addr.s_addr = INADDR_ANY;
			/*################### NEED TO FIX THIS */
			_server_listen_port = (*_serversConfig)[0].getPort();
			sv_addr.sin_port = htons(_server_listen_port);
		
			if (bind(_socketFD, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) != 0)
			{
				std::string errorMsg = "Socket::bind() failed::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			if (listen(_socketFD, MAX_LISTENSOCKET_CONNECTION) != 0)
			{
				std::string errorMsg = "Socket::listen() failed::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			// Add the server socket to epoll monitoring event
			epoll_event event;
			std::memset(&event, 0, sizeof(event));
			event.events = EPOLLIN;
			event.data.fd = _socketFD;
			if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &event) != 0)
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
					Logger::log(LC_SYSTEM, "Server <%s> is listening on port " + portStr, serverNameString.c_str());
				}
				++_serversConfigIndex;
			}
		
			break;
		}
		case CLIENT_SOCKET:
		{
			if (fcntl(_socketFD, F_SETFL, O_NONBLOCK) != 0)
			{
				std::string errorMsg = "Socket::fcntl() O_NONBLOCK Error::";
				errorMsg += std::strerror(errno);
				throw WebservException(errorMsg);
			}
		
			// register to epoll
			epoll_event event;
			event.events = EPOLLIN;
			event.data.fd = _socketFD;
			if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &event) != 0)
			{
				std::string errorMsg = "Socket::setupSocket::CLIENT_SCOKET::epoll_ctl() Error::";
				errorMsg += std::strerror(errno);
				throw(WebservException(errorMsg));
			}
			break;
		}
		// Things that fall here are CGI_FD_STDOUT and CGI_FD_STDERR
		default: {

			break;
		}
		return (true);
	}
	return (true);
}
// return false means this Socket should be DESTROYED after handleEVENT
bool Socket::handleEvent(const epoll_event &event, std::map<int, Socket> &socketMap)
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
				if (getsockopt(_socketFD, SOL_SOCKET, SO_ERROR, &error_code, &len) != 0)
				{
					std::string errMsg = "Socket#" + toString(_socketFD) + "Error::getsockopt()::";
					errMsg += std::strerror(errno);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				else
				{
					std::string errMsg = "Socket#" + toString(_socketFD) + "Error::";
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
						socketMap.insert(std::make_pair(client_socket, Socket(client_socket)));
						socketMap[client_socket].setupSocket(CLIENT_SOCKET, _serversConfig, _epollFD);
						continue;
					}
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						return (true);
					else if (errno == EMFILE || errno == ENFILE)
					{
						Logger::log(LC_RED, "ERROR::ServerSocker#%d::fd limit reached!", static_cast<int>(_epollFD));
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
						std::string errorMsg = "ServerSocket#" + toString(_socketFD) + "::Fatal Error::";
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
				if (getsockopt(_socketFD, SOL_SOCKET, SO_ERROR, &error_code, &len) != 0)
				{
					std::string errMsg = "Closing ClientSocket#" + toString(_socketFD) + "Error::getsockopt()::";
					errMsg += std::strerror(errno);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				else
				{
					std::string errMsg = "Closing ClientSocket#" + toString(_socketFD) + "Error::";
					errMsg += std::strerror(error_code);
					Logger::log(LC_CONN_LOG, errMsg);
				}
				// return false to signal to remove from socket map
				return false;
			}
			else if (event.events & EPOLLOUT){
				http.response(*this, socketMap);
				// Handle http response
			} else if (event.events & EPOLLIN){
				http.request(*this, socketMap);
				// Handle http request

			}
			return true;
		}
		case CGI_FD_STDOUT: {
			return true;
		}
		default:
		{
			std::string errorMsg = "Socket#" + toString(_socketFD) + "::handleEvent()::NO_TYPE can't handle event!";
			throw(WebservException(errorMsg));
			break;
		}
	}
	return (true);
}

const std::vector<ServerConfig> *Socket::getServersConfigPtr() const {
	return (_serversConfig);
}