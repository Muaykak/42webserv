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
				int socketFdInt = socket(AF_INET, SOCK_STREAM, 0);

				sockets.insert(std::make_pair(socketFdInt, Socket(socketFdInt)));
				sockets[socketFdInt].setupServerSocket(&serversConfigIterator->second, tempEventController, &sockets);
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

void WebServ::handleSocketEvents(std::map<int, s_webserv_event>::const_iterator& eventIt, std::map<int, s_webserv_event>& returnEvents)
{
	eventIt = returnEvents.begin();
	while (eventIt != returnEvents.end())
	{
	    std::map<int, Socket>::iterator sockIt = sockets.find(eventIt->first);
	    if (sockIt != sockets.end())
	    {
	        if (sockIt->second.handleEvent(returnEvents[eventIt->first]) == false)
	        {
	            sockets.erase(sockIt);
	        }
	    }
	    ++eventIt;
	}
}

/* true mean continue in the main checking loop*/
bool WebServ::checkCGIOUTSocketTimeOut(std::map<int, Socket>::iterator &socketIt, const time_t& currentTime)
{
	HttpCgi& httpCgi = **socketIt->second.getHttpCgi();

	if (httpCgi.status() == HTTPCGI_SENDING_TO_CGI)
	{
		/* meaning that it would okay because we not finished sending the data to cgi yet
		so the process doesn't have anything to send us yet*/
		++socketIt;
		return (true);
	}

	if (httpCgi.status() != HTTPCGI_FINISHED && httpCgi.status() != HTTPCGI_CLOSED_CGI)
	{
		if (std::difftime(currentTime, socketIt->second.getLastEventTime()) >= WEBSERV_CGI_SOCKET_TIMEOUT_SECOND)
		{
			httpCgi.forceSigTerm();
		}
	}

	if (httpCgi.status() == HTTPCGI_FINISHED)
	{
		/* check if it close the proess in the given time or not*/

		OptionalData<int> statusWait = httpCgi.getCgiProcess().waitProcess();

		if(statusWait.hasData() == true)
		{
			/* here we can specify the status of waitpid furthermore*/

			/* this mean the process is quit with sigterm and wait receives the return status
			correctly*/
			Logger::log(LC_INFO, "Closing Cgi Socket#%d::gracefully terminate the process", socketIt->first);
			sockets.erase(socketIt++);
			return (true);
		}

		/* if the statusWait has no data, meaning that the process is not terminated by
		the signal yet, so we need to check if it takes too much time*/
		if (std::difftime(currentTime, httpCgi.getCgiProcess().getTimeLastSigTimeStamp()) >= MAX_HTTP_CGI_PROCESS_WAIT_SIGTERM)
		{
			Logger::log(LC_INFO, "Closing Cgi Socket#%d::force terminate with SIGKILL to CGI process", socketIt->first);
			sockets.erase(socketIt++);
			return (true);
		}
	}
	return (false);
}

void WebServ::checkSocketTimeOut()
{
	time_t	currentTime = std::time(NULL);
	std::map<int, Socket>::iterator socketIt = sockets.begin();
	while (socketIt != sockets.end())
	{
		if (socketIt->second.getServerSockerType() == CLIENT_SOCKET && socketIt->second.timeOutMarked == false && socketIt->second.waitingResponse() == false && (std::difftime(currentTime, socketIt->second.getLastEventTime()) >= WEBSERV_CLIENT_SOCKET_TIMEOUT_SECOND))
		{
			std::map<int, s_webserv_custom_event>& customEventMap = _customEventMap;
			s_webserv_custom_event& targetCustomEvent = customEventMap[socketIt->first];
			targetCustomEvent.send408 = true;
			socketIt->second.timeOutMarked = true;
				
			/* should not silently quit the working connection, need to send 408 back to client,
			for this design we should be able to to that with sending custom event to client*/
			//Logger::log(LC_INFO, "Closing Socket#%d due to timeout.", socketIt->first);
			//sockets.erase(socketIt++);
			//continue ;
			++socketIt;
			continue;
		}
		else if (socketIt->second.getServerSockerType() == CGI_FD_STDOUT)
		{
			if (checkCGIOUTSocketTimeOut(socketIt, currentTime) == true)
				continue;
		}
		else if (socketIt->second.getServerSockerType() == CGI_FD_STDIN)
		{
			/* here is that if it takes too much time to send data CGI, what that i can
			imagine of is the body that we need to send to CGI is too large and but the main
			point is the EPOLLIN takes too much time */
			
			HttpCgi& httpCgi = **socketIt->second.getHttpCgi();

			if (httpCgi.status() == HTTPCGI_SENDING_TO_CGI)
			{
				if (std::difftime(currentTime, socketIt->second.getLastEventTime()) >= WEBSERV_CGI_SOCKET_TIMEOUT_SECOND)
				{
					Logger::log(LC_INFO, "Closing Cgi In Socket#%d::due to timeout", socketIt->first);
					sockets.erase(socketIt++);
					continue;
				}
			}
			else
			{
				/* if the httpCgiStatus is already finished sending, then should delete the socket also */
				Logger::log(LC_INFO, "Closing Cgi In Socket#%d::finished sending to CGI", socketIt->first);
				sockets.erase(socketIt++);
				continue;
			}
		}
		++socketIt;
	}
}

void WebServ::run(){
	//epoll loop
	{
		int returnEventsAmount;
		// int lastAmount = 0;
		std::map<int, s_webserv_event>::const_iterator eventIt;
		std::map<int, s_webserv_event> returnEvents;
		Logger::log(LC_DEBUG, "Webserv is Waiting for connection!");
		while (closeWebservSignal() == 0){

			returnEventsAmount = webservCheckEvent(returnEvents);

			// if (lastAmount != returnEventsAmount && returnEventsAmount >= 0)
				// Logger::log(LC_CONN_LOG, "Epoll event!:%d Total_Socket: %zu", returnEventsAmount, sockets.size());
			// lastAmount = returnEventsAmount;

			// ERROR but should retry if EINTR
			if (returnEventsAmount < 0){
				if (errno == EINTR){
					Logger::log(LC_DEBUG, "epoll_wait() got interrupted.");
					if (closeWebservSignal() == 1)
					{
						Logger::log(LC_DEBUG, "Closing the Webserv");
						break ;
					}
					Logger::log(LC_DEBUG, "Retrying... Webserv is Waiting for connection!");
					continue;
				}
				throw WebservException("epoll_wait() failed::" + std::string(std::strerror(errno)));
			}

			handleSocketEvents(eventIt, returnEvents);
			// check socket timeout
			checkSocketTimeOut();
		}
	}
}
