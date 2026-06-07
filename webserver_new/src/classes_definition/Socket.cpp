#include "../../include/classes/Socket.hpp"
#include "../../include/classes/WebServ.hpp"

Socket::Socket() :
_socketType(NO_TYPE),
_server_listen_port(-1),
_socketMap(NULL),
_serversConfig(NULL),
handleEventPtr(NULL),
timeOutMarked(false)
{
	Logger::log(LC_DEBUG, "socket default construct called!;");
	sleep(5);
	_lastEventTime = std::time(NULL);
}
Socket::Socket(const Socket &obj) :
_socketFD(obj._socketFD),
_eventController(obj._eventController),
_socketType(obj._socketType),
_serversConfig(obj._serversConfig),
_server_listen_port(obj._server_listen_port),
_socketMap(obj._socketMap),
_server_ip_host(obj._server_ip_host),
handleEventPtr(obj.handleEventPtr),
timeOutMarked(obj.timeOutMarked)
{
	_lastEventTime = std::time(NULL);
}
Socket::Socket(int fd) :
_socketFD(fd),
_socketType(NO_TYPE),
_server_listen_port(-1),
_socketMap(NULL),
_serversConfig(NULL),
handleEventPtr(NULL),
timeOutMarked(false)
{
	_lastEventTime = std::time(NULL);

}
Socket::Socket(const FileDescriptor& fd) :
_socketFD(fd),
_socketType(NO_TYPE),
_server_listen_port(-1),
_socketMap(NULL),
_serversConfig(NULL),
handleEventPtr(NULL),
timeOutMarked(false)
{
	_lastEventTime = std::time(NULL);
}
Socket& Socket::operator=(const Socket &obj)
{
	if (this != &obj)
	{
		_lastEventTime = std::time(NULL);
		_eventController = obj._eventController;
		_socketFD = obj._socketFD;
		_socketType = obj._socketType;
		_server_listen_port = obj._server_listen_port;
		_serversConfig = obj._serversConfig;
		_socketMap = obj._socketMap;
		_server_ip_host = obj._server_ip_host;
		handleEventPtr = obj.handleEventPtr;
		timeOutMarked = obj.timeOutMarked;
	}
	return (*this);
}
Socket::~Socket()
{
	//if (_socketType != NO_TYPE)
	//{
	//	std::string tempstr = "SocketType=";

	//	if (_socketType == CLIENT_SOCKET)
	//		tempstr += "CLIENT_SOCKET";
	//	else if (_socketType == SERVER_SOCKET)
	//		tempstr += "SERVER_SOCKET";
	//	else if (_socketType == CGI_FD_STDOUT)
	//		tempstr += "CGIOUT_SOCKET";
	//	else
	//		tempstr += "CGIIN_SOCKET";

	//	Logger::log(LC_RES_NOK_LOG, "REMOVE socket#%d %s EPOLL_CTL_DEL", _socketFD.getFd(), tempstr.c_str());
	//	epoll_ctl(_eventController.epollFD.getFd(), EPOLL_CTL_DEL, _socketFD.getFd(), NULL);
	//}
}
const FileDescriptor& Socket::getSocketFD() const
{
	return (_socketFD);
}
const FileDescriptor& Socket::getEpollFD() const
{
	return (_eventController.epollFD);
}

const s_webserv_event_controller& Socket::getEventContoller() const
{
	return (_eventController);
}

void Socket::setServerIpHost(const std::set<in_addr_t>& obj)
{
	_server_ip_host = obj;
}

const std::set<in_addr_t>& Socket::getServerIpHost() const
{
	return (_server_ip_host);
}

bool Socket::setupCGIOUTSocket(const std::vector<ServerConfig> *serversConfig,
	const s_webserv_event_controller& eventController, std::map<int, Socket>* socketMap, const Shared<HttpCgi>& cgiData)
{

	if (_socketType != NO_TYPE)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::SET NEW TYPE TO THE ALREADY SET ONE");
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

	_eventController = eventController;
	_socketType = CGI_FD_STDOUT;
	_socketMap = socketMap;
	_serversConfig = serversConfig;

	httpCgi = cgiData;

	epoll_event event;
	std::memset(&event, 0, sizeof(event));
	event.events = EPOLLIN;
	event.data.fd = _socketFD.getFd();
	if (epoll_ctl(_eventController.epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
	{
		std::string errorMsg = "Socket::setupSocket::SERVER_SCOKET::epoll_ctl() Error::";
		errorMsg += std::strerror(errno);
		throw(WebservException(errorMsg));
	}

	_lastEventTime = std::time(NULL);

	handleEventPtr = &Socket::handleCGISocketEvent;
	return (true);
}

bool Socket::setupCGIINSocket(const std::vector<ServerConfig> *serversConfig,
	const s_webserv_event_controller &eventController, std::map<int, Socket>* socketMap, const Shared<HttpCgi>& cgiData)
{

	if (_socketType != NO_TYPE)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::SET NEW TYPE TO THE ALREADY SET ONE");
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

	_eventController = eventController;
	_socketType = CGI_FD_STDIN;
	_socketMap = socketMap;
	_serversConfig = serversConfig;

	httpCgi = cgiData;

	epoll_event event;
	std::memset(&event, 0, sizeof(event));
	event.events = EPOLLOUT;
	event.data.fd = _socketFD.getFd();
	if (epoll_ctl(_eventController.epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
	{
		std::string errorMsg = "Socket::setupSocket::SERVER_SCOKET::epoll_ctl() Error::";
		errorMsg += std::strerror(errno);
		throw(WebservException(errorMsg));
	}

	_lastEventTime = std::time(NULL);

	handleEventPtr = &Socket::handleCGISocketEvent;
	return (true);
}


bool Socket::setupServerSocket(const std::vector<ServerConfig> *serversConfig,
		const s_webserv_event_controller &eventController, std::map<int, Socket>* socketMap)
{
	if (_socketType != NO_TYPE)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::SET NEW TYPE TO THE ALREADY SET ONE");
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

	_eventController = eventController;
	_socketType = SERVER_SOCKET;
	_socketMap = socketMap;
	_serversConfig = serversConfig;

	if (!_serversConfig)
	{
		throw WebservException("Socket::setup ServerSocket needs _serversConfig!");
	}
	_serversConfig = serversConfig;

	std::set<in_addr_t> temp_addr_set;
	// put serversConfig ip host together
	{
		std::vector<ServerConfig>::const_iterator	vecServerIt = _serversConfig->begin();
		while (vecServerIt != _serversConfig->end())
		{
			temp_addr_set = vecServerIt->getHostIp();

			// if any of server config doesn't have host ip. 
			// meaning that server socket should accept any ip connection
			if (temp_addr_set.size() == 0)
			{
				_server_ip_host.clear();
				break ;
			}
			else
				_server_ip_host.insert(temp_addr_set.begin(), temp_addr_set.end());
			++vecServerIt;
		}
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
	if (epoll_ctl(_eventController.epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
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
		tempServerNameVec = &(*_serversConfig)[_serversConfigIndex].getServerNameVec();
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

	_lastEventTime = std::time(NULL);

	handleEventPtr = &Socket::handleServerSocketEvent;

	return (true);
}

bool Socket::setupClientSocket(const std::vector<ServerConfig> *serversConfig,
	const s_webserv_event_controller &eventController, std::map<int, Socket>* socketMap)
{
	if (_socketType != NO_TYPE)
	{
		Logger::log(LC_RED, "ERROR::Socket::setSocketType::SET NEW TYPE TO THE ALREADY SET ONE");
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

	_eventController = eventController;
	_socketType = CLIENT_SOCKET;
	_socketMap = socketMap;
	_serversConfig = serversConfig;

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
	if (epoll_ctl(_eventController.epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
	{
		std::string errorMsg = "Socket::setupSocket::CLIENT_SCOKET::epoll_ctl() Error::";
		errorMsg += std::strerror(errno);
		throw(WebservException(errorMsg));
	}

	http = Http(this, socketMap);

	_lastEventTime = std::time(NULL);

	handleEventPtr = &Socket::handleClientSocketEvent;

	return (true);
}

bool Socket::handleServerSocketEvent(const s_webserv_event &event)
{
	if (event.epollEventData.hasData() == true)
	{
		// Error Handling
		if ((event.epollEventData->events & EPOLLRDHUP) || (event.epollEventData->events & EPOLLHUP) || (event.epollEventData->events & EPOLLERR))
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
		else if (event.epollEventData->events & EPOLLIN)
		{
			sockaddr_in client_address;
			std::memset(&client_address, 0, sizeof(sockaddr_in));
			socklen_t len = sizeof(client_address);
			int client_socket;
			while (true)
			{
				client_socket = accept(event.epollEventData->data.fd, (sockaddr *)&client_address, &len);
				if (client_socket > 0)
				{
					// need to check if cilent access to this server with the same ip that server recieves
					if (_server_ip_host.size() != 0
					&& _server_ip_host.find(client_address.sin_addr.s_addr) == _server_ip_host.end())
					{
						Logger::log(LC_CON_FAIL, "Incoming connection does not match any server %s", in_addr_t_to_string(client_address.sin_addr.s_addr).c_str());
						close(client_socket);
						continue;
					}
					Logger::log(LC_CONN_LOG, "Connection from %s", in_addr_t_to_string(client_address.sin_addr.s_addr).c_str());

					Logger::log(LC_CONN_LOG, "Port %d Establishing connection from client#%d", _server_listen_port, client_socket);

					// set up client Socket
					_socketMap->insert(std::make_pair(client_socket, Socket(client_socket)));
					(*_socketMap)[client_socket]._client_addr_in = client_address.sin_addr.s_addr;
					(*_socketMap)[client_socket].setServerIpHost(_server_ip_host);
					(*_socketMap)[client_socket].setupClientSocket(_serversConfig, _eventController, _socketMap);

					continue;
				}
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					return (true);
				}
				else if (errno == EMFILE || errno == ENFILE)
				{
					Logger::log(LC_RED, "ERROR::ServerSocker#%d::fd limit reached!", static_cast<int>(_eventController.epollFD.getFd()));
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
	}

	if (event.customEventData.hasData() == true)
	{
		/* custom event for server socket?? don't know yet */
		return (true);
	}
	return (true);

}

bool Socket::handleClientSocketEvent(const s_webserv_event &event)
{
	/* for custom event in client socket */
	if (event.epollEventData.hasData() == true)
	{
		if ((event.epollEventData->events & EPOLLRDHUP) || (event.epollEventData->events & EPOLLHUP) || (event.epollEventData->events & EPOLLERR))
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
		try {

			if (event.epollEventData->events & EPOLLOUT){
				http->writeToClient();
				// Handle http response
			}
			if ((event.epollEventData->events & EPOLLIN) && timeOutMarked == false){
				http->readFromClient();
				// Handle http request
			}

		}
		// should not have any throw in normal circum stance
		/*
			though the error that got thrown here can be

			epoll_ctl() error because we lose the control
			and best way to handle is just to close this socket
		*/
		catch (std::exception &e)
		{
			Logger::log(LC_ERROR, "socket#%d unexpected error occured closing this connection::%s", _socketFD.getFd(), e.what());
			return (false);
		}
		//catch (...)
		//{
		//	Logger::log(LC_ERROR, "socket#%d unexpected error occured closing this connection", _socketFD.getFd());
		//	return (false);
		//}

		if (http->isKeepConnection() == false)
			return (false);
	}

	if (event.customEventData.hasData() == true)
	{
		if (event.customEventData->clientSocketManualDisconnect.hasData() == true)
		{
			 /* here means we want to terminate the client immediately */
			if (*event.customEventData->clientSocketManualDisconnect == true)
				return (false);
		}
		// process local redirect here
		if (event.customEventData->httpRequestData.hasData() == true)
			http->directRequestProcess(event.customEventData->httpRequestData);

		if (event.customEventData->send408.hasData() == true)
		{
			if (event.epollEventData.hasData() == false)
				http->send408();
			else
				timeOutMarked = false;
		}
	}

	return http->isKeepConnection();
}

bool Socket::handleCGISocketEvent(const s_webserv_event& event)
{
	// EPOLLIN is coming here 
	/*
		we dont need to check what the event is
		because we don't know if we fully read all from the buffer yet
		whether what event is

		it is different from Client Socket that we need the
		connection so we can send back to the same socket

		in this case we don't need to check what event came here

		just read() only once and check the errno() would be more practical

		no we cannot check errno according to the subject
	*/

	/*
		EPOLLOUT will be here
		and we need to send the temporay file body to the CGI process
	*/
	(*httpCgi)->processCGI(this, **event.epollEventData);

	return (*httpCgi)->isKeepConnection(this);
}

// return false means this Socket should be DESTROYED after handleEVENT
bool Socket::handleEvent(const s_webserv_event &event)
{
	_lastEventTime = std::time(NULL);

	//std::cout << "   HANDLING ";
	//if (_socketType == SERVER_SOCKET)
	//	std::cout << "SERVER_SOCKET" << std::endl;
	//else if (_socketType == CLIENT_SOCKET)
	//	std::cout << "CLIENT_SOCKET" << std::endl;
	//else if (_socketType == CGI_FD_STDOUT)
	//	std::cout << "CGI OUT SOCKET" << std::endl;
	//else
	//	std::cout << "NO_TYPE" << std::endl;

	if (handleEventPtr)
		return (this->*handleEventPtr)(event);
	else
	{
		std::string errorMsg = "Socket#" + toString(_socketFD.getFd()) + "::handleEvent()::NO_TYPE can't handle event!";
		throw(WebservException(errorMsg));
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

time_t	Socket::getLastEventTime() const
{
	return (_lastEventTime);
}

e_socket_type Socket::getServerSockerType() const
{
	return (_socketType);
}

OptionalData<Shared<HttpCgi> >& Socket::getHttpCgi()
{
	return (httpCgi);
}
