#include "../../include/classes/HttpResponse.hpp"
#include "../../include/classes/Socket.hpp"

HttpResponse::HttpResponse()
:_statusCode(1000),
_canModify(true),
_keepAfterResponse(false),
_responseBodyType(HTTP_RESPONSE_NOBODY),
_isComplete(false),
_hasSomethingtoSend(false),
_isReachEOF(false),
_isCgiFinished(false),
_sendBufferOffset(0)
{
_sendBuffer.reserve(HTTP_SEND_BUFFER);
}

void	HttpResponse::addHeader(const std::string& headerName, const std::string &headerValue)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	if (!headerName.empty())
	{
		_responseHeader[headerName].insert(headerValue);
	}
}

bool HttpResponse::getKeepAfterResponse() const
{
	return (_keepAfterResponse);
}

void HttpResponse::setKeepAfterResponse(bool op)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_keepAfterResponse = op;
}

int HttpResponse::getStatusCode() const
{
	return (_statusCode);
}

void HttpResponse::setStatusCode(unsigned int statusCode)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_statusCode = statusCode;
}

const std::string& HttpResponse::getStatusMessage() const
{
	return (_statusMessage);
}

void HttpResponse::setStatusMessage(const std::string& statusMessage)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_statusMessage = statusMessage;
}

const std::string& HttpResponse::getContentType() const
{
	return (_contentType);
}
void HttpResponse::setContentType(const std::string& contentType)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_contentType = contentType;
}

e_http_response_body_type HttpResponse::getResponseBodyType() const
{
	return (_responseBodyType);
}
void HttpResponse::setResponseBodyType(e_http_response_body_type bodyType)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_responseBodyType = bodyType;
}

const std::string& HttpResponse::getFixedBodyStr() const
{
	return (_fixedBodyStr);
}
void HttpResponse::setFixedBodyStr(const std::string& bodyStr)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_fixedBodyStr = bodyStr;
}

const FileDescriptor& HttpResponse::getFileFd() const
{
	return (_fileFd);
}

void HttpResponse::setFileFd(const FileDescriptor& fd)
{
	if (_canModify == false)
		throw WebservException("HttpResponse::cannot modify response when not in modifying state");

	_fileFd = fd;
}

void	HttpResponse::generateResponse()
{
	if (_canModify = false)
		throw WebservException("HttpResponse:: can only generate response 1 time only");
	
	_canModify = false;

	// some safety checking
	if (_statusCode >= 1000)
		throw WebservException("HttpResponse:: _statusCode must not >= 1000 ");
	
	if (_bufferList.size() != 0)
		throw WebservException("HttpResponse:: _bufferList size() != 0 something wrong");

	std::string tempStr;

	tempStr += "HTTP/1.1 " + toString(_statusCode) + " " +_statusMessage + "\r\n";

	//check for Date:
	{
		time_t	rawTimeData = std::time(NULL);

		tm * timeGmt = std::gmtime(&rawTimeData);

		std::vector<char> tempTimeBuffer(100);

		std::strftime(&tempTimeBuffer[0], tempTimeBuffer.size(), "%a, %d %b %Y %H:%M:%S GMT", timeGmt);

		tempStr += "Date: ";
		tempStr += &tempTimeBuffer[0];
		tempStr += "\r\n";
	}

	/*
	check content length or transfer encoding
	*/
	{
		if (_responseBodyType == HTTP_RESPONSE_BODY_FIXED_STR)
		{
			tempStr += "Content-Length: ";
			tempStr += toString(_fixedBodyStr.size());
			tempStr += "\r\n";
		}
		else if (_responseBodyType == HTTP_RESPONSE_BODY_FILE)
		{
			tempStr += "Transfer-Encoding: chunked\r\n";
		}
		else
		{
			throw WebservException("YOOOOOO");
		}
	}

	// get the content type
	{
		// only if has body
		if (_responseBodyType != HTTP_RESPONSE_NOBODY)
		{
			tempStr += "Content-Type: ";
			tempStr += _contentType;
			tempStr += "\r\n";
		}
	}

	// about keep connection
	{
		tempStr += "Connection: ";
		if (_keepAfterResponse == true)
			tempStr += "keep-alive\r\n";
		else
			tempStr += "close\r\n";
	}

	// other optional header
	{
		std::map<std::string, std::set<std::string> >::const_iterator responseHeaderIt = _responseHeader.begin();

		std::set<std::string>::const_iterator responseHeaderFieldIt;

		while (responseHeaderIt != _responseHeader.end())
		{
			tempStr += responseHeaderIt->first;
			tempStr += ": ";

			responseHeaderFieldIt = responseHeaderIt->second.begin();
			while (responseHeaderFieldIt != responseHeaderIt->second.end())
			{
				if (responseHeaderFieldIt != responseHeaderIt->second.begin())
					tempStr += ", ";

				tempStr += (*responseHeaderFieldIt);

				++responseHeaderFieldIt;
			}
			tempStr += "\r\n";
			++responseHeaderIt;
		}
	}

	// end the header part with another "\r\n"
	tempStr += "\r\n";

	// now put all this to BufferList
	{
		_bufferList.push_back(s_response_buff());

		s_response_buff& targetBuff = _bufferList.back();

		targetBuff.buffer.insert(targetBuff.buffer.end(), tempStr.begin(), tempStr.end());
		targetBuff.currentIndex = 0;
	}

	// about the body section
	{
		if (_responseBodyType == HTTP_RESPONSE_BODY_FIXED_STR)
		{
			_bufferList.push_back(s_response_buff());

			s_response_buff& targetBuff = _bufferList.back();


			targetBuff.currentIndex = 0;
			targetBuff.buffer.insert(targetBuff.buffer.end(), _fixedBodyStr.begin(), _fixedBodyStr.end());
		}
	}
	
	_hasSomethingtoSend = true;
}

ssize_t	HttpResponse::sendHttpResponse(const Socket& clientSocket)
{
	if (_bufferList.empty() && _sendBuffer.empty())
		return (0);

	size_t	needToappendSize = HTTP_RECV_BUFFER - _sendBuffer.size();
	// appending buffer

	std::list<s_response_buff>::iterator bufferListIt;
	while (needToappendSize > 0)
	{
		if (_bufferList.empty())
		{
			if (_responseBodyType == HTTP_RESPONSE_BODY_FILE && _isReachEOF == false)
			{
				_bufferList.push_back(s_response_buff());

				s_response_buff& targetResBuff = _bufferList.back();

				targetResBuff.currentIndex = 0;

				std::vector<char> temp(HTTP_SEND_BUFFER);

				// read to buffer
				ssize_t	readAmount = read(_fileFd.getFd(), &temp[0], HTTP_SEND_BUFFER);

				if (readAmount < 0)
				{
					// fatal error
					if (errno != EINTR)
					{
						return (-1);
					}
					continue;
				}
				else if (readAmount == 0)
				{
					// reach EOF
					_isReachEOF = true;
					std::string tempchunkStr = "0\r\n\r\n";
					targetResBuff.buffer.insert(targetResBuff.buffer.end(), tempchunkStr.begin(), tempchunkStr.end());
				}
				else
				{
					std::string startchunkhex = size_t_to_hex(readAmount);

					startchunkhex += "\r\n";

					targetResBuff.buffer.insert(targetResBuff.buffer.end(), startchunkhex.begin(), startchunkhex.end());
					targetResBuff.buffer.insert(targetResBuff.buffer.end(), temp.begin(), temp.begin() + readAmount);

					std::string endlind = "\r\n";

					targetResBuff.buffer.insert(targetResBuff.buffer.end(), endlind.begin(), endlind.end());
				}

			}
			else
				break ;
		}

		bufferListIt = _bufferList.begin();

		if (needToappendSize < bufferListIt->buffer.size() - bufferListIt->currentIndex)
		{
			_sendBuffer.insert(_sendBuffer.end(), bufferListIt->buffer.begin() + bufferListIt->currentIndex, bufferListIt->buffer.begin() + bufferListIt->currentIndex + needToappendSize);

			bufferListIt->currentIndex += needToappendSize;
			needToappendSize = 0;
		}
		else
		{
			_sendBuffer.insert(_sendBuffer.end(), bufferListIt->buffer.begin() + bufferListIt->currentIndex, bufferListIt->buffer.end());
			needToappendSize -= bufferListIt->buffer.size() - bufferListIt->currentIndex;
			_bufferList.pop_front();
		}
	}

	ssize_t	sendAmount = send(clientSocket.getSocketFD().getFd(), &_sendBuffer[_sendBufferOffset], _sendBuffer.size() - _sendBufferOffset, 0);

	if (sendAmount > 0)
	{
		_sendBufferOffset += sendAmount;
		if (_sendBufferOffset >= _sendBuffer.size())
		{
			_sendBuffer.clear();
			_sendBufferOffset = 0;
		}

	}

	// temporary
	if (_sendBuffer.empty() && _bufferList.empty())
	{
		_hasSomethingtoSend = false;
		_isComplete = true;

		if (_responseBodyType == HTTP_RESPONSE_BODY_FILE && _isReachEOF == false)
			_isComplete = false;
	}
	
	return (sendAmount);
}


bool HttpResponse::isComplete() const
{
	return (_isComplete);
}

bool HttpResponse::hasSomethingtoSend() const
{
	return (_hasSomethingtoSend);
}



