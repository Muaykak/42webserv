#include "../../include/classes/HttpCgi.hpp"
#include "../../include/classes/Socket.hpp"

HttpCgi::HttpCgi()
:_clientResponseList(NULL),
_cgiTargetResponse(NULL)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::HttpCgi(std::list<HttpResponse>* clientResponseList,
HttpResponse* cgiTargetResponse,
const FileDescriptor& mainHttpSocket,
Socket *thisCgiSocket)
: _clientResponseList(clientResponseList),
_cgiTargetResponse(cgiTargetResponse),
_thisCgiSocket(thisCgiSocket),
_mainHttpSocketFd(mainHttpSocket)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::HttpCgi(const HttpCgi& obj)
: _clientResponseList(obj._clientResponseList),
_cgiTargetResponse(obj._cgiTargetResponse),
_thisCgiSocket(obj._thisCgiSocket),
_mainHttpSocketFd(obj._mainHttpSocketFd)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::~HttpCgi()
{
	
}

void HttpCgi::readFromCGI()
{
	// use read() right?
	ssize_t	readAmount;

	readAmount = read(_thisCgiSocket->getSocketFD().getFd(), &_readCgiBuffer[0], HTTP_READ_FROM_CGI_BUFFER_SIZE);
	// this mean done
	if (readAmount == 0)
	{
		Logger::log(LC_CONN_LOG, "CGI socket out (has nothing to read).#%d:: Disconnecting..", _thisCgiSocket->getSocketFD().getFd());
		_isFinishedRead = true;
	}
	else if (readAmount < 0)
	{
		return ;
	}
	else
	{
		_responseBuffer.append(&_readCgiBuffer[0], readAmount);
		
	}
}

void HttpCgi::sendToCGI()
{
	ssize_t	writeAmount;
}
