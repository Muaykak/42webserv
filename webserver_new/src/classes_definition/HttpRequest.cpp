#include "../../include/classes/HttpRequest.hpp"
#include "../../include/classes/CharTable.hpp"
#include "../../include/utility_function.hpp"


static const CharTable& headerFieldCharset(){
	static CharTable allowedChar("", true);

	return (allowedChar);
}

static const CharTable& controlCharTable(){
	static const CharTable allowedChar("\n\f\v\r", true);

	return (allowedChar);
}

HttpRequest::HttpRequest(): _status(NO_STATUS), _errorCode(-1){
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
	size_t	currIndex = 0;
	size_t	reqBuffSize = _requestBuffer.size();
	if (_status == NO_STATUS)
		_status = READING_REQUEST_LINE;
	if (_status = READING_REQUEST_LINE){
		size_t	lineBreakPos = _requestBuffer.find_first_of("\r\n");

		// wrong format, give bad request
		if (lineBreakPos == _requestBuffer.npos){
			_status = ERR_STATUS;
			_errorCode = 400;
			return ;
		}

		// split string to temporary vector
		std::vector<std::string> tempVec;
		splitString(_requestBuffer, linearWhiteSpaceTable(), tempVec, currIndex, lineBreakPos - currIndex);

		if (tempVec.size() != 3){}
	}
}

