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
		s_webserv_event_controller tempEventController;
		tempEventController.customEventMap = &_customEventMap;
		tempEventController.epollFD = _epollFD;
		while (serversConfigIterator != serversConfigMapPtr->end()){
			try {
				Socket tempSocket = socket(AF_INET, SOCK_STREAM, 0);
				tempSocket.setupServerSocket(&serversConfigIterator->second, tempEventController, &sockets);
				sockets.insert(std::make_pair(tempSocket.getSocketFD().getFd(), tempSocket));
			} 
			catch (std::exception &e) {
				throw WebservException("::Create serverSocket failed::" + std::string(e.what()));
			}
			++serversConfigIterator;
		}
	}

}

// return amount of event for quick check like epoll_wait
int WebServ::webservCheckEvent(std::map<int , s_webserv_event>& returnEvents)
{
	returnEvents.clear();
	
	/* here we would need to use the epoll_wait*/
	int epollWaitReturnEvent = epoll_wait(_epollFD.getFd(), _epollEvents, MAX_EPOLL_EVENT, WEBSERV_EPOLL_WAIT_MILLISEC);

	/* if the return value is less than 0 should return back because some error might occur*/
	if (epollWaitReturnEvent < 0)
		return (epollWaitReturnEvent);
	/* if more than 0 then we create the return event structure map*/

	/* iterate to all events*/
	for (int i = 0; i < epollWaitReturnEvent; i++)
	{
		returnEvents[_epollEvents[i].data.fd].epollEventData = &_epollEvents[i];
		returnEvents[_epollEvents[i].data.fd].eventFd = _epollEvents[i].data.fd;
	}

	// then for custom events
	std::map<int, s_webserv_custom_event>::const_iterator it = _customEventMap.begin();
	while (it != _customEventMap.end())
	{
		returnEvents[it->first].customEventData = it->second;
		returnEvents[it->first].eventFd = it->first;
		++it;
	}

	/* then need to clear the _customEventMap */
	_customEventMap.clear();

	/* return the size of the map*/
	return (returnEvents.size());
}

void WebServ::run(){
	//epoll loop
	{
		int returnEventsAmount;
		int lastAmount = 0;
		std::map<int, s_webserv_event>::const_iterator eventIt;
		std::map<int, s_webserv_event> returnEvents;
		Logger::log(LC_DEBUG, "Webserv is Waiting for connection!");
		while (true){

			returnEventsAmount = webservCheckEvent(returnEvents);

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

			eventIt = returnEvents.begin();
			while (eventIt != returnEvents.end())
			{
				sockets[eventIt->first].handleEvent(returnEvents[eventIt->first]);
				++eventIt;
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
