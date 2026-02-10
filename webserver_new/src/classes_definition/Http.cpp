#include "../../include/classes/Http.hpp"
#include "../../include/classes/Socket.hpp"
#include "../../include/utility_function.hpp"
#include <cctype>

Http::Http()
: _keepConnection(true),
_processStatus(NO_STATUS),
_errorStatusCode(-1),
_targetServer(NULL)
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
			std::cout << _method << " " << _requestTarget << " " << _protocol << std::endl;
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
	_requestBuffer.clear(); //should clean or not!!?
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
		if (_method.empty())
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

			// find pos any whitespaces but not '\n'
			size_t	pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
			_method = pos == _requestBuffer.npos ? _requestBuffer.substr(currIndex) : _requestBuffer.substr(currIndex, pos - currIndex);

			if (_method.empty() || alphaAtoZ().isNotMatch(_method) == true)
			{
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

			// also check if contain any forbidden chars
			if (allowRequestTargetChar().isMatch(_requestTarget) == false)
			{
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

			_processStatus = VALIDATING_REQUEST;
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
		if (allowedFieldNameChar().isMatch(tempFieldName) == false)
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

		// normalize field name because it is case insensitive
		stringToLower(tempFieldName);

		if (!tempSet.empty())
			_headerField[tempFieldName].insert(tempSet.begin(), tempSet.end());
		tempSet.clear();
		
		// successfully read one field name and value, move to next one
		currIndex = endLinePos + 2;

	}

	_process_return = 1;
	return ;
}

// decoding the '%' in URI path
static bool	pathDecoding(std::string& pathStr)
{
	size_t	curPos = 0;
	size_t	symPos = 0;
	std::string	returnStr;
	std::string hexStr = "0123456789abcdef";
	unsigned char temp;

	// should not empty string here
	if (pathStr.empty())
		return (false);
	while (curPos < pathStr.size())
	{
		symPos = pathStr.find_first_of('%', curPos);
		if (symPos == pathStr.npos)
		{
			returnStr += pathStr.substr(curPos);
			break ;
		}
		else 
		{
			// add normal chars to returnStr
			if (symPos != curPos)
				returnStr += pathStr.substr(curPos, symPos - curPos);
			// because '%' must followed by 2 char
			if (symPos + 2 >= pathStr.size() || hexChar().isMatch(pathStr, symPos + 1, 2) == false)
				return (false);
			temp = static_cast<unsigned char>(hexStr.find_first_of(std::tolower(pathStr[symPos + 1])) << 4);
			temp |= static_cast<unsigned char>(hexStr.find_first_of(std::tolower(pathStr[symPos + 2])));

			// will not accept if temp is control chars, '/', or DEL (127)
			if (temp < 32 || temp == '/' || temp == 127)
				return (false);
			returnStr += temp;
			curPos = symPos + 3;
		}
		pathStr = returnStr;
	}
	return true;
}

void	Http::validateRequestBufferSelectServer(const Socket& clientSocket,const std::string& authStr)
{
	//separate port and host from authStr
	int			portNum;
	std::string	host;
	{
		std::string portStr;
		size_t	colonPos = authStr.find_first_of(':', 0);
		if (colonPos == authStr.npos)
		{
			// we didn't allow https so assume the default is always 80
			host = authStr;
			portStr = "80";
		}
		else 
		{
			host = authStr.substr(0, colonPos);
			if (host.empty())
			{
				httpError(400, "Invalid::URI Authority::");
				_process_return = 0;
				return ;
			}

			portStr = authStr.substr(colonPos + 1);
			if (portStr.empty() || portStr.size() > 5 || digitChar().isMatch(portStr) == false)
			{
				httpError(400, "Invalid::URI Authority::");
				_process_return = 0;
				return ;
			}
		}
		portNum = std::atoi(portStr.c_str());
	}

	// check port
	{
		// if the port is out of range we can define as
		// 400 bad request
		if (portNum > 65535)
		{
			httpError(400, "Invalid::URI Authority::Port is out of range");
			_process_return = 0;
			return ;
		}

		// if the port is in the range but doesn't match with
		// the connection it was coming through
		// we can give 403 Forbidden because we are not
		// Proxy server
		if (portNum != clientSocket.getServerListenPort())
		{
			std::string msg = "Invalid::URI Authority::Port mismatch request_target:" + toString(portNum) + " server_port:" + toString(clientSocket.getServerListenPort());
			
			httpError(403, msg);
			_process_return = 0;
			return ;
		}
	}

	// now we check the host, if it is ip or server_name (or whatever it's called)
	{

		// it is probably something like 172.233.21.34 some thing like that
		if (hostipChar().isMatch(host) == true)
		{
			in_addr_t	tempAddr;
			// conversion failed
			if (string_to_in_addr_t(host, tempAddr) == false)
			{
				httpError(400, "Invalid::URI Authority::failed to convert IP");
				_process_return = 0;
				return ;
			}

			// correct syntax but should not allow, it is broadcast address
			if (tempAddr == 0xFFFFFFFF)
			{
				httpError(403, "Invalid::URI Authority:: IP cannot be 255.255.255.255");
				_process_return = 0;
				return ;
			}

			/*
			Remember the accept() function that we
			use to create new client socket ?

			client_socket = accept(event.data.fd, (sockaddr *)&client_address, &len);

			we have client_address struct to look at the destination IP
			that client connect to server (not the client's IP)

			so what happens when the IP in request target does not match
			with the socket IP actually is?

			in general according to HTTP RFC9112
			there are 2 types of server
			1. the proxy server
			2. origin server

			for proxy server
			if the mismatch means the the client want us to connect to
			another server using this request target (act as a middle man)

			for origin server
			this is what Webserv project lives. we are the origin server.
			We are suppose not to act as middle man.
			If the mismatch happens the RFC9112 only specify that
			we should not treat this as bad request (because the syntax is correct).
			but it allows freedom how we handle this case

			-> for Nginx server, it handles by default fallback server.
			If the mismatch happens, nginx will choose the default server. (usually the one on the top)

			-> for us we can response with 403 Forbidden. this way we can avoid trouble the most.
			*/
			if (tempAddr != clientSocket._client_addr_in)
			{
				std::string msg = "Invalid::URI Authority::IP mismatch:: server_ip:" + in_addr_t_to_string(clientSocket._client_addr_in) + " request_target:" + in_addr_t_to_string(tempAddr);
				httpError(403, msg);
				_process_return = 0;
				return ;
			}
		}
		else
		{
			/*

				this part mean that is host of authority part
				is not the form of 123.123.123.123

				it has some name which we would match with

				because we kind of implement virtual hosting

				this part we have fallback server
			*/

			// but check first if contain any invalid host field of http

			const ServerConfig* fallback_server = NULL;
			const std::vector<ServerConfig>* ptr = clientSocket.getServersConfigPtr();
			std::vector<ServerConfig>::const_iterator serverVecIt = ptr->begin();
			std::vector<std::string>::const_iterator tempNameVecIt;
			_targetServer = NULL;
			while (serverVecIt != ptr->end() && _targetServer == NULL)
			{
				if (serverVecIt->getServerNameVec().size() == 0)
				{
					if (fallback_server == NULL)
						fallback_server = &(*serverVecIt);
				}
				else
				{
					tempNameVecIt = serverVecIt->getServerNameVec().begin();
					while (tempNameVecIt != serverVecIt->getServerNameVec().end())
					{
						// if matches, then pick this server and break
						if (*tempNameVecIt == host)
						{
							_targetServer = &(*serverVecIt);
							break ;
						}
						++tempNameVecIt;
					}
				}
				++serverVecIt;
			}
			if (_targetServer == NULL)
			{
				if (fallback_server)
					_targetServer = fallback_server;
				else
					_targetServer = &((*clientSocket.getServersConfigPtr())[0]);
			}
		}
	}


}

void	Http::validateRequestBufffer(const Socket& clientSocket)
{
	if (_process_return != 1 || _processStatus != VALIDATING_REQUEST)
		return ;

	// checking the request line
	{
		{
			// rough check for method first
			if (_method != "GET" && _method != "POST" && _method != "DELETE")
			{
				httpError(405, "Method not allowed");
				_process_return = 0;
				return ;
			}
		}

		// before anything, we check the protocol version first
		{
			
			char maj;
			char min;
			// the string should be at least 8
			// HTTP/X.X
			// whether what version you are
			// must start with "HTTP/"
			if (_protocol.size() != 8 || _protocol.compare(0, 5, "HTTP/") != 0 || _protocol[6] != '.')
			{
				httpError(400, "ERROR::protocol version wrong format");
				_process_return = 0;
				return ;
			}

			maj = _protocol[5];
			if (maj != '1')
			{
				if (maj >= '0' && maj <= '3')
				{
					// response with unsupported version
					httpError(505, "Error::version not supported");
				}
				else 
				{
					// some weird characters
					httpError(400, "ERROR::protocol version wrong format");
				}
				_process_return = 0;
				return ;
			}

			min = _protocol[7];
			if (min < '0' || min > '9')
			{
				// must be digit
				httpError(400, "ERROR::protocal version wrong format");
				_process_return = 0;
				return ;
			}

			_protocol.clear();
			_protocol += maj;
			_protocol += min;
			// finished processing protocol

		}

		// whether what we check, if host is missing from
		// header field, the server should not accept it.
		if (_headerField.find("host") == _headerField.end())
		{
			// treated as bad request
			httpError(400, "Bad request::cannot find \'host\' in header field");
			_process_return = 0;
			return ;
		}
		// we need to check the 'request target' first
		// this is the tedious process
		{
			// check the first character to see if it is
			// origin form or absolute form
			{
				// if first character is not '/'
				// then it might be absolute form
				// we need to check that scheme
				if (_requestTarget[0] != '/')
				{
					//this request targen legth if in absolute form must
					// longer than  characters
					if (_requestTarget.size() <= 7)
					{
						httpError(400, "Invalid::URI Scheme::");
						_process_return = 0;
						return ;
					}


					// allow only this scheme
					if (_requestTarget.compare(0, 7, "http://") != 0)
					{
						httpError(400, "Invalid::URI Scheme::allowed only \"http://\"");
						_process_return = 0;
						return ;
					}

					// check the authority part
					{
						std::string authStr;

						size_t	pathPos = _requestTarget.find_first_of('/', 7);
						// if cannot find then it is only /
						if (pathPos == _requestTarget.npos)
							authStr = _requestTarget.substr(7);
						else
							authStr = _requestTarget.substr(7, pathPos - 7);

						// authority cannot empty
						if (authStr.empty())
						{
							httpError(400, "Invalid::URI Scheme::");
							_process_return = 0;
							return ;
						}

						validateRequestBufferSelectServer(clientSocket, authStr);
						if (_process_return == 0)
							return ;

						// now for authority part we check both host and ip
						// so now we can recreate _requestTarget string
						if (pathPos == _requestTarget.npos)
							_requestTarget = "/";
						else
							_requestTarget = _requestTarget.substr(pathPos);
					}

				}
				else
				{
				/*
					In case of target that starts with '/' 
					meaning that it is URI origin form
					in this case we need to check the host:
					in header
				*/
					std::set<std::string>& fieldValueSet = _headerField.find("host")->second;

					// must have only 1 value in host:
					if (fieldValueSet.size() != 1)
					{
						httpError(400, "Http::Invalid field value:: host must have only 1 element");
						_process_return = 0;
						return ;
					}

					std::set<std::string>::const_iterator	fieldValueSetIt = fieldValueSet.begin();

					validateRequestBufferSelectServer(clientSocket, *fieldValueSetIt);
					if (_process_return == 0)
						return ;
				}

			}

			// separate query and path
			{
				size_t	pos = _requestTarget.find_first_of('?');
				if (pos == _requestTarget.npos)
					_targetPath = _requestTarget;
				else
				{
					_targetPath = _requestTarget.substr(0, pos);
					if (pos < _requestTarget.size() - 1)
						_queryString = _requestTarget.substr(pos + 1);
				}
			}

			// this is to processing the path.
			{

				// according to the standard, we must
				// separate path into segments separated by
				// the '/' as the delimitter first

				std::list<std::string> splitList;
				splitString(_targetPath, "/", splitList);

				//split vector should not empty
				if (splitList.size() == 0)
				{
					httpError(400, "Bad Path. Or my bad parser lol");
					_process_return = 0;
					return ;
				}

				std::list<std::string>::iterator	splitListIt = splitList.begin();

				while (splitListIt != splitList.end())
				{
					if (*splitListIt == ".")
						splitList.erase(splitListIt++);
					else if (*splitListIt == "..")
					{
						if (splitListIt != splitList.begin())
						{
							--splitListIt;
							splitList.erase(splitListIt++);
						}
						splitList.erase(splitListIt++);
					}
					else
					{
						/*
						this part means that this segment 
						is filename 
						and what we should do is decode the
						'%' first
						*/

						/*
						then we check after decoding again if this segment
						string is "." or "..". i will assume this as bad intent
						because why you don't use  '.' in the first place	
						*/
						if (pathDecoding(*splitListIt) == false || *splitListIt == "." || *splitListIt == "..")
						{
							// failed to decode 
							// wrong use of '%'
							httpError(400, "request target::'%' encoding invalid::");
							_process_return = 0;
							return ;
						}

						// we proved this string is valid path name
						// continue to next one
						++splitListIt;
					}
				}

				//finished processing the list
				// now we combined that list back to string
				_targetPath.clear();
				splitListIt = splitList.begin();
				size_t	newSize = 0;
				while (splitListIt != splitList.end())
				{
					newSize += splitListIt->size() + 1;
					++splitListIt;
				}
				_targetPath.reserve(newSize);
				splitListIt = splitList.begin();
				while (splitListIt != splitList.end())
				{
					_targetPath += '/';
					_targetPath += *splitListIt;
					++splitListIt;
				}
			}

			// path process done, now 

		}

		// now we check the method
		{
			// we should have target server now
			if (_targetServer == NULL)
			{
				httpError(500, "Internal Error:: _targetServer Not Found");
				_process_return = 0;
				return ;
			}

			const std::vector<std::string>* allowMethodVec = _targetServer->getLocationData(_targetPath, "allowed_methods");
			// must found, if not then config file is wrong, or something is wrong
			if (allowMethodVec == NULL)
			{
				httpError(500, "Internal Error:: cannot find allowed_methods");
				_process_return = 0;
				return ;
			}

			bool match = false;
			
			for (size_t i = 0; i < allowMethodVec->size(); i++)
			{
				if (_method == (*allowMethodVec)[i])
				{
					match = true;
					break ;
				}
			}
			// Method not alowed
			if (match == false)
			{
				httpError(405, "Method not allowed");
				_process_return = 0;
				return ;
			}

		}
	}

	//
	sdf

}


// use the _requestBuffer
void	Http::processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	try
	{
		size_t	currIndex = 0;
		size_t	reqBuffSize = _requestBuffer.size();

		_process_return = 1;

		// put into structure
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
