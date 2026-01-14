#include "../../include/classes/HttpRequest.hpp"
#include "../../include/classes/CharTable.hpp"
#include "../../include/utility_function.hpp"

#include <set>

static const CharTable& headerFieldCharset(){
	static CharTable allowedChar("", true);

	return (allowedChar);
}

static const CharTable& controlCharTable(){
	static const CharTable allowedChar("\n\f\v\r", true);

	return (allowedChar);
}

const std::set<std::string>& allowMethod(){
	static std::set<std::string> method;
	if (method.empty()){
		method.insert("GET");
		method.insert("POST");
		method.insert("DELETE");
	}
	return (method);
}

HttpRequest::HttpRequest():
_status(NO_STATUS), _errorCode(-1), _method(NO_METHOD){
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
		if (reqBuffSize > MAX_REQUEST_BUFFER_SIZE){
			_status = ERR_STATUS;
			_errorCode = 400;
			return;
		}

		// get the method
		if (_method == NO_METHOD){

			// skip any empty line '\r\n' before request line
			while (currIndex + 1 < reqBuffSize
			&& _requestBuffer[currIndex] == '\r'
			&& _requestBuffer[currIndex + 1] == '\n'){
				currIndex += 2;
			}

			if (currIndex >= reqBuffSize){
				_requestBuffer.clear();
				return ;
			}
			std::string methodStr;

			size_t	pos = linearWhiteSpaceTable().findFirstCharset(_requestBuffer, currIndex);
			methodStr = pos == _requestBuffer.npos ? _requestBuffer.substr(currIndex) : _requestBuffer.substr(currIndex, pos);

			if (methodStr == "GET") {
				_method = GET;
			}
			else if (methodStr == "POST") {
				_method = POST;
			}
			else if (methodStr == "DELETE") {
				_method = DELETE;
			}
			else {
				_status = ERR_STATUS;
				_errorCode = 400;
				return ;
			}
		}
	}
}

