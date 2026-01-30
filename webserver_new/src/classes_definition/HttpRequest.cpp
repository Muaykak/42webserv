#include "../../include/classes/HttpRequest.hpp"
#include "../../include/classes/CharTable.hpp"
#include "../../include/utility_function.hpp"

#include <set>

void	HttpRequest::httpError(int errorCode){
	_status = ERR_STATUS;
	_errorCode = errorCode;
}

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
:
	_status(obj._status),
	_errorCode(obj._errorCode),
	_headerField(_headerField),
	_method(obj._method),
	_requestTarget(obj._requestTarget),
	_protocol(obj._protocol){

}
HttpRequest& HttpRequest::operator=(const HttpRequest& obj){
	if (this != &obj){
		_status = obj._status;
	}
	return (*this);
}
HttpRequest::~HttpRequest(){
}

e_http_request_status	HttpRequest::getStatus() const {
	return (_status);
}

void	HttpRequest::processingReadBuffer(std::string& readBuffer){
	size_t	currIndex = 0;
	size_t	reqBuffSize = readBuffer.size();
	if (_status == NO_STATUS)
		_status = READING_REQUEST_LINE;
	if (_status = READING_REQUEST_LINE){
		if (reqBuffSize > MAX_REQUEST_BUFFER_SIZE){
			httpError(400);
		}

		// get the method
		if (_method == NO_METHOD){
			/*
			Although the line terminator for the start-line and
			fields is the sequence CRLF, a recipient MAY recognize a
			single LF as a line terminator and ignore any preceding CR.

			According to RFC9112  HTTP/1.1
			*/

			// skip any empty line '\r\n' or '\n' before request line
			while (currIndex + 1 < reqBuffSize){
				if (readBuffer[currIndex] == '\r') {
					// '\r' must always followed by '\n'
					if (readBuffer[currIndex + 1] != '\n') {

					}
					currIndex += 2;
					continue;
				}
				if (readBuffer[currIndex] == '\n')
					++currIndex;
			}

			if (currIndex >= reqBuffSize){
				readBuffer.clear();
				return ;
			}
			// now currIndex sits at the first char of the 
			// request line

			// I need the whole request line first
			// before process anything
			size_t	endLinePos = crlf().findFirstCharset(readBuffer, currIndex);

			// doesn't find any endline delimiter
			// will process later
			if (endLinePos == readBuffer.npos){
				readBuffer = readBuffer.substr(currIndex);
				return ;
			}

			if (readBuffer[endLinePos] == '\r'){

				// wait for next round
				if (endLinePos + 1 == readBuffer.size())
					return ;

				// if '\r' then always need to check for  '\n'
				else if (readBuffer[endLinePos + 1] != '\n'){
					httpError(400);
					return ;
				}
			}

			// found endLinePos
			// each error code we gave should respect the document RFC9112 (HTTP 1.1)

			/*
			Although the request-line grammar rule 
			requires that each of the component elements 
			be separated by a single SP octet, recipients MAY 
			instead parse on whitespace-delimited word boundaries 
			and, aside from the CRLF terminator, treat any form of 
			whitespace as the SP separator while ignoring preceding or 
			trailing whitespace; such whitespace includes one or more of 
			the following octets: SP, HTAB, VT (%x0B), FF (%x0C), 
			or bare CR. However, lenient parsing can result in request 
			smuggling security vulnerabilities if there are multiple 
			recipients of the message and each has its own unique 
			interpretation of robustness (see Section 11.2).

				RFC9112
			*/

			std::string methodStr;

			// find pos any whitespaces but not '\n'
			size_t	pos = whiteSpaceNoLFtable().findFirstCharset(readBuffer, currIndex, endLinePos - currIndex);
			methodStr = pos == readBuffer.npos ? readBuffer.substr(currIndex) : readBuffer.substr(currIndex, pos - currIndex);

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
				httpError(400);
				return ;
			}
			// skip 1 pos which is the first whitespace
			currIndex = pos + 1;
			// should not reach endlinePos yet
			if (currIndex >= endLinePos){
				httpError(400);
			}
			// now get the request target.
			pos = whiteSpaceNoLFtable().findFirstCharset(readBuffer, currIndex, endLinePos - currIndex);
			// must can find next whitespace.. it IS the format
			if (pos == readBuffer.npos){
				httpError(400);
			}
			_requestTarget = readBuffer.substr(currIndex, pos - currIndex);

			// must not empty
			if (_requestTarget.empty()){
				httpError(400);
			}

			// then move our currIndex
			currIndex = pos + 1;
			// prevent edge case where now currIndex might right at the endLinepos
			// should not reach endlinePos yet
			if (currIndex >= endLinePos){
				httpError(400);
			}

			// now the last part is protocol version
			_protocol = readBuffer.substr(currIndex, endLinePos - currIndex);

			// check must not empty
			if (_protocol.empty()){
				httpError(400);
			}

			// we get all the request line then we move to reading the header field
			_status = READING_HEADER;
			// move Index to next line
			currIndex = readBuffer[endLinePos] == '\r' ? endLinePos + 2 : endLinePos + 1;
		}
	}

	// now we get all the header fields
	while (_status == READING_HEADER){

		// field-line   = field-name ":" OWS field-value OWS
		/*
			field-value    = *field-content
			field-content  = field-vchar
			                 [ 1*( SP / HTAB / field-vchar ) field-vchar ]
			field-vchar    = VCHAR / obs-text
			obs-text       = %x80-FF
		*/
	
	}
}

