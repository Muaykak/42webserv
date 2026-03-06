#include "../../include/classes/WebServ.hpp"


WebServ::WebServ(const std::string &configPath) : _webservConfigPath(configPath), _configData(_webservConfigPath)
{
	// epoll creation
	try {
		_epollFD = epoll_create(1);
	} catch (std::exception &e){
		std::string epoll_msg = e.what();
		epoll_msg += "::epoll_create() failed";
		throw WebservException(epoll_msg);
	}

	//create server socket
	{
		const std::map<int, std::vector<ServerConfig> > *serversConfigMapPtr = _configData.getServersConfigMap();
		std::map<int, std::vector<ServerConfig> >::const_iterator serversConfigIterator = serversConfigMapPtr->begin();
		while (serversConfigIterator != serversConfigMapPtr->end()){
			try {
				Socket tempSocket = socket(AF_INET, SOCK_STREAM, 0);
				tempSocket.setupServerSocket(&serversConfigIterator->second, _epollFD, &sockets);
				sockets.insert(std::make_pair(tempSocket.getSocketFD().getFd(), tempSocket));
			} 
			catch (std::exception &e) {
				throw WebservException("::Create serverSocket failed::" + std::string(e.what()));
			}
			++serversConfigIterator;
		}
	}

}

void WebServ::run(){
	//epoll loop
	{
		epoll_event	events[MAX_EPOLL_EVENT];
		int returnEventsAmount;
		int lastAmount = 0;
		int	eventsIndex;
		Logger::log(LC_DEBUG, "Webserv is Waiting for connection!");
		while (true){
			returnEventsAmount = epoll_wait(_epollFD.getFd(), events, MAX_EPOLL_EVENT, WEBSERV_EPOLL_WAIT_MILLISEC);
			if (lastAmount != returnEventsAmount && returnEventsAmount >= 0)
				Logger::log(LC_CONN_LOG, "Epoll event!:%d Total_Socket: %zu", returnEventsAmount, sockets.size());
			lastAmount = returnEventsAmount;

			// ERROR but should retry if EINTR
			if (returnEventsAmount < 0){
				if (errno == EINTR){
					Logger::log(LC_DEBUG, "epoll_wait() got interrupted. Retrying...");
					Logger::log(LC_DEBUG, "Webserv is Waiting for connection!");
					continue;
				}
				throw WebservException("epoll_wait() failed::" + std::string(std::strerror(errno)));
			}

			eventsIndex = 0;
			while (eventsIndex < returnEventsAmount){
				if (sockets[events[eventsIndex].data.fd].handleEvent(events[eventsIndex]) == false){
					sockets.erase(events[eventsIndex].data.fd);
				}
				++eventsIndex;
			}

			// check socket timeout
			{
				time_t	currentTime = std::time(NULL);
				std::map<int, Socket>::iterator socketIt = sockets.begin();
				while (socketIt != sockets.end())
				{
					if (socketIt->second.getServerSockerType() == CLIENT_SOCKET)
					{
						if (std::difftime(currentTime, socketIt->second.getLastEventTime()) >= WEBSERV_CLIENT_SOCKET_TIMEOUT_SECOND)
						{
							Logger::log(LC_INFO, "Closing Socket#%d due to timeout.", socketIt->first);
							sockets.erase(socketIt++);
							continue ;
						}
					}
					++socketIt;
				}
			}
		}
	}
}
