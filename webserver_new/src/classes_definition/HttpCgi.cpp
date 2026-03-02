#include "../../include/classes/HttpCgi.hpp"

HttpCgi::HttpCgi()
:_clientSocketHttp(NULL)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}


HttpCgi::HttpCgi(Http* clientSocketHttp, const FileDescriptor& mainHttpSocketFd)
:_clientSocketHttp(clientSocketHttp),
_mainHttpSocketFd(mainHttpSocketFd)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::HttpCgi(const HttpCgi& obj)
:_clientSocketHttp(obj._clientSocketHttp),
_mainHttpSocketFd(obj._mainHttpSocketFd)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::~HttpCgi()
{
	
}

Http* HttpCgi::getClientSocketHttp() const
{
	return (_clientSocketHttp);
}
