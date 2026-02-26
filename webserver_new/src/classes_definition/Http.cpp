#include "../../include/classes/Http.hpp"
#include "../../include/classes/Socket.hpp"
#include "../../include/utility_function.hpp"
#include <cctype>
#include <sys/stat.h>

Http::Http()
: _keepConnection(true),
_processStatus(NO_STATUS),
_targetServer(NULL),
_readBody(false),
_isEpollout(false),
_body_type(0),
_checkExpectBody(false),
_body_size(0),
_curr_body_read(0),
_targetLocationBlock(NULL)
{
	_recvBuffer.reserve(HTTP_RECV_BUFFER);
	_sendBuffer.reserve(HTTP_SEND_BUFFER);
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

void	Http::parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize)
{
	if (_processStatus == NO_STATUS)
		_processStatus = READING_REQUEST_LINE;
	if (_processStatus == READING_REQUEST_LINE){

		if (reqBuffSize > MAX_REQUEST_BUFFER_SIZE)
			generate4xx5xxErrorReponse(400, false,"request buffer should not higher than MAX_REQUEST_BUFFER_SIZE");

		if (_method.empty())
		{

			// skip any empty line '\r\n"""
			while (currIndex + 1 < reqBuffSize){
				if (_requestBuffer[currIndex] == '\r') {
					// '\r' must always followed by '\n'
					if (_requestBuffer[currIndex + 1] != '\n')
						generate4xx5xxErrorReponse(400, false, "the \'\\r\' must always followed by \'\\n\'");
					currIndex += 2;
					continue;
				}
				else
					break ;
			}
		
			if (currIndex >= reqBuffSize){
				_requestBuffer.clear();
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
				return ;
			}

			// find pos any whitespaces but not '\n'
			size_t	pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
			_method = pos == _requestBuffer.npos ? _requestBuffer.substr(currIndex) : _requestBuffer.substr(currIndex, pos - currIndex);

			if (_method.empty() || alphaAtoZ().isNotMatch(_method) == true)
				generate4xx5xxErrorReponse(400, false, "the method must not empty. Or method must contain only A-Z in uppercase");

			// skip 1 pos which is the first whitespace
			currIndex = pos + 1;
			// should not reach endlinePos yet
			if (currIndex >= endLinePos)
				generate4xx5xxErrorReponse(400, false, "bad spacing in request line");
			// now get the request target.
			pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
			// must can find next whitespace.. it IS the format
			if (pos == _requestBuffer.npos)
				generate4xx5xxErrorReponse(400, false, "bad formating in request line");
			_requestTarget = _requestBuffer.substr(currIndex, pos - currIndex);

			// must not empty
			if (_requestTarget.empty())
				generate4xx5xxErrorReponse(400, false, "request target in request line must not empty");

			// also check if contain any forbidden chars
			if (allowRequestTargetChar().isMatch(_requestTarget) == false)
				generate4xx5xxErrorReponse(400, false, "request target must not contain any forbidden char");

			// then move our currIndex
			currIndex = pos + 1;
			// prevent edge case where now currIndex might right at the endLinepos
			// should not reach endlinePos yet
			if (currIndex >= endLinePos)
				generate4xx5xxErrorReponse(400, false, "bad request line. the protocol version is missing");

			// now the last part is protocol version
			_protocol = _requestBuffer.substr(currIndex, endLinePos - currIndex);

			// check must not empty
			if (_protocol.empty())
				generate4xx5xxErrorReponse(400, false, "request line protocol must not empty");
			// we get all the request line then we move to reading the header field
			currIndex = endLinePos + 2;
		}

		_processStatus = READING_HEADER;

	}
	return ;
}

void	Http::parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize)
{
	if (currIndex >= reqBuffSize)
	{
		_requestBuffer.clear();
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
			//printHeaderField();
			break ;
		}


		// we found end line pos

		// must separate name and value by colon ':'
		colonPos = _requestBuffer.find_first_of(':', currIndex);
		// not separate by :  must reject
		if (colonPos == _requestBuffer.npos || colonPos >= endLinePos)
			generate4xx5xxErrorReponse(400, false, "name and value in the header field must seperated by \':\'");
		// then get the field name in temp first
		tempFieldName = _requestBuffer.substr(currIndex, colonPos - currIndex);

		// must not empty
		if (tempFieldName.empty())
			generate4xx5xxErrorReponse(400, false, "name in header field must not empty string");

		// We can check field name length here

		// simple checking that it must not contain any forbidden char
		if (allowedFieldNameChar().isMatch(tempFieldName) == false)
			generate4xx5xxErrorReponse(400, false, "name in header field must not contain any forbidden char");

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
					break ;

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
					generate4xx5xxErrorReponse(400, false, "value in header field must not contain any forbidden char");
				tempSet.insert(newStr);
				newStr.clear();
				
				i = commaPos + 1;
			}
		}

		// normalize field name because it is case insensitive
		tempFieldName = stringToLower(tempFieldName);

		if (!tempSet.empty())
			_headerField[tempFieldName].insert(tempSet.begin(), tempSet.end());
		tempSet.clear();
		
		// successfully read one field name and value, move to next one
		currIndex = endLinePos + 2;

	}

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

void	Http::generate3xxRedirectResponse(unsigned int statusCode)
{


	_httpResponseList.push_back(HttpResponse());

	HttpResponse& targetResponse = _httpResponseList.back();

	targetResponse.setResponseBodyType(HTTP_RESPONSE_BODY_FIXED_STR);

	// here normally you would
	// keep connection with 3xx response
	// but if it has body we need to read and discard
	// that body first
	targetResponse.setKeepAfterResponse(true);
	targetResponse.setStatusCode(statusCode);
	targetResponse.setContentType("text/html");

	// status message
	switch (statusCode)
	{
		case 301:
		{
			targetResponse.setStatusMessage("Moved Permanently");
			break;
		}
		case 302:
		{
			targetResponse.setStatusMessage("Found");
			break;
		}
		case 303:
		{
			targetResponse.setStatusMessage("See Other");
			break;
		}
		case 307:
		{
			targetResponse.setStatusMessage("Temporary Redirect");
			break;
		}
		case 308:
		{
			targetResponse.setStatusMessage("Permanent Redirect");
			break;
		}
		default:
		{
			break;
		}
	}

	std::string absoluteRedirectPath = "http://";
	{
		absoluteRedirectPath += _authorityPart;
		absoluteRedirectPath += _targetPath;

		targetResponse.addHeader("Location", absoluteRedirectPath);
	}


	std::string tempHtml =
	"<html>\r\n"
	"<head><title>";

	tempHtml += targetResponse.getStatusMessage();

	tempHtml +=
	"</title></head>\r\n"
	"<body><h1>";

	tempHtml += targetResponse.getStatusMessage();

	tempHtml +=
	"</h1>\r\n<p>Redirect <a href=\"";

	tempHtml += absoluteRedirectPath;

	tempHtml +=
	"\">here</a>.</p>\r\n</body>\r\n</html>";


	targetResponse.setFixedBodyStr(tempHtml);
	//throw HttpThrowStatus(returnCode, (*return_redirect)[1]);

}

void	Http::generate4xx5xxErrorReponse(unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg)
{
	// first we check if there is already default error page
	bool	hasDefaultErrorPageFile = false;

	const std::vector<std::string>* foundErrorPageVec = NULL;
	if (_targetServer != NULL)
	{
		if (_targetLocationBlock != NULL)
		{
			foundErrorPageVec = _targetServer->getLocationData(_targetLocationBlock, "error_page");
		}
		else
		{
			// found targetServer but not _target location
			foundErrorPageVec = _targetServer->getServerData("error_page");
		}
	}

	std::string errorPageFilePath;

	size_t	convertCode;

	if (foundErrorPageVec != NULL)
	{
		size_t i = 0;
		while (i < foundErrorPageVec->size())
		{
			if (string_to_size_t((*foundErrorPageVec)[i], convertCode) == false)
			{
				i += 2;
				continue;
			}

			if (convertCode == errorStatusCode)
			{
				if (++i < foundErrorPageVec->size())
				{
					errorPageFilePath = (*foundErrorPageVec)[i];
					break ;
				}
				else
					break ;
			}
			else
			{
				i += 2;
			}
		}
	}

	// testing if the file is open and readable
	FileDescriptor errorFileFD;

	if (!errorPageFilePath.empty())
	{
		int fd = open(errorPageFilePath.c_str(), O_RDONLY);
		if (fd > -1)
		{
			errorFileFD = fd;
			hasDefaultErrorPageFile = true;
		}
	}

	// try to 
	_httpResponseList.push_back(HttpResponse());

	HttpResponse& targetResponse = _httpResponseList.back();

	targetResponse.setKeepAfterResponse(keepAfterResponse);
	targetResponse.setStatusCode(errorStatusCode);
	targetResponse.setContentType("text/html");

	// set status message
	{
		switch (errorStatusCode)
		{
			case (400):
			{
				targetResponse.setStatusMessage("Bad Request");
				break;
			}
			case (404):
			{
				targetResponse.setStatusMessage("Not Found");
				break;
			}
			case (500):
			{
				targetResponse.setStatusMessage("Internal Error");
				break;
			}
			case (403):
			{
				targetResponse.setStatusMessage("Forbidden");
				break;
			}
			case (405):
			{
				// the allowed method to the header
				{
					const std::vector<std::string>* foundAllowedMethodPtr = _targetServer->getLocationData(_targetLocationBlock, "allowed_methods");

					for (size_t i = 0; i < foundAllowedMethodPtr->size(); i++)
					{
						targetResponse.addHeader("Allow", (*foundAllowedMethodPtr)[i]);
					}
				}

				targetResponse.setStatusMessage("Method Not Allowed");
				break;
			}
			case (417):
			{
				targetResponse.setStatusMessage("Expection Failed");
				break;
			}
			default:
			{
				targetResponse.setStatusMessage("");
				break;
			}

		}


	}

	if (hasDefaultErrorPageFile == true)
	{
		targetResponse.setResponseBodyType(HTTP_RESPONSE_BODY_FILE);
		targetResponse.setFileFd(errorFileFD);
		
	}
	else
	{
		targetResponse.setResponseBodyType(HTTP_RESPONSE_BODY_FIXED_STR);

		std::string htmlMsg = toString(errorStatusCode) + " " + targetResponse.getStatusMessage();

		std::string bodyStr =
		"<html>\r\n"
		"<head><title>";

		bodyStr += htmlMsg;

		bodyStr +=
		"</title></head>\r\n"
		"<body>\r\n"
		"<center><h1>";

		bodyStr += htmlMsg;

		bodyStr += "</h1>\r\n";

		bodyStr += throwMsg;
		
		"</center>\r\n"
		"</body>\r\n"
		"</html>\r\n";

		targetResponse.setFixedBodyStr(bodyStr);
	}

	targetResponse.generateResponse();

	clearRequestData();

	throw HttpThrowStatus(errorStatusCode, throwMsg);
}

void Http::clearRequestData()
{
	// clear the storing request data
	_processStatus = NO_STATUS;
	_method.clear();
	_requestTarget.clear();
	_targetPath.clear();
	_combinedPath.clear();
	_queryString.clear();
	_protocol.clear();
	_cgiPath.clear();
	_uploadStorePath.clear();
	_authorityPart.clear();
	_checkExpectBody = false;
	_readBody = false;
	_body_type = 0;
	_body_size = 0;
	_curr_body_read = 0;
	_headerField.clear();
	_targetServer = NULL;
	_targetLocationBlock = NULL;

}

void	Http::validateRequestBufferSelectServer(const Socket& clientSocket,const std::string& authStr)
{
	if (_processStatus != VALIDATING_REQUEST)
		return ;
	printHeaderField();
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
				generate4xx5xxErrorReponse(400, false, "Invalid::URI Authority::");

			portStr = authStr.substr(colonPos + 1);
			if (portStr.empty() || portStr.size() > 5 || digitChar().isMatch(portStr) == false)
				generate4xx5xxErrorReponse(400, false, "Invalid::URI Authority::");
		}
		portNum = std::atoi(portStr.c_str());
	}

	// check port
	{
		// if the port is out of range we can define as
		// 400 bad request
		if (portNum > 65535)
		{
			generate4xx5xxErrorReponse(400, false, "Invalid::URI Authority::Port is out of range");
		}

		// if the port is in the range but doesn't match with
		// the connection it was coming through
		// we can give 403 Forbidden because we are not
		// Proxy server
		if (portNum != clientSocket.getServerListenPort())
		{
			std::string msg = "Invalid::URI Authority::Port mismatch request_target:" + toString(portNum) + " server_port:" + toString(clientSocket.getServerListenPort());
			generate4xx5xxErrorReponse(403, false, msg);
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
				generate4xx5xxErrorReponse(400, false, "Invalid::URI Authority::failed to convert IP");

			// correct syntax but should not allow, it is broadcast address
			if (tempAddr == 0xFFFFFFFF)
				generate4xx5xxErrorReponse(403, false, "Invalid::URI Authority:: IP cannot be 255.255.255.255");

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
				generate4xx5xxErrorReponse(403, false, msg);
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

	_authorityPart = authStr;
}

void	Http::validateRequestBufffer(const Socket& clientSocket)
{
	if (_processStatus != VALIDATING_REQUEST)
		return ;

	// checking the request line
	{
		{
			// rough check for method first
			if (_method != "GET" && _method != "POST" && _method != "DELETE")
				generate4xx5xxErrorReponse(501, false, "Http::Method not implemented: " + _method);
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
				generate4xx5xxErrorReponse(400, false, "ERROR::protocol version wrong format");

			maj = _protocol[5];
			if (maj != '1')
			{
				if (maj >= '0' && maj <= '3')
				{
					// response with unsupported version
					generate4xx5xxErrorReponse(505, false, "Error::version not supported");
				}
				else 
				{
					// some weird characters
					generate4xx5xxErrorReponse(400, false, "ERROR::protocol version wrong format");
				}
			}

			min = _protocol[7];
			if (min < '0' || min > '9')
			{
				// must be digit
				generate4xx5xxErrorReponse(400, false, "ERROR::protocal version wrong format");
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
			generate4xx5xxErrorReponse(400, false, "Bad request::cannot find \'host\' in header field");
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
						generate4xx5xxErrorReponse(400, false, "Invalid::URI Scheme::");


					// allow only this scheme
					if (_requestTarget.compare(0, 7, "http://") != 0)
						generate4xx5xxErrorReponse(400, false, "Invalid::URI Scheme::allowed only \"http://\"");

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
							generate4xx5xxErrorReponse(400, false, "Invalid::URI Scheme::");

						validateRequestBufferSelectServer(clientSocket, authStr);

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
						generate4xx5xxErrorReponse(400, false, "Http::Invalid field value:: host must have only 1 element");

					std::set<std::string>::const_iterator	fieldValueSetIt = fieldValueSet.begin();

					validateRequestBufferSelectServer(clientSocket, *fieldValueSetIt);
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

				////split vector should not empty
				//if (splitList.size() == 0)
				//	generate4xx5xxErrorReponse(400, false, "Bad Path. Or my bad parser lol");

				bool isEndwithSlash = _targetPath[_targetPath.size() - 1] == '/' ? true : false;

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
						if (pathDecoding(*splitListIt) == false || *splitListIt == "..")
						{
							// failed to decode 
							// wrong use of '%'
							generate4xx5xxErrorReponse(400, false, "request target::'%' encoding invalid::");
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
				if (isEndwithSlash == true)
					_targetPath.reserve(newSize + 1);
				else
					_targetPath.reserve(newSize);
				splitListIt = splitList.begin();
				while (splitListIt != splitList.end())
				{
					_targetPath += '/';
					_targetPath += *splitListIt;
					++splitListIt;
				}
				if (isEndwithSlash == true)
					_targetPath.append("/");
			}

			// path process done, now 

		}

	}

	// get the targeted location block
	{
		// we should have target server now
		if (_targetServer == NULL)
			generate4xx5xxErrorReponse(500, false, "Internal Error:: _targetServer Not Found");

		_targetLocationBlock = _targetServer->findLocationBlock(_targetPath);
		// must not be null also
		if (_targetLocationBlock == NULL)
			generate4xx5xxErrorReponse(500, false, "Internal Error:: _targetServer Not Found");

	}


	/*
	check if found Transfer-Encoding or Content-Length
		if both are found , simply return error,
		we treat this as an error, however, another
		way to implement this is if Both are found
		'Transfer-Encoding' takes priority
	*/
	{

		// check the client_max_body_size of the server block if exist first
		const std::vector<std::string>* clientMaxBodySizePtr = _targetServer->getLocationData(_targetLocationBlock, "client_max_body_size");
		if (clientMaxBodySizePtr == NULL || clientMaxBodySizePtr->size() != 1)
		{
			// treat as internal error
			generate4xx5xxErrorReponse(500, false, "Internal Error::cannot find client_max_body_size, Or it has no matching element");
		}

		// 
		std::map<std::string, std::set<std::string> >::const_iterator content_length = _headerField.find("content-length");
		std::map<std::string, std::set<std::string> >::const_iterator tranfer_encoding = _headerField.find("transfer-encoding");

		// I will not allow DELETE or GET method to have body
		if (_method != "POST" && (tranfer_encoding != _headerField.end() || content_length != _headerField.end()))
		{
			generate4xx5xxErrorReponse(400, false, "Bad request:: DELETE or GET method must not contain body");
		}

		if (tranfer_encoding != _headerField.end() && content_length != _headerField.end())
			generate4xx5xxErrorReponse(400, false, "Bad request:: Transfer-Encoding and Content-Length cannot both exist in header");

		// check Expect the body
		{
			std::map<std::string, std::set<std::string> >::const_iterator foundExpect = _headerField.find("expect");
			
			// if found
			if (foundExpect != _headerField.end())
			{
				const std::set<std::string>& targetValue = foundExpect->second;

				if (targetValue.size() != 1)
				{
					generate4xx5xxErrorReponse(417, false, "Http:: Expect: wrong value :: expects only \"100-continue\"");
				}

				const std::string& targetExpectValue = *(targetValue.begin());

				if (targetExpectValue != "100-continue")
				{
					generate4xx5xxErrorReponse(417, false, "Http:: Expect: wrong value :: expects only \"100-continue\"");
				}

				_checkExpectBody = true;
			}
		}

		// if Content-Length is found, check if it exceeds the client_max_body_size
		if (content_length != _headerField.end())
		{
			// this field must have only 1 element
			if (content_length->second.size() != 1)
			{
				// treat this as a bad request
				generate4xx5xxErrorReponse(400, false, "Bad request:: Content-Length:: must have only one element in this field");
			}

			std::string numstr = *(content_length->second.begin());

			// must contain only digit
			if (digitChar().isMatch(numstr) == false)
				generate4xx5xxErrorReponse(400, false, "Bad request:: Content-Length:: value must be numeric characters (0-9)");


			// conversion to number with myfunction
			if (string_to_size_t(numstr, _body_size) == false)
				generate4xx5xxErrorReponse(400, false, "Bad request:: Content-Length::exceeds size_t value or something");

			// type 1 is body size that specified by Content-Length
			_body_type = 1;

			// now check the body size if it exceeds the client_max_body_size
			{
				const std::vector<std::string>* client_max_body_size_ptr = _targetServer->getLocationData(_targetLocationBlock, "client_max_body_size");

				// internal error
				if (client_max_body_size_ptr == NULL || client_max_body_size_ptr->size() != 1 || digitChar().isMatch((*client_max_body_size_ptr)[0]) == false)
					generate4xx5xxErrorReponse(500, false, "Internal Error::cannot find client_max_body_size value, or the value is not correct");

				size_t	maxBodySize;
				if (string_to_size_t((*client_max_body_size_ptr)[0], maxBodySize) == false)
				{
					// cannot torelate because no boundary to check client request
					generate4xx5xxErrorReponse(500, false, "Internal Error:: client_max_body_size value conversion failed");
				}
				
				if (_body_size > maxBodySize)
					generate4xx5xxErrorReponse(413, false, "HTTP::request Content-Length is Larger than client_max_body_size");

				// check the 
				
				_readBody = true;
			}
		}
		else if (tranfer_encoding != _headerField.end())
		{
			// here need to check the Transfer Encoding
			const std::set<std::string>& valueSet = tranfer_encoding->second;

			// mean that chunked is not found in the list
			if (valueSet.find("chunked") == valueSet.end())
			{
				std::string msg = "Http::request Transfer-Encoding:: \"chunked\" not found::key=";
				
				std::set<std::string>::const_iterator it = valueSet.begin();
				while (it != valueSet.end())
				{
					if (it != valueSet.begin())
						msg += ", ";
					msg += *it;
					++it;
				}
				
				generate4xx5xxErrorReponse(400, false, msg);
			}

			// chunked found then this is chunked encoding.
			_body_type = 2;
			_body_size = 0;
			_readBody = true;
		}
		// both Content-Length and Transfer-Encoding not found
		else
		{
			_body_type = 0;
			_body_size = 0;
		}
	}

	// check for redirection
	{
		// check for 'return' in location block
		const std::vector<std::string>* return_redirect = _targetServer->getLocationData(_targetLocationBlock, "return");
		// if found then we need to redirect
		if (return_redirect != NULL)
		{
			// it must have 2 elements or treat as internal error
			if (return_redirect->size() != 2)
				generate4xx5xxErrorReponse(500, false, "Internal Error:: 'return' redirect in config file must have 2 elements");
			
			const std::string& returnCodeStr = (*return_redirect)[0];

			size_t	returnCode = 0;
			// the string size must be 3 digit and dont start with '0'
			if (returnCodeStr.size() != 3 || string_to_size_t(returnCodeStr, returnCode) == false || returnCode < 300 || returnCode > 399)
				generate4xx5xxErrorReponse(500, false, "Internal Error::config file::redirect contains wrong status code, or wrong format idk");
				
			if (returnCode >= 300 && returnCode < 400)
			{
				generate3xxRedirectResponse(returnCode);
				_processStatus = READING_BODY;
				return ;
			}
			else
				generate4xx5xxErrorReponse(500, false, "Internal Error::config file::redirect contain status code that is not 3xx");
		}
	}

	// check the method
	// return 405 if method is not allowed
	{
		// _method = _method of this current request

		// allowed_methods in the targeted block of the config file
		const std::vector<std::string>* allowMethodPtr = _targetServer->getLocationData(_targetLocationBlock, "allowed_methods");

		if (allowMethodPtr == NULL)
			generate4xx5xxErrorReponse(500, false, "Internal Error::allowed_methods not found in the targeted location block");
		
		bool found = false;
		for (size_t i = 0; i < allowMethodPtr->size(); i++)
		{
			if (_method == (*allowMethodPtr)[i])
			{
				found = true;
				break ;
			}
		}

		// the method is not allowed in this location block
		// return 405 Not Implemented
		if (found == false)
		{
			generate4xx5xxErrorReponse(405, false, "Http::method::not implemented::" + _method);
		}
	}

	bool isEndWithSlash = _targetPath[_targetPath.size() - 1] == '/' ? true : false;

	if (_method != "DELETE")
	{

		// should not delete file with 'index' seems to make sense to me
		if (isEndWithSlash == true)
		{
			// if the path end with slash
			// try to find the 'index' in that location block
			
			const std::vector<std::string>* foundIndex = _targetServer->getLocationData(_targetLocationBlock, "index");

			// only append if found
			if (foundIndex)
			{
				if (foundIndex->size() != 1)
				{
					generate4xx5xxErrorReponse(500, false, "Internal Error:: invalid \'index\' directive in location block");
				}

				_targetPath += (*foundIndex)[0];
				isEndWithSlash = false;
			}
		}

		{
			size_t dotPos = _targetPath.find_last_of('.');

			// mean that the target having some extensions to check
			if (dotPos != _targetPath.npos)
			{
				std::string	extensionStr = _targetPath.substr(dotPos);
				// here, we need cgi_pass directive in configuration file

				/*
				the cgi pass should consist of 2 elements
				first is the extension of the CGI type (like .php, .py)
				and second is their executable in absolute path
				*/

				// first we get the cgi_pass vector

				const std::vector<std::string>* cgi_pass_vec = _targetServer->getLocationData(_targetLocationBlock, "cgi_pass");

				// if found then check
				if (cgi_pass_vec != NULL)
				{
					// check the whole vector if it matches
					size_t i = 0;

					while (i < cgi_pass_vec->size())
					{
						// process here is simple, if some thing doesn't work as expected
						// the config file parsing is bad
						if (extensionStr == (*cgi_pass_vec)[i])
						{
							// the next index should be the path
							if (i + 1 == cgi_pass_vec->size())
								generate4xx5xxErrorReponse(500, false, "Internal Error::'cgi_pass' in configuration file is wrong:: must be [CGI extension] followed by [absolute path to CGI]");
							else
								_cgiPath = (*cgi_pass_vec)[i + 1];
						}
						i += 2;
					}
				}

				// now we know if the request target is CGI or not by
				// checking _cgiPath.empty()
			}
		}
	}

	// now we can check the file existence
	{

		// we need to get the system path first
		std::string systemPath;
		/*
			The reason to have this section is we need to know
			whether we want to pick 'root' or 'upload_store'

			- the GET method always need 'root'
			- the POST method depends if the target is CGI or not
				if CGI then use 'root' if not then 'upload store'
				but if not found 'root' then just take 'upload_store'
			- the DELETE also 
		*/
		{
			if (_method == "POST" && _cgiPath.empty())
			{



				// should use 'upload_store' may be we can give this as a must in configuration file ??
				// try with this method first
				const std::vector<std::string>* foundUploadStore = _targetServer->getLocationData(_targetLocationBlock, "upload_store");

				/*
				i'm not sure but now i'm assume that if you upload a file 
				via post method, you must explicitly define 'upload_store' in that location
				block in configuration file	
				*/
				if (foundUploadStore == NULL)
					generate4xx5xxErrorReponse(500, false, "Internal Error::using POST to upload file to server requires \'upload_store\' to be defined in configuration file");

				// just for sure checking
				if (foundUploadStore->size() != 1 || (*foundUploadStore)[0].empty())
				{
					generate4xx5xxErrorReponse(500, false, "Internal Error::invalid \'root\' value");
				}

				_uploadStorePath = (*foundUploadStore)[0];
				systemPath = (*foundUploadStore)[0];
			}
			else // GET or DELETE will use root always
			{

				// take 'root' as main path

				const std::vector<std::string>* foundRoot = _targetServer->getLocationData(_targetLocationBlock, "root");

				// it should not be null but if so then treat as internal error
				if (foundRoot == NULL)
				{
					generate4xx5xxErrorReponse(500, false, "Internal Error::require 'root' in this location block");
				}

				// just for sure checking
				if (foundRoot->size() != 1 || (*foundRoot)[0].empty())
				{
					generate4xx5xxErrorReponse(500, false, "Internal Error::invalid \'root\' value");
				}

				systemPath = (*foundRoot)[0];
			}
		}

		// we need to combine root of location block
		// with the URI that we resolved
		_combinedPath = systemPath + _targetPath;

		//struct stat fileStat;
		//std::memset(&fileStat, 0, sizeof(fileStat));
		//if (stat(_combinedPath.c_str(), &fileStat) != 0)
		//{
		//	std::string ErrMsg = "Http::stat()::target_path " + _targetPath + "::";
		//	ErrMsg += strerror(errno);
		//	if (errno == EACCES)
		//		throw HttpThrowStatus(403, ErrMsg);
		//	throw HttpThrowStatus(404, ErrMsg);
		//}

		//check for DELETE method first
		/*
			my implement is whether the file 
			is a directory or regular file
			the DELETE just
			called std::remove()
			and return value would work
			the same and don't need those
			CGI and any other process to work	
		*/
		if (_method == "DELETE")
		{
			if (std::remove(_combinedPath.c_str()) != 0)
			{
				if (errno == ENOENT)
					generate4xx5xxErrorReponse(404, false, "Http::DELETE method::file not found");
				else if (errno == EACCES)
					generate4xx5xxErrorReponse(403, false, "Http::DELETE method::permission denied");
				else
					generate4xx5xxErrorReponse(500, false, "Internal Error::DELETE method::std::remove()::failed::" + std::string(strerror(errno)));
				
			}
			else
			{
				/*
				DELETE on success can return 2 ways
				
				200 ok need to have body to return
				and
				204 no content means that the operation
				is success but will not send the body
				*/

				// but i don't know yet so just
				// 204
				{
					_httpResponseList.push_back(HttpResponse());
					throw HttpThrowStatus(204, "Http::DELETE method::success no content");
				}
			}
		}
		else if (_method == "GET")
		{
 
			/*
			GET	method need to check if the path
			i wants is exist and whether it is
			a directory or not
			*/
			struct stat fileStat;
			std::memset(&fileStat, 0, sizeof(fileStat));
			if (stat(_combinedPath.c_str(), &fileStat) != 0)
			{
				std::string ErrMsg = "Http::stat()::target_path " + _targetPath + "::";
				ErrMsg += strerror(errno);
				if (errno == EACCES)
					generate4xx5xxErrorReponse(403, false, ErrMsg);
				generate4xx5xxErrorReponse(404, false, ErrMsg);
			}

			if (S_ISDIR(fileStat.st_mode))
			{
				// check if it target ends with '/' or not
				if (isEndWithSlash == true)
				{
					// check
					// check for directory listing (auto index)

					const std::vector<std::string>* foundAutoIndex = _targetServer->getLocationData(_targetLocationBlock, "autoindex");

					// if not found then should return 403 forbidden or not found
					if (foundAutoIndex == NULL)
						generate4xx5xxErrorReponse(403, false, "Http::\"autoindex\" is not found in this block.");

					if (foundAutoIndex->size() != 1)
						generate4xx5xxErrorReponse(403, false, "Http::\"autoindex\" invalid value");

					if ((*foundAutoIndex)[0] == "on")
					{
						// generate auto indexing 
					}
					else
						generate4xx5xxErrorReponse(403, false, "Http::\"autoindex\" is not on for directory listing");
				
				}
				else
				{
					// make redirections
					_targetPath += "/";
					generate3xxRedirectResponse(301);
					_processStatus = READING_BODY;
					return ;

				}
			}

			// check if the file is regular file. must 
			// be regular file, right ?
			if (S_ISREG(fileStat.st_mode))
			{
				// GET will not have body because i want it that way
				
				// check if it is CGI or not
				if (_cgiPath.empty())
				{

					// try to open the targeted file
					int fd = open(_combinedPath.c_str(), O_RDONLY);

					// if failed should response accordingly
					if (fd < 0)
					{
						if (errno == EACCES)
							generate4xx5xxErrorReponse(403, false, "Http::GET to regular file failed::open() failed");

						else if (errno == EMFILE || errno == ENFILE)
						{
							generate4xx5xxErrorReponse(500, false, "Http:: GET to regular file:: fd limit is reached");
						}
						else if (errno == ENOENT)
						{
							generate4xx5xxErrorReponse(404, false, "Http:: Get to regular file:: file is missing");
						}
						// some unknown error
						else
							generate4xx5xxErrorReponse(500, false, "Http:: Get to regular file::Internal Unknown Error");
					}

					FileDescriptor tempFd = fd;

					// now the file is open and ready to read
					_httpResponseList.push_back(HttpResponse());

					HttpResponse& targetResponse = _httpResponseList.back();
 
					std::cout << "combined path is: \"" << _combinedPath << "\"" << std::endl;
					std::cout << "Content-Type: " << contentTypeTable().extensionToContentType(_combinedPath) << std::endl;

					// set Last-Modified
					{
						// the st_mtim is time_t

						tm * timeGmt = std::gmtime(&fileStat.st_mtim.tv_sec);

						std::vector<char> tempTimeBuffer(100);

						std::strftime(&tempTimeBuffer[0], tempTimeBuffer.size(), "%a, %d %b %Y %H:%M:%S GMT", timeGmt);

						std::string tempModTime = &tempTimeBuffer[0];

						targetResponse.addHeader("Last-Modified", tempModTime);
					}

					targetResponse.setContentType(contentTypeTable().extensionToContentType(_combinedPath));
					targetResponse.setResponseBodyType(HTTP_RESPONSE_BODY_FILE);
					targetResponse.setFileFd(tempFd);
					targetResponse.setKeepAfterResponse(true);
					targetResponse.setStatusCode(200);
					targetResponse.setStatusMessage("OK");

					_processStatus = FINISHED_READ_BODY;
					targetResponse.generateResponse();
					return ;
				}
				else
				{
					// GET to CGI didn't build yet so
					generate4xx5xxErrorReponse(500, false, "Not CGI yet");
				}
			}
			else 
			{
				generate4xx5xxErrorReponse(403, false, "target is not directory or regular file");
			}
		}

		// for post method just leave it as error for now
		else
		{
			generate4xx5xxErrorReponse(500, false, "Post did not finished yet");
		}

	}




}


// use the _requestBuffer
void	Http::processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	try
	{
		size_t	currIndex = 0;
		size_t	reqBuffSize = _requestBuffer.size();

		// put into structure
		parsingHttpRequestLine(currIndex, reqBuffSize);
		parsingHttpHeader(currIndex, reqBuffSize);

		validateRequestBufffer(clientSocket);

	}
	catch (std::exception &e)
	{
		Logger::log(LC_ERROR, "something weird, Consider as server error ::%s", e.what());
		generate4xx5xxErrorReponse(500, false, e.what());
		//throw HttpThrowStatus(500, e.what());
	}

	return ;
}

void Http::readFromClient(const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	ssize_t	readAmount;

	readAmount = recv(clientSocket.getSocketFD().getFd(), &_recvBuffer[0], HTTP_RECV_BUFFER, 0);
	if (readAmount == 0)
	{
		Logger::log(LC_CONN_LOG, "Disconnecting client Socket#%d", clientSocket.getSocketFD().getFd());
		_keepConnection = false;
		return ;
	}
	else if (readAmount < 0)
	{
		return ;
	}
	else 
	{
		_requestBuffer.append(&(_recvBuffer[0]), readAmount);
		try
		{
			processingRequestBuffer(clientSocket, socketMap);
		}
		catch (HttpThrowStatus &status)
		{
			Logger::log(LC_INFO, "Http::socket#%d::reponse with status code %d::%s", clientSocket.getSocketFD().getFd(), status.statusCode(), status.message().c_str());
		}
		catch (std::exception &e)
		{

		}
		catch (...)
		{
			// what 
		}
	}
	
	if (_isEpollout == false && _httpResponseList.empty() == false && _httpResponseList.front().hasSomethingtoSend() == true)
	{
		epoll_event	event;
		std::memset(&event, 0, sizeof(event));
		event.events = EPOLLIN | EPOLLOUT;
		event.data.fd = clientSocket.getSocketFD().getFd();

		if (epoll_ctl(clientSocket.getEpollFD().getFd(), EPOLL_CTL_MOD, clientSocket.getSocketFD().getFd(), &event) == -1)
		{
			std::string msg = "epoll_ctl()::error::";
			msg += std::strerror(errno);
			throw WebservException(msg);
		}
		_isEpollout = true;
	}
	//_keepConnection = false;
}

void	Http::writeToClient(const Socket& clientSocket, std::map<int, Socket>& SocketMap)
{
	HttpResponse *response_block = NULL;

	// check if it is empty or what
	if (_httpResponseList.empty() || _httpResponseList.front().isComplete())
	{
		Logger::log(LC_DEBUG, "WHAAAAAAT");
		return ;
	}
	response_block = &_httpResponseList.front();

	ssize_t	sendReturn = response_block->sendHttpResponse(clientSocket);

	if (sendReturn < 0)
	{
		Logger::log(LC_DEBUG, "WHATTT2");
		_keepConnection = false;
		return ;
	}

	// remove this reponse from list if complete
	if (response_block->isComplete())
	{
		//check keepafterresponse
		_keepConnection = response_block->getKeepAfterResponse();
		_httpResponseList.pop_front();
	}

	// then if the response list is empty now
	// we close the EPOLLOUT
	if (_httpResponseList.empty() || _httpResponseList.front().hasSomethingtoSend() == false)
	{
		epoll_event	event;
		std::memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = clientSocket.getSocketFD().getFd();

		if (epoll_ctl(clientSocket.getEpollFD().getFd(), EPOLL_CTL_MOD, clientSocket.getSocketFD().getFd(), &event) == -1)
		{
			std::string msg = "epoll_ctl()::error::";
			msg += std::strerror(errno);
			throw WebservException(msg);
		}
		_isEpollout = false;
	}
}
