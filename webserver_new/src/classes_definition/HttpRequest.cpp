#include "../../include/classes/HttpRequest.hpp"


HttpRequest::HttpRequest(): _status(NO_STATUS){
}
HttpRequest::HttpRequest(const HttpRequest& obj)
: _requestBuffer(obj._requestBuffer), _status(obj._status){

}
HttpRequest& HttpRequest::operator=(const HttpRequest& obj){
	if (this != &obj){
		_status = obj._status;
		_requestBuffer = obj._requestBuffer;
	}
	return (*this);
}
HttpRequest::~HttpRequest(){
}

e_http_request_status	HttpRequest::getStatus() const {
	return (_status);
}

void	HttpRequest::processingReadBuffer(const std::string& readBuffer, ssize_t readAmount){
	_requestBuffer.append(readBuffer.begin(), readBuffer.begin() + readAmount);


}

