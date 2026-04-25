#include "../../include/classes/HttpCgi.hpp"
#include "../../include/classes/Socket.hpp"
#include "../../include/classes/WebServ.hpp"

HttpCgi::HttpCgi()
:_clientResponseList(NULL),
_cgiTargetResponse(NULL),
_isFinishedRead(false),
_keepConnection(true),
_cgioutProcessStatus(HTTPCGIOUT_NO_STATUS)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::HttpCgi(std::list<HttpResponse>* clientResponseList,
HttpResponse* cgiTargetResponse,
const FileDescriptor& mainHttpSocket,
Socket *thisCgiSocket)
: _clientResponseList(clientResponseList),
_cgiTargetResponse(cgiTargetResponse),
_thisCgiSocket(thisCgiSocket),
_mainHttpSocketFd(mainHttpSocket),
_isFinishedRead(false),
_keepConnection(true),
_cgioutProcessStatus(HTTPCGIOUT_NO_STATUS)

{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::HttpCgi(std::list<HttpResponse>* clientResponseList,
HttpResponse* cgiTargetResponse,
const FileDescriptor& mainHttpSocket,
Socket *thisCgiSocket,
const FileDescriptor& tempReadFileFd)
: _clientResponseList(clientResponseList),
_cgiTargetResponse(cgiTargetResponse),
_thisCgiSocket(thisCgiSocket),
_mainHttpSocketFd(mainHttpSocket),
_tempReadFileFd(tempReadFileFd),
_isFinishedRead(false),
_keepConnection(true),
_cgioutProcessStatus(HTTPCGIOUT_NO_STATUS)
{
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::HttpCgi(const HttpCgi& obj)
: _clientResponseList(obj._clientResponseList),
_cgiTargetResponse(obj._cgiTargetResponse),
_thisCgiSocket(obj._thisCgiSocket),
_mainHttpSocketFd(obj._mainHttpSocketFd),
_isFinishedRead(obj._isFinishedRead),
_keepConnection(obj._keepConnection),
_cgioutProcessStatus(obj._cgioutProcessStatus)
{
	if (obj._tempReadFileFd.getFd() >= 0)
		_tempReadFileFd = obj._tempReadFileFd;
	_readCgiBuffer.reserve(HTTP_READ_FROM_CGI_BUFFER_SIZE);
}

HttpCgi::~HttpCgi()
{
}

bool HttpCgi::isKeepConnection() const
{
	return (_keepConnection);
}

void HttpCgi::generate5xxCGIOUTresponseError(unsigned int errorCode, const std::string& throwMsg)
{
	bool	hasDefaultErrorPageFile = false;

	const std::vector<std::string>* foundErrorPageVec = NULL;

	const ServerConfig* targetServerPtr = _cgiTargetResponse->targetServer;
	const t_config_map* targetLocationPtr = _cgiTargetResponse->targetLocationBlock;

	if (targetServerPtr != NULL)
	{
		if (targetLocationPtr != NULL)
		{
			foundErrorPageVec = targetServerPtr->getLocationData(targetLocationPtr, "error_page");
		}
		else
		{
			foundErrorPageVec = targetServerPtr->getServerData("error_page");
		}
	}

	std::string errorPageFilePath;

	size_t convertCode;

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

			if (convertCode == errorCode)
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
	size_t			fileSize = 0;

	if (!errorPageFilePath.empty())
	{
		// check stat to get the file 
		struct stat fileStat;
		std::memset(&fileStat, 0, sizeof(fileStat));
		if (stat(errorPageFilePath.c_str(), &fileStat) == 0)
		{
			if (S_ISREG(fileStat.st_mode))
			{
				int fd = open(errorPageFilePath.c_str(), O_RDONLY);
				if (fd > -1)
				{
					fileSize = fileStat.st_size;
					errorFileFD = fd;
					hasDefaultErrorPageFile = true;
				}
			}
		}
	}

	_cgiTargetResponse->keepAfterResponse = false;
	_cgiTargetResponse->statusCode = errorCode;
	_cgiTargetResponse->contentType = "text/html";

	switch (errorCode)
	{
		case (500):
		{
			_cgiTargetResponse->statusMessage = "Internal Error";
			break;
		}
		default:
		{
			_cgiTargetResponse->statusMessage = "";
			break;
		}
	}

	if (hasDefaultErrorPageFile == true)
	{
		_cgiTargetResponse->responseBodyType = HTTP_RESPONSE_BODY_FILE;
		_cgiTargetResponse->fileFd = errorFileFD;
		_cgiTargetResponse->fileSize = fileSize;
	}
	else
	{
		_cgiTargetResponse->responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

		std::string htmlMsg = toString(errorCode) + " " + _cgiTargetResponse->statusMessage;

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
		
		bodyStr +=
		"</center>\r\n"
		"</body>\r\n"
		"</html>\r\n";

		_cgiTargetResponse->fixedBodyStr = bodyStr;
	}

	_cgiTargetResponse->generateResponse();

	throw HttpThrowStatus(errorCode, throwMsg);
}

void HttpCgi::parsingCGIOUTresponseHeader()
{
	if (_cgioutProcessStatus == HTTPCGIOUT_NO_STATUS)
		_cgioutProcessStatus = HTTPCGIOUT_READING_RESPONSE_HEADER;

	if (_cgioutProcessStatus != HTTPCGIOUT_READING_RESPONSE_HEADER)
		return ;

	size_t	responseBuffSize = _responseBuffer.size();
	size_t	currIndex = 0;
	bool	isCRLF;
	size_t	colonPos;
	std::string tempFieldName;
	std::string tempFieldValue;
	std::string tempSep;
	size_t temp;

	size_t endlinePos;
	while (_cgioutProcessStatus == HTTPCGIOUT_READING_RESPONSE_HEADER)
	{
		endlinePos = _responseBuffer.find('\n', currIndex);

		// cannot find endline yet, so wait for later
		if (endlinePos == std::string::npos)
		{
			if (currIndex != 0)
				_responseBuffer.erase(0, currIndex);
			return ;
		}

		// handle both if CRLF or just LF
		if (endlinePos == currIndex
		|| (endlinePos - 1 == currIndex
			&& _responseBuffer[currIndex] == '\r'))
		{
			/* the logic is simple because currindex will always 
			move to after the endline pos so if it found again 
			the '\n' position at currindex, that signals that
			this line is empty and should finished and process the header 
			but how about the CRLF ?? simple, the endlinePos is need
			to be at currIndex + 1 and currIndex needs to be '\r' */

			_cgioutProcessStatus = HTTPCGIOUT_VALIDATING_RESPONSE;
			if (currIndex + 2 >= responseBuffSize)
				_responseBuffer.clear();
			else
			{
				/* here we are suppose to remove to the endline pos*/
				_responseBuffer.erase(0, endlinePos + 1);
			}
			break ;
		}

		/* use to skip the character */
		if (endlinePos > 0 && _responseBuffer[endlinePos - 1] == '\r')
			isCRLF = true;
		else
			isCRLF = false;

		/* now we need to extract that line to our data structure
		and it need to throw some errors now*/

		/* colonPos, each line ALWAYS needs to have ':' to separate 
		header field name and header field value*/
		colonPos = _responseBuffer.find_first_of(':', currIndex);
		if (colonPos == std::string::npos
		|| colonPos >= (isCRLF == false ? endlinePos : endlinePos - 1))
		{
			/* we need to generate error here, and we can do that by
			writing directly to the _cgiTargetResponse*/

			/* maybe we can still use generate response, just need to
			add the appropriate response header */

			generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::parsing Response Header::"
			"name and value the header field must separated by \':\'");
		}

		tempFieldName = _responseBuffer.substr(currIndex, colonPos - currIndex);
		if (tempFieldName.empty())
			generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::parsing Response Header::"
			"name in header field must not empty string");

		if (httpFieldNameChar().isMatch(tempFieldName) == false)
		{
			generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::parsing Response Header::"
			"name in header field must not contain any forbiddin char");
		}
		if (allAlphaChar()[tempFieldName[0]] == false)
			generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::parsing Response Header::"
			"name in header field must with a letter");

		currIndex = colonPos + 1;
		tempSep = _responseBuffer.substr(currIndex, (isCRLF == false ? endlinePos - currIndex : endlinePos - currIndex - 1));

		/*trim the SP and HTAB */
		tempSep = my_ft_trim(tempSep, " \t");

		if (!tempSep.empty())
		{
			if (forbiddenFieldValueChar().isNotMatch(tempSep) == false)
				generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::parsing Response Header::"
				"name in header field must with a letter");
		}
		else
		/* A NULL field value is equivalent to a field not being sent.*/
		{
			currIndex = isCRLF == false ? endlinePos + 1 : endlinePos + 2;
			continue;
		}

		tempFieldName = stringToLower(tempFieldName);


		/* from the CGI documentation
				Each CGI field MUST NOT appear more than once in the response
			CGI-field = Content-Type | Location | Status */
		if ((tempFieldName == "content-type"
			|| tempFieldName == "location"
			|| tempFieldName == "status")
		&& _responseHeaderCGIOUT.find(tempFieldName) == _responseHeaderCGIOUT.end())
		{
			generate5xxCGIOUTresponseError(400, "CGIOUT::Bad request:: Each CGI field MUST NOT appear more than once in the response");
		}

		std::string &headerValueTarget = _responseHeaderCGIOUT[tempFieldName];

		if (headerValueTarget.empty() == false)
			headerValueTarget += ", ";

		headerValueTarget += tempSep;

		currIndex = isCRLF == false ? endlinePos + 1 : endlinePos + 2;
		
	}

	return;
}

void HttpCgi::validateCGIOUTresponse()
{
	/* check the response header overall */

	if (_cgioutProcessStatus != HTTPCGIOUT_VALIDATING_RESPONSE)
		return ;
	/* would not tolerate if the response has no header */
	if (_responseHeaderCGIOUT.empty())
		generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::the response must have header field");

	/* From the CGI documentation, there are 4 main response types so we can
	separate that here */

	/* If found the location header we need to check if it is local redirect or
	client redirect*/

	std::map<std::string, std::string>::const_iterator foundLocation = _responseHeaderCGIOUT.find("location");
	std::map<std::string, std::string>::const_iterator foundContentType = _responseHeaderCGIOUT.find("content-type");

	if (foundLocation != _responseHeaderCGIOUT.end())
	{
		/* The Location header field is used to specify to the server that the script is returning
		a reference to a document rather than an actual document. It is either an absolute URI,
		indicaing that the client is to fetch the referenced document, or a local URI path
		(optionally with a query string), indicating that the server is to fetch the referenced
		document and return it to the client as the response

		*/
		const std::string& string = foundLocation->second;		
		
		/* if it starts with the '/' assuming it might be local-Location
		and else i would assume that it is client-Location */
		if (string[0] == '/')
		{
			/* local-Location here */
			/* if the request contained more than the Location header
			the CGI documention states that
				
			Local Redirect Response
				The CGI script can return a URI path and query-string
				('local-pathquery') for a local resource in a Location
				header field using the path specified.

				local-redir-response = local-Location NL

				The script MUST NOT return any other header fields or a
				message-body, and the serber MUST generate the response
				that it would have produced in response to a request
				containing the URL
			*/
			if (_responseHeaderCGIOUT.size() != 1)
				generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::local-Location header detected, but the ");

			/* we need to reprocess the whole request again but change the requestTarget */
			
			/* get the old request data and remodify it*/
			HttpRequest& newRequestData = _cgiTargetResponse->httpRequestData[0];

			/* what i need to change here */
			newRequestData.processStatus = VALIDATING_REQUEST;

			/* change the request target to new ?*/
			newRequestData.requestData.requestTarget = string;

			std::map<int, s_webserv_custom_event>& customEventMap = *_thisCgiSocket->getEventContoller().customEventMap;

			s_webserv_custom_event& targetCustomEvent = customEventMap[_thisCgiSocket->getSocketFD().getFd()];

			targetCustomEvent.httpRequestData.hasData = true;
			targetCustomEvent.httpRequestData.data = newRequestData;

			/* now i think we should finish this CGI socket so we can reprocess all over again */
			_keepConnection = false;
			_isFinishedRead = true;
			_cgioutProcessStatus = HTTPCGIOUT_FINISHED;
			return ;
		}
		else
		{

		}

	}
	else if (foundContentType != _responseHeaderCGIOUT.end())
	{


	}
	/* Don't know what type*/
	else 
	{
		generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::the response type doens't match with any");
	}

	
}

void HttpCgi::processCGIOUTresponseBuffer()
{
	/* should kinda the same as normal http reading the buffer */
	/* the important thing to note here is that, NL or the newline element
		If we look into the document, it stated like this
	*/
	/*
		NL = <newline>	

		Note that newline (NL) need not be a single control character,
		but can be a sequence of control characters. A system
		MAY define TEXT to be a larget set of characters than
		<any CHAR excluding CTLs but including LWSP>

		So i think so simply avoid any confusions, because
		it can be anything and it is very hard for me to process
		with all different NL element

		after doing some google searchs, decided that should accept
		only LF <\n> or CRLF <\r\n> only (i little bit different from
		HTTP1.1 that i built. That one accepts only CRLF)
	*/
	parsingCGIOUTresponseHeader();

}


void HttpCgi::readFromCGI()
{
	// use read() right?
	ssize_t	readAmount;

	readAmount = read(_thisCgiSocket->getSocketFD().getFd(), &_readCgiBuffer[0], HTTP_READ_FROM_CGI_BUFFER_SIZE);
	// this mean done
	if (readAmount == 0)
	{
		Logger::log(LC_CONN_LOG, "CGI socket out (has nothing to read).#%d:: Disconnecting..", _thisCgiSocket->getSocketFD().getFd());
		_isFinishedRead = true;

		// something here that would remove this socket cleanly
	}
	else if (readAmount < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;

		_keepConnection = false;
		// something here that would remove this socket cleanly
		return ;
	}
	else
	{
		_responseBuffer.append(&_readCgiBuffer[0], readAmount);
		/* we can try the same method from http here. but the implementation
		is a bit different from Http class so it would be kinda the same but it's not
		*/
	}
	return ;
}

void HttpCgi::sendToCGI()
{
	ssize_t	writeAmount;
}
