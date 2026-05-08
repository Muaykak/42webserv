#include "../../include/classes/HttpResponse.hpp"
#include "../../include/classes/Socket.hpp"

HttpResponse::HttpResponse()
:
_isComplete(false),
_hasSomethingtoSend(false),
_sendBufferOffset(0),
fileSize(0),
responseBodyType(HTTP_RESPONSE_NOBODY),
keepAfterResponse(false),
canModify(true),
isReachEOF(false),
socketMapPtr(NULL),
targetServer(NULL),
targetLocationBlock(NULL)
{
	_sendBuffer.reserve(HTTP_SEND_BUFFER);
}

HttpResponse::~HttpResponse()
{
	if (cgiProcessData.hasData())
	{
		(*cgiProcessData)->clientSocketPtr.clear();
	}
}

void HttpResponse::pushNewResponseBuff(s_response_buff& newBuff)
{
	_bufferList.push_back(newBuff);
	_hasSomethingtoSend = true;
}

void HttpResponse::addHeader(const std::string& headerName, const std::string& headerValue)
{
	std::vector<std::string>& targetVec = responseHeader[headerName];

	if (!headerName.empty() && !headerValue.empty())
	{

		std::vector<std::string>::const_iterator vecIt = targetVec.begin();

		while (vecIt != targetVec.end())
		{
			/* don't modify if the value is the same */
			if (headerValue == *vecIt)
				return ;

			++vecIt;
		}

		targetVec.push_back(headerValue);

	}
}

void	HttpResponse::generateResponse()
{
	if (canModify == false)
		throw WebservException("HttpResponse:: can only generate response 1 time only");

	// tell the browser to not caching
	if (responseHeader.find("Cache-Control") == responseHeader.end())
	{
		this->addHeader("Cache-Control", "no-store");
		this->addHeader("Cache-Control", "no-cache");
		this->addHeader("Cache-Control", "must-revalidate");
	}
	
	canModify = false;

	// some safety checking
	
	if (_bufferList.size() != 0)
		throw WebservException("HttpResponse:: _bufferList size() != 0 something wrong");

	std::string tempStr;

	if (statusLine.hasData())
	{
		tempStr += "HTTP/1.1 " + toString(statusLine->first) + " " + statusLine->second + "\r\n";
	}

	//check for Date:
	if (responseHeader.find("Date") == responseHeader.end())
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
	if (responseHeader.find("Content-Length") == responseHeader.end() && responseHeader.find("Transfer-Encoding") == responseHeader.end())
	{
		if (responseBodyType == HTTP_RESPONSE_BODY_FIXED_STR)
		{
			tempStr += "Content-Length: ";
			tempStr += toString(fixedBodyStr.size());
			tempStr += "\r\n";
		}
		else if (responseBodyType == HTTP_RESPONSE_BODY_FILE)
		{
			tempStr += "Content-Length: ";
			tempStr += toString(fileSize);
			tempStr += "\r\n";
		}
		else
		{
			tempStr += "Transfer-Encoding: chunked\r\n";
		}
	}

	// get the content type
	if (responseHeader.find("Content-Type") == responseHeader.end())
	{
		// only if has body
		if (responseBodyType != HTTP_RESPONSE_NOBODY)
		{
			tempStr += "Content-Type: ";
			tempStr += contentType;
			tempStr += "\r\n";
		}
	}

	// about keep connection
	if (responseHeader.find("Connection") == responseHeader.end())
	{
		tempStr += "Connection: ";
		if (keepAfterResponse == true)
			tempStr += "keep-alive\r\n";
		else
			tempStr += "close\r\n";
	}

	// other optional header
	{
		std::map<std::string, std::vector<std::string> >::const_iterator responseHeaderIt = responseHeader.begin();

		std::vector<std::string>::const_iterator responseHeaderFieldIt;

		while (responseHeaderIt != responseHeader.end())
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
		if (responseBodyType == HTTP_RESPONSE_BODY_FIXED_STR)
		{
			_bufferList.push_back(s_response_buff());

			s_response_buff& targetBuff = _bufferList.back();


			targetBuff.currentIndex = 0;
			targetBuff.buffer.insert(targetBuff.buffer.end(), fixedBodyStr.begin(), fixedBodyStr.end());
		}
	}
	
	_hasSomethingtoSend = true;
}

ssize_t	HttpResponse::sendHttpResponse(const Socket& clientSocket)
{
	if (_bufferList.empty() && _sendBuffer.empty())
		return (0);

	if (_sendBuffer.size() < HTTP_SEND_BUFFER)
	{
		size_t	needToappendSize = HTTP_SEND_BUFFER - _sendBuffer.size();
		// appending buffer

		std::list<s_response_buff>::iterator bufferListIt;
		while (needToappendSize > 0)
		{
			if (_bufferList.empty())
			{
				if (responseBodyType == HTTP_RESPONSE_BODY_FILE && isReachEOF == false)
				{
					_bufferList.push_back(s_response_buff());

					s_response_buff& targetResBuff = _bufferList.back();

					targetResBuff.currentIndex = 0;

					std::vector<char> temp(HTTP_SEND_BUFFER);

					// read to buffer
					ssize_t	readAmount = read(fileFd->getFd(), &temp[0], HTTP_SEND_BUFFER);

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
						isReachEOF = true;
						//std::string tempchunkStr = "0\r\n\r\n";
						//targetResBuff.buffer.insert(targetResBuff.buffer.end(), tempchunkStr.begin(), tempchunkStr.end());
					}
					else
					{
						//std::string startchunkhex = size_t_to_hex(readAmount);

						//startchunkhex += "\r\n";

						//targetResBuff.buffer.insert(targetResBuff.buffer.end(), startchunkhex.begin(), startchunkhex.end());
						targetResBuff.buffer.insert(targetResBuff.buffer.end(), temp.begin(), temp.begin() + readAmount);

						//std::string endlind = "\r\n";

						//targetResBuff.buffer.insert(targetResBuff.buffer.end(), endlind.begin(), endlind.end());
					}

				}
				else
					break ;
				//else if (_responseBodyType == HTTP_RESPONSE_BODY_CGI && _isReachEOF == false)
				//	break ;
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

		if (responseBodyType == HTTP_RESPONSE_BODY_FILE && isReachEOF == false)
			_isComplete = false;

		if (cgiProcessData.hasData() == true && (*cgiProcessData)->status == CGI_PROCESS_RUNNING)
			_isComplete = false;
	}
	
	return (sendAmount);
}

void HttpResponse::forcePrintAllResponse()
{
	// print what is on the list first
	{
		std::list<s_response_buff>::const_iterator respBuffIt = _bufferList.begin();

		while (respBuffIt != _bufferList.end())
		{
			std::cout << std::string(respBuffIt->buffer.begin(), respBuffIt->buffer.end());
			++respBuffIt;
		}
	}

	// if the response is 
	if (responseBodyType == HTTP_RESPONSE_BODY_FILE)
	{
		std::vector<char>	writeBuff;
		writeBuff.reserve(HTTP_SEND_BUFFER);
		ssize_t	amountToRead;
		ssize_t readAmount;
		ssize_t	writeAmount = 0;
		ssize_t	writeRet;

		while (true)
		{
			amountToRead = HTTP_SEND_BUFFER - writeBuff.size();
			readAmount = read(fileFd->getFd(), &writeBuff[writeBuff.size() - 1], amountToRead);
			if (readAmount < 0 && writeBuff.size() == 0)
			{
				if (errno == EINTR)
					continue ;
				else
					break ;
			}
			else if (readAmount == 0 && writeBuff.size() == 0)
			{
				break ;
			}

			writeRet = write(STDOUT_FILENO, &writeBuff[0], writeBuff.size());
			if (writeRet < 0)
			{
				if (errno == EINTR)
					continue ;
				else
					break ;
			}
			writeAmount += writeRet;
			if (writeAmount >= HTTP_SEND_BUFFER)
			{
				writeBuff.clear();
				writeAmount = 0;
			}
		}
	}
}


bool HttpResponse::isComplete() const
{
	return (_isComplete);
}

bool HttpResponse::hasSomethingtoSend() const
{
	return (_hasSomethingtoSend);
}



