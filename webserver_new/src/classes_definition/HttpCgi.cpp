#include "../../include/classes/HttpCgi.hpp"

HttpCgi::HttpCgi()
: _cgiProcessOpen(NULL),
_httpResponseList(NULL)
{

}


HttpCgi::HttpCgi(bool* cgiProcOpenPtr,
std::list<HttpResponse>* httpResponseListPtr)
: _cgiProcessOpen(cgiProcOpenPtr),
_httpResponseList(_httpResponseList)
{

}

HttpCgi::HttpCgi(const HttpCgi& obj)
:_cgiProcessOpen(obj._cgiProcessOpen),
_httpResponseList(obj._httpResponseList)
{

}

HttpCgi& HttpCgi::operator=(const HttpCgi& obj)
{
	if (this != &obj)
	{
		_cgiProcessOpen = obj._cgiProcessOpen;
		_httpResponseList = obj._httpResponseList;
	}
	
	return (*this);
}

HttpCgi::~HttpCgi()
{
	
}
