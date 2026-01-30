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
				tempSocket.setupSocket(SERVER_SOCKET, &serversConfigIterator->second, _epollFD, &sockets);
				sockets.insert(std::make_pair(tempSocket.getSocketFD().getFd(), tempSocket));
			} 
			catch (std::exception &e) {
				throw WebservException("::Create serverSocket failed::" + std::string(e.what()));
			}
			++serversConfigIterator;
		}
	}

	//epoll loop
	{
		epoll_event	events[MAX_EPOLL_EVENT];
		int returnEventsAmount;
		int lastAmount = 0;
		int	eventsIndex;
		Logger::log(LC_DEBUG, "Webserv is Waiting for connection!");
		while (signal_status() == 1){
			returnEventsAmount = epoll_wait(_epollFD.getFd(), events, MAX_EPOLL_EVENT, 1000);
			if (lastAmount != returnEventsAmount && returnEventsAmount >= 0)
				Logger::log(LC_CONN_LOG, "Epoll event!:%d Total_Socket: %zu", returnEventsAmount, sockets.size());
			lastAmount = returnEventsAmount;
			if (returnEventsAmount == 0)
				continue;

			// ERROR but should retry if EINTR
			if (returnEventsAmount < 0){
				if (errno == EINTR){
					if (signal_status() == 0)
						return ;
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
		}
	}
}

void WebServ::run(){

}
