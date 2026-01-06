#include "../../include/classes/WebservException.hpp"


WebservException::WebservException(const std::string &message) throw()
{

}
WebservException::~WebservException() throw() {
}

const char *WebservException::what() const throw()
{
	return (_message.c_str());
}