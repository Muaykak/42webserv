#include "../../include/classes/Http.hpp"
#include "../../include/classes/Socket.hpp"

Http::Http()
: _keepConnection(true),
_processStatus(NO_STATUS),
_method(NO_METHOD),
_errorStatusCode(-1)
{
	_recvBuffer.reserve(HTTP_RECV_BUFFER);
}

Http::~Http()
{

}

void Http::printHeaderField() const
{
	if (_processStatus != NO_STATUS
	&& _processStatus != READING_HEADER
	&& _processStatus != READING_REQUEST_LINE)
	{
		std::cout << "====================================" << std::endl;
		//print request line
		{
			switch (_method)
			{
			case NO_METHOD:
				std::cout << "NO_METHOD ";
				break;
			case GET:
				std::cout << "GET ";
				break;
			case POST:
				std::cout << "POST ";
				break;
			case DELETE:
				std::cout << "DELETE ";
				break;
			default:
				break;
			}
			std::cout << _requestTarget << " " << _protocol << std::endl;
		}

		std::map<std::string, std::set<std::string> >::const_iterator	it = _headerField.begin();
		std::set<std::string>::const_iterator vecIt;
		while (it != _headerField.end())
		{
			std::cout << it->first << ": ";
			vecIt = it->second.begin();
			while (vecIt != it->second.end())
			{
				if (vecIt != it->second.begin())
					std::cout << ", ";
				std::cout << *vecIt;
				++vecIt;
			}
			std::cout << std::endl;
			++it;
		}
		std::cout << "====================================" << std::endl;
	}
}

bool Http::isKeepConnection() const
{
	return _keepConnection;
}

void Http::httpError(int errorCode, const std::string& throwToClient)
{
	_errorStatusCode = errorCode;
	_throwMessageToClient = throwToClient;
	_requestBuffer.clear();
}
void Http::httpError(int errorCode)
{
	_errorStatusCode = errorCode;
	_requestBuffer.clear();
}

// call to processRequestBuffer

void Http::readingRequestBuffer()
{

}

void	Http::parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize)
{
	if (_process_return != 1)
		return ;
	if (_processStatus == NO_STATUS)
		_processStatus = READING_REQUEST_LINE;
	if (_processStatus == READING_REQUEST_LINE){

		if (reqBuffSize > MAX_REQUEST_BUFFER_SIZE)
		{
			httpError(400, "request buffer should not higher than MAX_REQUEST_BUFFER_SIZE");
		}
		if (_method == NO_METHOD)
		{

			// skip any empty line '\r\n"""
			while (currIndex + 1 < reqBuffSize){
				if (_requestBuffer[currIndex] == '\r') {
					// '\r' must always followed by '\n'
					if (_requestBuffer[currIndex + 1] != '\n') {
						httpError(400);
						_process_return = 0;
						return ;
					}
					currIndex += 2;
					continue;
				}
				else
					break ;
			}
		
			if (currIndex >= reqBuffSize){
				_requestBuffer.clear();
				_process_return = 2;
				return ;
			}
			// now currIndex sits at the first char of the 
			// request line
			
			// I need the whole request line first
			// before process anything
			size_t	endLinePos = _requestBuffer.find("\r\n");

			// doesn't find any endline delimiter
			// will process later
			if (endLinePos == _requestBuffer.npos){
				if (currIndex != 0)
					_requestBuffer = _requestBuffer.substr(currIndex);
				_process_return = 2;
				return ;
			}

			std::string methodStr;

			// find pos any whitespaces but not '\n'
			size_t	pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
			methodStr = pos == _requestBuffer.npos ? _requestBuffer.substr(currIndex) : _requestBuffer.substr(currIndex, pos - currIndex);

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
				httpError(400, "");
				_process_return = 0;
				return ;
			}
			// skip 1 pos which is the first whitespace
			currIndex = pos + 1;
			// should not reach endlinePos yet
			if (currIndex >= endLinePos){
				httpError(400, "");
				_process_return = 0;
				return ;
			}
			// now get the request target.
			pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
			// must can find next whitespace.. it IS the format
			if (pos == _requestBuffer.npos){
				httpError(400, "");
				_process_return = 0;
				return ;
			}
			_requestTarget = _requestBuffer.substr(currIndex, pos - currIndex);

			// must not empty
			if (_requestTarget.empty()){
				httpError(400, "");
				_process_return = 0;
				return ;
			}

			// then move our currIndex
			currIndex = pos + 1;
			// prevent edge case where now currIndex might right at the endLinepos
			// should not reach endlinePos yet
			if (currIndex >= endLinePos){
				httpError(400, "");
				_process_return = 0;
				return ;
			}

			// now the last part is protocol version
			_protocol = _requestBuffer.substr(currIndex, endLinePos - currIndex);

			// check must not empty
			if (_protocol.empty()){
				httpError(400, "");
				_process_return = 0;
				return ;
			}
			// we get all the request line then we move to reading the header field
			currIndex = endLinePos + 2;
		}

		_processStatus = READING_HEADER;

	}

	_process_return = 1;
	return ;
}

void	Http::parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize)
{
	if (_process_return != 1)
		return ;
	if (currIndex >= reqBuffSize)
	{
		_requestBuffer.clear();
		_process_return = 2;
		return ;
	}

	// should fix about \r\n\r\n

	size_t	endLinePos;

	std::string	tempFieldName;
	std::string	tempFieldValue;
	std::string	tempSep;
	std::set<std::string> tempSet;
	size_t	colonPos;
	size_t	temp;

	while (_processStatus == READING_HEADER)
	{
		endLinePos = _requestBuffer.find("\r\n", currIndex);

		// cannot find endline yet, process next read later
		if (endLinePos == _requestBuffer.npos)
		{
			if (currIndex != 0)
				_requestBuffer.erase(0, currIndex);
			_process_return = 2;
			return ;
		}

		// it is \r\n\r\n determine the end of header
		if (endLinePos == currIndex)
		{

			_processStatus = PROCESSING_REQUEST;
			if (currIndex + 2 >= reqBuffSize)
				_requestBuffer.clear();
			else
				_requestBuffer.erase(0, currIndex);
			currIndex = 0;
			printHeaderField();
			break ;
		}


		// we found end line pos

		// must separate name and value by colon ':'
		colonPos = _requestBuffer.find_first_of(':', currIndex);
		// not separate by :  must reject
		if (colonPos == _requestBuffer.npos || colonPos >= endLinePos)
		{
			httpError(400);
			_process_return = 0;
			return ;
		}
		// then get the field name in temp first
		tempFieldName = _requestBuffer.substr(currIndex, colonPos - currIndex);

		// must not empty
		if (tempFieldName.empty())
		{
			httpError(400);
			_process_return = 0;
			return ;
		}

		// We can check field name length here

		// simple checking that it must not contain any forbidden char
		if (allowedFieldNameChar().isNotMatch(tempFieldName))
		{
			httpError(400);
			_process_return = 0;
			return ;
		}


		// now we got field name, next we want field value
		currIndex = colonPos + 1;
		tempSep = _requestBuffer.substr(currIndex, endLinePos - currIndex);
		// Field value allow to be empty
		// so only process if not empty
		if (!tempSep.empty())
		{
			size_t	commaPos;
			size_t	i = 0;
			size_t	j = 0;
			std::string	newStr;

			while (i < tempSep.size())
			{
				// skip whitespace at the front first
				temp = htabSp().findFirstNotCharset(tempSep, i);
				// found only \t' ' so no value
				if (temp == tempSep.npos)
				{
					break ;
				}

				// move to first char
				i = temp;
				commaPos = tempSep.find_first_of(',', i);
				if (commaPos == tempSep.npos)
					commaPos = tempSep.size();
				j = htabSp().findLastNotCharset(tempSep, commaPos - 1, commaPos - 1 - i);

				if (j <= i || j == tempSep.npos)
					newStr = tempSep.substr(i, 1);
				else
					newStr = tempSep.substr(i, j - i + 1);
				//check if field value doesn't contain any forbidden char
				if (forbiddenFieldValueChar().isNotMatch(newStr) == false)
				{
					httpError(400);
					_process_return = 0;
					return ;
				}
				tempSet.insert(newStr);
				newStr.clear();
				
				i = commaPos + 1;
			}
		}
		if (!tempSet.empty())
			_headerField[tempFieldName].insert(tempSet.begin(), tempSet.end());
		tempSet.clear();
		
		// successfully read one field name and value, move to next one
		currIndex = endLinePos + 2;

	}

	_process_return = 1;
	return ;
}


// use the _requestBuffer
void	Http::processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	try
	{
		size_t	currIndex = 0;
		size_t	reqBuffSize = _requestBuffer.size();

		_process_return = 1;

		parsingHttpRequestLine(currIndex, reqBuffSize);
		parsingHttpHeader(currIndex, reqBuffSize);
	}
	catch (std::exception &e)
	{
		Logger::log(LC_ERROR, "something weird, Consider as server error ::%s", e.what());
		httpError(500, e.what());
		_process_return = 0;
		return ;
	}

	return ;
}

void Http::readFromClient(const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	ssize_t	readAmount;

	while (_keepConnection == true)
	{
		readAmount = recv(clientSocket.getSocketFD().getFd(), &_recvBuffer[0], HTTP_RECV_BUFFER, 0);
		if (readAmount == 0)
		{
			Logger::log(LC_CONN_LOG, "Disconnecting client Socket#%d", clientSocket.getSocketFD().getFd());
			_keepConnection = false;
		}
		else if (readAmount < 0)
		{
			if (errno == EINTR)
				continue ;	
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
				break ;
			else
				_keepConnection = false;
		}
		else 
		{
			_requestBuffer.append(_recvBuffer.data(), readAmount);
			processingRequestBuffer(clientSocket, socketMap);
		}
	}
	if (_errorStatusCode != -1)
		_keepConnection = false;
}

void	Http::writeToClient(const Socket& clientSocket, std::map<int, Socket>& SocketMap)
{

}
