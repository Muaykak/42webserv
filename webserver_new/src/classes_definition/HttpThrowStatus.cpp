#include "../../include/classes/HttpThrowStatus.hpp"

HttpThrowStatus::HttpThrowStatus(const HttpThrowStatus& obj) throw()
: _throwMsg(obj._throwMsg),
_statusCode(obj._statusCode)
{

}

HttpThrowStatus::HttpThrowStatus(int statusCode, const std::string& throwMessage) throw()
: _throwMsg(throwMessage),
_statusCode(statusCode)
{

}
HttpThrowStatus::HttpThrowStatus(int statusCode) throw()
: _statusCode(statusCode)
{

}

HttpThrowStatus::~HttpThrowStatus() throw()
{

}

const std::string& HttpThrowStatus::message() const throw()
{
	return (_throwMsg);
}

int HttpThrowStatus::statusCode() const throw()
{
	return (_statusCode);
}
