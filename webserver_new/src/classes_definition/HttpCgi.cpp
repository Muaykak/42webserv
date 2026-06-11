#include "../../include/classes/HttpCgi.hpp"
#include "../../include/classes/Socket.hpp"
#include "../../include/classes/WebServ.hpp"
#include "../../include/classes/TempFileManager.hpp"
#include "../../include/utility_function.hpp"

HttpCgi::HttpCgi()
:
_clientResponseList(NULL),
_cgiTargetResponse(NULL),
_cgiOutSocket(NULL),
httpCgiStatus(HTTPCGI_NO_STATUS),
_readCgiBuffer(HTTP_READ_FROM_CGI_BUFFER_SIZE),
_isFinishedRead(false),
_keepConnection(true)
{
	_writeCgiBuffer.reserve(HTTP_WRITE_TO_CGI_BUFFER_SIZE);
}

void HttpCgi::setHttpCgiNoCgiIn(std::list<HttpResponse>* clientResponseList,
	HttpResponse* cgiTargetResponse,
	Socket *cgiOutSocket, Shared<CgiProcess>& cgiProcessData)
{
	_clientResponseList = clientResponseList;
	_cgiTargetResponse = cgiTargetResponse;
	_cgiOutSocket = cgiOutSocket;
	this->cgiProcessData = cgiProcessData;
	_isFinishedRead = false;
	httpCgiStatus = HTTPCGI_READING_RESPONSE_HEADER;
}

void HttpCgi::setHttpCgiHasCgiIn(std::list<HttpResponse>* clientResponseList,
	HttpResponse* cgiTargetResponse,
	Socket *cgiOutSocket, Socket* cgiInSocket,
	const s_http_cgi_temp_file_data& tempFileData,
	Shared<CgiProcess>& cgiProcessData)
{
	_clientResponseList = clientResponseList;
	_cgiTargetResponse = cgiTargetResponse;
	_cgiOutSocket = cgiOutSocket;
	_cgiInSocket = cgiInSocket;
	_tempFileData = tempFileData;
	this->cgiProcessData = cgiProcessData;
	_isFinishedRead = false;
	_keepConnection = true;
	httpCgiStatus = HTTPCGI_SENDING_TO_CGI;
}

HttpCgi::HttpCgi(const HttpCgi& obj)
:
_clientResponseList(obj._clientResponseList),
_cgiTargetResponse(obj._cgiTargetResponse),
_cgiOutSocket(obj._cgiOutSocket),
_cgiInSocket(obj._cgiInSocket),
httpCgiStatus(obj.httpCgiStatus),
_readCgiBuffer(HTTP_READ_FROM_CGI_BUFFER_SIZE),
_tempFileData(obj._tempFileData),
_isFinishedRead(obj._isFinishedRead),
_keepConnection(obj._keepConnection)
{
	_writeCgiBuffer.reserve(HTTP_WRITE_TO_CGI_BUFFER_SIZE);
}

HttpCgi::~HttpCgi()
{
	if (_tempFileData.hasData())
	{
		tempFileManager().removeTempFile(_tempFileData->tempFileNum);
		_tempFileData.clear();
	}

	/* heereree */
	if (httpCgiStatus == HTTPCGI_SENDING_TO_CGI || httpCgiStatus == HTTPCGI_READING_RESPONSE_HEADER)
	{
		/* if this HttpCgi object, during the process that we not finish working with Cgi process yet
		meaning that we */

		try
		{
			generate5xxCGIOUTresponseError(500, "Internal Error::CGI closed unfinished");
		}
		catch (HttpThrowStatus &e)
		{
			Logger::log(LC_INFO, "Http::CgiSocket#%d::response with status code %d::%s", _cgiOutSocket->getSocketFD().getFd(), e.statusCode(), e.message().c_str());
		}
		catch (std::exception &e)
		{
			Logger::log(LC_INFO, "CgiSocket#%d::Error Occurred when writing error response::%s", _cgiOutSocket->getSocketFD().getFd(), e.what());
		}
		catch (...)
		{
			Logger::log(LC_INFO, "CgiSocket#%d::unknown error", _cgiOutSocket->getSocketFD().getFd());
		}

		httpCgiStatus = HTTPCGI_FINISHED;
	}

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
	//FileDescriptor errorFileFD;
	Shared<std::ifstream> targetErrorFile;
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
				//int fd = open(errorPageFilePath.c_str(), O_RDONLY);
				while (true)
				{
					targetErrorFile->open(errorPageFilePath.c_str());
					if (targetErrorFile->is_open())
						break ;
					if (errno == EINTR)
					{
						targetErrorFile->clear();
						continue ;
					}
					break ;
				}
				
				if (targetErrorFile->is_open() == false)
				{
					fileSize = fileStat.st_size;
					//errorFileFD = fd;
					hasDefaultErrorPageFile = true;
				}
			}
		}
	}

	_cgiTargetResponse->keepAfterResponse = false;
	_cgiTargetResponse->statusLine->first = errorCode;
	_cgiTargetResponse->contentType = "text/html";

	_cgiTargetResponse->statusLine->second = getStatusCodeMessage(errorCode);

	if (hasDefaultErrorPageFile == true)
	{
		_cgiTargetResponse->responseBodyType = HTTP_RESPONSE_BODY_FILE;
		_cgiTargetResponse->fileBody = targetErrorFile;
		_cgiTargetResponse->fileSize = fileSize;
	}
	else
	{
		_cgiTargetResponse->responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

		std::string htmlMsg = toString(errorCode) + " " + _cgiTargetResponse->statusLine->second;

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
	if (httpCgiStatus != HTTPCGI_READING_RESPONSE_HEADER)
		return ;

	size_t	responseBuffSize = _responseBuffer.size();
	size_t	currIndex = 0;
	bool	isCRLF;
	size_t	colonPos;
	std::string tempFieldName;
	std::string tempFieldValue;
	std::string tempSep;

	size_t endlinePos;
	while (httpCgiStatus == HTTPCGI_READING_RESPONSE_HEADER)
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

			//{
			//	std::map<std::string, std::string>::iterator it = _responseHeaderCGIOUT.begin();

			//	std::cout << "======================== CGI HEADER OUT ========================\n";
			//	while (it != _responseHeaderCGIOUT.end())
			//	{
			//		std::cout << it->first << ": " << it->second << std::endl;
			//		++it;
			//	}
			//	std::cout << "================================================================\n";

			//}

			httpCgiStatus = HTTPCGI_VALIDATING_RESPONSE;
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

			std::cout << std::endl << "######### PRINT WHOLE RESPONSE BUFFER ######" << std::endl;
			std::cout << _responseBuffer;
			std::cout << std::endl << "############################################" << std::endl;

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
			"name in header field must not contain any forbiddin char:: " + tempFieldName);
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
			currIndex = endlinePos + 1;
			continue;
		}

		tempFieldName = stringToLower(tempFieldName);


		/* from the CGI documentation
				Each CGI field MUST NOT appear more than once in the response
			CGI-field = Content-Type | Location | Status */
		if ((tempFieldName == "content-type"
			|| tempFieldName == "location"
			|| tempFieldName == "status")
		&& _responseHeaderCGIOUT.find(tempFieldName) != _responseHeaderCGIOUT.end())
		{
			generate5xxCGIOUTresponseError(400, "CGIOUT::Bad request:: Each CGI field MUST NOT appear more than once in the response");
		}

		std::string &headerValueTarget = _responseHeaderCGIOUT[tempFieldName];

		if (headerValueTarget.empty() == false)
			headerValueTarget += ", ";

		headerValueTarget += tempSep;

		currIndex = endlinePos + 1;
		
	}

	return;
}

void HttpCgi::validateCGIOUTresponse()
{
	/* check the response header overall */

	if (httpCgiStatus != HTTPCGI_VALIDATING_RESPONSE)
		return ;

	/* would not tolerate if the response has no header */
	if (_responseHeaderCGIOUT.empty())
		generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::Test If CGI REACH HERE");

	/* From the CGI documentation, there are 4 main response types so we can
	separate that here */

	/* If found the location header we need to check if it is local redirect or
	client redirect*/

	/* does not accept trailer header */
	{
		std::map<std::string, std::string>::const_iterator foundTrailerHeader = _responseHeaderCGIOUT.find("trailer");

		if (foundTrailerHeader != _responseHeaderCGIOUT.end())
			generate5xxCGIOUTresponseError(500, "Internal Error::Cgi Out::Trailer Header Not Allowed");
	}

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
			

			if (cgiProcessData->clientSocketPtr.hasData() == true)
			{
				/* get the old request data and remodify it*/
				HttpRequest& newRequestData = *_cgiTargetResponse->httpRequestData;

				/* what i need to change here */
				newRequestData.setProcessStatus(VALIDATING_REQUEST);

				/* change the request target to new ?*/
				newRequestData.requestData.requestTarget = string;

				std::map<int, s_webserv_custom_event>& customEventMap = *_cgiOutSocket->getEventContoller().customEventMap;

				s_webserv_custom_event& targetCustomEvent = customEventMap[cgiProcessData->clientSocketPtr->getSocketFD().getFd()];

				targetCustomEvent.httpRequestData = newRequestData;

				/* now i think we should finish this CGI socket so we can reprocess all over again */
				_keepConnection = false;
				_isFinishedRead = true;
				httpCgiStatus = HTTPCGI_FINISHED;

			}
			return ;
		}
		else
		{
			/* if not start with '/' then it is client redirection 
			
			according to RFC3875 i can set the return status to 302 found */

			/* if the status header is omitted then assumed that it is 302*/
			{
				std::map<std::string, std::string>::const_iterator foundIt = _responseHeaderCGIOUT.find("status");

				if (foundIt != _responseHeaderCGIOUT.end())
				{
					std::string statusCodeTempStr;

					if (Http::httpFieldNormalSingletonTrim(foundIt->second, statusCodeTempStr) == false)
					{
						generate5xxCGIOUTresponseError(500, "Internal Error::CGI Response::Status Value Invalid");
					}
					std::vector<std::string> splitVec;
					splitString(statusCodeTempStr, "\t ", splitVec);

					if (splitVec.size() == 0)
						generate5xxCGIOUTresponseError(500, "Internal Error::CGI Response::Status Value Invalid:: failed split");

					size_t statusCodeNum = 0;
					if (string_to_size_t(splitVec[0], statusCodeNum) == false || (statusCodeNum < 100 || statusCodeNum > 599))
					{
						generate5xxCGIOUTresponseError(500, "Internal Error::CGI Response::Status Value Invalid");
					}

					_cgiTargetResponse->statusLine->first = statusCodeNum;

					for (size_t i = 1; i < splitVec.size(); i++)
					{
						if (i > 1)
							_cgiTargetResponse->statusLine->second += " ";
						_cgiTargetResponse->statusLine->second += splitVec[i];
					}
				}
				else
				{
					_cgiTargetResponse->statusLine->first = 302;
					_cgiTargetResponse->statusLine->second = "Found";
				}
			}
		}
	}
	else
	{
		std::map<std::string, std::string>::const_iterator foundIt = _responseHeaderCGIOUT.find("status");

		if (foundIt != _responseHeaderCGIOUT.end())
		{

			std::string statusCodeTempStr;

			if (Http::httpFieldNormalSingletonTrim(foundIt->second, statusCodeTempStr) == false)
			{
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI Response::Status Value Invalid:: failed trim");
			}

			std::vector<std::string> splitVec;
			splitString(statusCodeTempStr, "\t ", splitVec);

			if (splitVec.size() == 0)
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI Response::Status Value Invalid:: failed split");

			size_t statusCodeNum = 0;
			if (string_to_size_t(splitVec[0], statusCodeNum) == false || (statusCodeNum < 100 || statusCodeNum > 599))
			{
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI Response::Status Value Invalid::" + statusCodeTempStr + std::string("::") + toString(statusCodeNum));
			}

			_cgiTargetResponse->statusLine->first = statusCodeNum;
			for (size_t i = 1; i < splitVec.size(); i++)
			{
				if (i > 1)
					_cgiTargetResponse->statusLine->second += " ";
				_cgiTargetResponse->statusLine->second += splitVec[i];
			}
		}
		else
		{
			_cgiTargetResponse->statusLine->first = 200;
			_cgiTargetResponse->statusLine->second = "OK";
		}
	}

	/* erase because i may create duplication when copy header to responsetarget*/
	_responseHeaderCGIOUT.erase("status");
	

	if (foundContentType != _responseHeaderCGIOUT.end())
	{
		/* if content type is found i need to check if it has Content-length or Transfer-Encoding */

		std::map<std::string, std::string>::const_iterator	foundContentLength = _responseHeaderCGIOUT.find("content-length");
		std::map<std::string, std::string>::const_iterator	foundTransferEncoding = _responseHeaderCGIOUT.find("transfer-encoding");

		if (foundContentLength != _responseHeaderCGIOUT.end() && foundTransferEncoding != _responseHeaderCGIOUT.end())
		{
			/* should not found both of these header*/
			generate5xxCGIOUTresponseError(500, "Internal Error::CGI response::Both Content-Length and Transfer-Encoding found");
		}
		else if (foundContentLength != _responseHeaderCGIOUT.end())
		{
			std::string tempExtractString;
			/* check the value must be chunked*/
			if (Http::httpFieldNormalSingletonTrim(foundContentLength->second, tempExtractString) == false)
			{
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI response::Content-Length invalid value");
			}

			if (string_to_size_t(tempExtractString, _bodyData->bodySize) == false || _bodyData->bodySize == 0)
			{
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI response::Content-Length invalid value");
			}

			_bodyData->isChunkBody = false;
			_cgiTargetResponse->addHeader("Content-Length", toString(_bodyData->bodySize));
			_responseHeaderCGIOUT.erase("content-length");
		}
		else if (foundTransferEncoding != _responseHeaderCGIOUT.end())
		{
			std::string tempExtractString;
			/* check the value must be chunked*/
			if (Http::httpFieldNormalSingletonTrim(foundTransferEncoding->second, tempExtractString) == false)
			{
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI response::Transfed-Encoding invalid value");
			}

			if (tempExtractString != "chunked")
			{
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI response::Transfed-Encoding invalid value");

			}

			_bodyData->isChunkBody = true;
			_bodyData->isManualChunk = false;
			_bodyData->bodySize = 0;
			_bodyData->chunkBodyIsFinished = false;
			_cgiTargetResponse->addHeader("Transfer-Encoding", "chunked");
			_responseHeaderCGIOUT.erase("transfer-encoding");
		}
		else
		{
			/* if the content-length or transfer-encoding does not found at all 
			treat body as chunked body*/

			_bodyData->isManualChunk = true;
			_bodyData->isChunkBody = true;
			_bodyData->bodySize = 0;
			_bodyData->chunkBodyIsFinished = false;
			_cgiTargetResponse->addHeader("Transfer-Encoding", "chunked");
		}

		_bodyData->curr_body_read = 0;
		_cgiTargetResponse->responseBodyType = HTTP_RESPONSE_CGI_BODY;
		_cgiTargetResponse->contentType = foundContentType->second;
		_responseHeaderCGIOUT.erase("content-type");
	}
	else
	{
		_cgiTargetResponse->responseBodyType = HTTP_RESPONSE_NOBODY;
		/* */
	}

	/* copy all header to TargetResponse */
	{
		std::map<std::string, std::string>::const_iterator copyIt = _responseHeaderCGIOUT.begin();

		std::string temp;
		while (copyIt != _responseHeaderCGIOUT.end())
		{
			temp = copyIt->first;
			stringCapital(temp);
			_cgiTargetResponse->addHeader(temp, copyIt->second);
			++copyIt;
		}
	}

	//{
	//	std::cout << " =========== PRINT CGI HEADER ==========" << std::endl;

	//	t_config_map::const_iterator it = _cgiTargetResponse->getHeader().begin();

	//	std::vector<std::string>::const_iterator vecIt;
	//	while (it != _cgiTargetResponse->getHeader().end())
	//	{
	//		std::cout << it->first << ": ";
	//		vecIt = it->second.begin();
	//		while (vecIt != it->second.end())
	//		{
	//			if (vecIt != it->second.begin())
	//				std::cout << ", ";
	//			std::cout << *vecIt;
	//			++vecIt;
	//		}
	//		std::cout << std::endl;
	//		++it;
	//	}
	//}

	/* complete checking the response header, now preparing to next process */
	_cgiTargetResponse->generateResponse();

	if (_cgiTargetResponse->responseBodyType == HTTP_RESPONSE_NOBODY)
	{
		httpCgiStatus = HTTPCGI_FINISHED;
		return ;
	}
	else
	{
		httpCgiStatus = HTTPCGI_SENDING_RESPONSE_BUFFER;
		return ;
	}


	///* Don't know what type*/
	//{
	//	generate5xxCGIOUTresponseError(500, "Internal Error::CGIOUT::the response type doens't match with any");
	//}

}

void HttpCgi::sendToResponseBuffer()
{
	if (httpCgiStatus != HTTPCGI_SENDING_RESPONSE_BUFFER)
		return ;


	/* if the buffer is empty then wait for next read event*/
	if (_responseBuffer.size() == 0)
		return ;

	/* separate ways to put into buffer by*/
	if (_bodyData->isChunkBody == false)
	{
		/* here is normal Content-Length with specified Length */

		size_t& bodySize = _bodyData->bodySize;

		if (_responseBuffer.size() < bodySize)
		{
			/* deduct the body size */
			bodySize = bodySize - _responseBuffer.size();

			s_response_buff tempBuff;

			tempBuff.buffer.insert(tempBuff.buffer.end(), _responseBuffer.begin(), _responseBuffer.end());
			tempBuff.currentIndex = 0;

			_cgiTargetResponse->pushNewResponseBuff(tempBuff);
			/* body size still not 0 then wait for next read event*/

			_responseBuffer.clear();

			return ;
		}
		else
		{
			s_response_buff tempBuff;

			std::string tempStr = _responseBuffer.substr(0, bodySize);

			tempBuff.buffer.insert(tempBuff.buffer.end(), tempStr.begin(), tempStr.end());
			tempBuff.currentIndex = 0;

			_cgiTargetResponse->pushNewResponseBuff(tempBuff);

			_responseBuffer.erase(0, bodySize);

			httpCgiStatus = HTTPCGI_FINISHED;
			return ;
		}
	}
	else
	{
		/* transfer encoding*/

		/* if it is not manual chunk then do nothing*/
		if (_bodyData->isManualChunk == false)
		{
			size_t endLinePos;

			while (true)
			{
				if (_bodyData->chunkBodyIsFinished == true)
				{
					/* true means that waiting for last bit of chunk */
					if (_responseBuffer.size() < 2)
						return ;

					std::string tempStr = "0\r\n\r\n";

					s_response_buff tempBuff;

					tempBuff.buffer.insert(tempBuff.buffer.end(), tempStr.begin(), tempStr.end());
					tempBuff.currentIndex = 0;

					_cgiTargetResponse->pushNewResponseBuff(tempBuff);


					if (_responseBuffer.size() == 2)
					{
						httpCgiStatus = HTTPCGI_FINISHED;
						_isFinishedRead = true;
						return ;
					}

					std::map<int, s_webserv_custom_event>& customEventMap = *_cgiOutSocket->getEventContoller().customEventMap;
					s_webserv_custom_event& targetCustomEvent = customEventMap[cgiProcessData->clientSocketPtr->getSocketFD().getFd()];
					targetCustomEvent.clientSocketManualDisconnect = true;
					_keepConnection = false;
					_isFinishedRead = true;
					httpCgiStatus = HTTPCGI_FINISHED;
					return ;
					/* else would consider wrong format and would trigger manual event to close
					client connection */

				}

				if (_bodyData->curr_body_read >= _bodyData->bodySize)
				{
					//endLinePos = _responseBuffer.find("\r\n");
					if (findNextCRLF(_responseBuffer, endLinePos) == false)
						generate5xxCGIOUTresponseError(400, "the \'\\r\' must always followed by \'\\n\'");

					if (endLinePos == std::string::npos)
					{
						/* maybe next read */

						return ;
					}

					std::string hexString = _responseBuffer.substr(0, endLinePos);

					size_t hexNum = 0;

					if (hex_to_size_t(hexString, hexNum) == false)
					{
						/* if conversion failed then it might be wrong value,
						thus what we can only do is just quits everything and
						disconnect the client */

						std::map<int, s_webserv_custom_event>& customEventMap = *_cgiOutSocket->getEventContoller().customEventMap;
						s_webserv_custom_event& targetCustomEvent = customEventMap[cgiProcessData->clientSocketPtr->getSocketFD().getFd()];
						targetCustomEvent.clientSocketManualDisconnect = true;
						_keepConnection = false;
						_isFinishedRead = true;
						httpCgiStatus = HTTPCGI_FINISHED;
						return ;
					}

					/* hexNum converted */

					if (hexNum == 0)
					{
						_responseBuffer.erase(0, endLinePos + 2);

						_bodyData->chunkBodyIsFinished = true;
						/* if less than 2 then wait for next read */
						if (_responseBuffer.size() < 2)
							return ;
						else
							continue;
					}
					else
					{
						_bodyData->chunkBodyIsFinished = false;
						_bodyData->bodySize += hexNum;
					}

					_responseBuffer.erase(0, endLinePos + 2);
				}


				size_t readBodyAmount = _bodyData->bodySize - _bodyData->curr_body_read;

				if (readBodyAmount > _responseBuffer.size())
					readBodyAmount = _responseBuffer.size();
				
				_bodyData->curr_body_read += readBodyAmount;

				std::string tempCutResBuff = _responseBuffer.substr(0, readBodyAmount);

				std::string startChunkHex = size_t_to_hex(readBodyAmount);

				startChunkHex += "\r\n";

				s_response_buff tempBuff;
				tempBuff.buffer.insert(tempBuff.buffer.end(), startChunkHex.begin(), startChunkHex.end());
				tempBuff.buffer.insert(tempBuff.buffer.end(), tempCutResBuff.begin(), tempCutResBuff.end());

				std::string endLine = "\r\n";

				tempBuff.buffer.insert(tempBuff.buffer.end(), endLine.begin(), endLine.end());
				tempBuff.currentIndex = 0;

				_responseBuffer.erase(0, readBodyAmount);

				_cgiTargetResponse->pushNewResponseBuff(tempBuff);
			}

			return ;
		}
		else
		{
			std::string startChunkHex = size_t_to_hex(_responseBuffer.size());

			startChunkHex += "\r\n";

			s_response_buff tempBuff;
			tempBuff.buffer.insert(tempBuff.buffer.end(), startChunkHex.begin(), startChunkHex.end());
			tempBuff.buffer.insert(tempBuff.buffer.end(), _responseBuffer.begin(), _responseBuffer.end());


			std::string endLine = "\r\n";

			tempBuff.buffer.insert(tempBuff.buffer.end(), endLine.begin(), endLine.end());
			tempBuff.currentIndex = 0;

			_cgiTargetResponse->pushNewResponseBuff(tempBuff);

			_responseBuffer.clear();

			return ;
		}
	}

	return ;
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
	//std::cout << "     HTTPCGI::processCGIOUTresponseBuffer()" << std::endl;

	parsingCGIOUTresponseHeader();

	validateCGIOUTresponse();

	sendToResponseBuffer();
}


void HttpCgi::readFromCGI(Socket* currentSocket, const epoll_event& epollEvent)
{
	if (cgiProcessData->status != CGI_PROCESS_RUNNING)
		return ;
	// use read() right?

	// this mean done
	try
	{
		if (epollEvent.events & EPOLLIN)
		{
			ssize_t	readAmount;
			readAmount = read(_cgiOutSocket->getSocketFD().getFd(), _readCgiBuffer.data(), HTTP_READ_FROM_CGI_BUFFER_SIZE);
			if (readAmount == 0)
			{
				Logger::log(LC_CONN_LOG, "CGI socket out (has nothing to read).#%d:: Disconnecting..", _cgiOutSocket->getSocketFD().getFd());
				_isFinishedRead = true;

				/* need to handle in case of still reading body of cgi*/
				if (httpCgiStatus == HTTPCGI_SENDING_RESPONSE_BUFFER)
				{
					/* problem may occur if the body type is content length*/
					if (_bodyData->isChunkBody == false)
					{
						/* what i could quickfix here is just append with '\n'*/
						std::string tempStr(_bodyData->bodySize, '\n');

						s_response_buff tempBuff;
						tempBuff.buffer.insert(tempBuff.buffer.end(), tempStr.begin(), tempStr.end());
						tempBuff.currentIndex = 0;

						_cgiTargetResponse->pushNewResponseBuff(tempBuff);
					}
					else
					{
						if (_bodyData->isManualChunk == true)
						{
							std::string tempChunkStr = "0\r\n\r\n";

							s_response_buff tempBuff;

							tempBuff.buffer.insert(tempBuff.buffer.end(), tempChunkStr.begin(), tempChunkStr.end());
							tempBuff.currentIndex = 0;
							_cgiTargetResponse->pushNewResponseBuff(tempBuff);
						}

					}

					/* would trigger as a finished*/
					httpCgiStatus = HTTPCGI_FINISHED;
				}


				std::string currentStatusString  = "           CURRENT STATUS: ";
				if (httpCgiStatus == HTTPCGI_NO_STATUS)
					currentStatusString += "HTTPCGI_NO_STATUS";
				else if (httpCgiStatus == HTTPCGI_SENDING_TO_CGI)
					currentStatusString += "HTTPCGI_SENDING_TO_CGI";
				else if (httpCgiStatus == HTTPCGI_READING_RESPONSE_HEADER)
					currentStatusString += "HTTPCGI_READING_RESPONSE_HEADER";
				else if (httpCgiStatus == HTTPCGI_VALIDATING_RESPONSE)
					currentStatusString += "HTTPCGI_VALIDATING_RESPONSE";
				else if (httpCgiStatus == HTTPCGI_SENDING_RESPONSE_BUFFER)
					currentStatusString += "HTTPCGI_SENDING_RESPONSE_BUFFER";
				else if (httpCgiStatus == HTTPCGI_FINISHED)
					currentStatusString += "HTTPCGI_FINISHED";
				else
					currentStatusString += "HTTPCGI_CLOSED_CGI";

				std::cout << currentStatusString << std::endl;

				if (httpCgiStatus == HTTPCGI_READING_RESPONSE_HEADER || httpCgiStatus == HTTPCGI_SENDING_TO_CGI)
				{
					httpCgiStatus = HTTPCGI_FINISHED;
					generate5xxCGIOUTresponseError(500, "Internal Error::CGI doesn\'t complete the response");
				}
				// something here that would remove this socket cleanly
			}
			else if (readAmount < 0)
			{
				//if (errno == EAGAIN || errno == EWOULDBLOCK)
				//	return ;

				//_keepConnection = false;
				//// something here that would remove this socket cleanly
				//return ;
			}
			else
			{
				// std::cout << "======== READ FROMC CGI RAW BUFFER READ FROM  =========" << '\n';
				// std::cout << std::string(_readCgiBuffer.data(), readAmount) << '\n';
				// std::cout << "=========================" << '\n';

				_responseBuffer.append(_readCgiBuffer.data(), readAmount);
				/* we can try the same method from http here. but the implementation
				is a bit different from Http class so it would be kinda the same but it's not
				*/
					processCGIOUTresponseBuffer();
			}
		}

		if ((epollEvent.events & EPOLLERR) || ((epollEvent.events & ~EPOLLIN) & EPOLLHUP))
		{
			if (httpCgiStatus == HTTPCGI_READING_RESPONSE_HEADER || httpCgiStatus == HTTPCGI_VALIDATING_RESPONSE)
			{
				/* here we can still throw 5xx response back to client */
				generate5xxCGIOUTresponseError(500, "Internal Error::Cgi Out::Doesn't complete the response");
			}

			else if (httpCgiStatus == HTTPCGI_SENDING_RESPONSE_BUFFER)
			{
				/* if it is content length then fatal error, but if manual transfer encoding then just 0\r\n\r\n */
				if (_bodyData->isChunkBody == false || (_bodyData->isChunkBody == true && _bodyData->isManualChunk == false))
				{
					/* tells to close client socket by using manual custom event */

					std::map<int, s_webserv_custom_event>& customEventMap = *_cgiOutSocket->getEventContoller().customEventMap;

					s_webserv_custom_event& targetCustomEvent = customEventMap[cgiProcessData->clientSocketPtr->getSocketFD().getFd()];

					targetCustomEvent.clientSocketManualDisconnect = true;
				}

				else if (_bodyData->isChunkBody == true && _bodyData->isManualChunk == true)
				{
					std::string tempChunkStr = "0\r\n\r\n";

					s_response_buff tempBuff;

					tempBuff.buffer.insert(tempBuff.buffer.end(), tempChunkStr.begin(), tempChunkStr.end());
					tempBuff.currentIndex = 0;
					_cgiTargetResponse->pushNewResponseBuff(tempBuff);
				}

				httpCgiStatus = HTTPCGI_FINISHED;
			}

		}
	}
	catch (HttpThrowStatus &e)
	{
		httpCgiStatus = HTTPCGI_FINISHED;
		Logger::log(LC_INFO, "Http::CgiSocket#%d::response with status code %d::%s", currentSocket->getSocketFD().getFd(), e.statusCode(), e.message().c_str());
	}
	catch (std::exception &e)
	{
		httpCgiStatus = HTTPCGI_FINISHED;
		Logger::log(LC_ERROR, "HttpCgi::Exception: %s", e.what());
	}
	catch (...)
	{

	}


	return ;
}

void HttpCgi::sendToCGI(Socket* currentSocket, const epoll_event& epollEvent)
{
	try
	{
		if (cgiProcessData->status != CGI_PROCESS_RUNNING)
			return ;

		if (httpCgiStatus != HTTPCGI_SENDING_TO_CGI)
			return ;
		
		if ( (epollEvent.events & ~EPOLLOUT) && ((epollEvent.events & EPOLLHUP) || (epollEvent.events & EPOLLERR)))
		{
			/* no EPOLLOUT, and some EPOLL event that we should end immediately*/

			if (epollEvent.events & EPOLLHUP)
				std::cout << "              CGIIN::EPOLLHUP" << std::endl;
			if (epollEvent.events & EPOLLERR)
				std::cout << "              CGIIN::EPOLLERR" << std::endl;
			if (epollEvent.events & EPOLLOUT)
				std::cout << "              CGIIN::EPOLLOUT" << std::endl;
			/**/
			generate5xxCGIOUTresponseError(500, "Internal Error::CGI IN:: epollEvent::failed");
		}

		/* append the buffer first */
		if (_writeCgiBuffer.size() < HTTP_WRITE_TO_CGI_BUFFER_SIZE && _tempFileData->isReachEOF == false)
		{
			size_t needToAppendSize = HTTP_WRITE_TO_CGI_BUFFER_SIZE - _writeCgiBuffer.size();

			std::vector<char> temp(HTTP_WRITE_TO_CGI_BUFFER_SIZE);


			while (needToAppendSize > 0)
			{
				ssize_t readAmount = read(_tempFileData->tempReadFileFd.getFd(), &temp[0], HTTP_WRITE_TO_CGI_BUFFER_SIZE);

				if (readAmount < 0)
				{
					if (errno == EINTR)
						continue;

					/* fatal error here */
					generate5xxCGIOUTresponseError(500, "Internal Error::CGI_IN::sendToCgi()::read from TempFile failed::" + std::string(std::strerror(errno)));
					break ;
				}
				else if (readAmount == 0)
				{
					_tempFileData->isReachEOF = true;
					break ;
				}
				else
				{
					/* */
					_writeCgiBuffer.insert(_writeCgiBuffer.end(), temp.begin(), temp.begin() + readAmount);

					/* deduct the needToAppenSize */
					if (needToAppendSize <= static_cast<size_t>(readAmount))
						needToAppendSize = 0;
					else
					{
						needToAppendSize = needToAppendSize - readAmount;
					}
				}
			}
		}


		/* here we read from the tempFd that we used */
		ssize_t	writeAmount = write(_cgiInSocket->getSocketFD().getFd(), &_writeCgiBuffer[0], _writeCgiBuffer.size());

		//static size_t count = 0;

		//std::cout << "sendToCGI_count: " << count++ << "   writeAmount: " << writeAmount << '\n';

		if (writeAmount < 0)
		{
			/* we've already handled through epoll event */
			return ;
		}

		/* erase the buffer by the amount of read */
		_writeCgiBuffer.erase(_writeCgiBuffer.begin(), _writeCgiBuffer.begin() + writeAmount);

		if (_writeCgiBuffer.size() == 0 && _tempFileData->isReachEOF == true)
		{
			/* if both buffer and tempFileData is empty, then it is finished and
			we can move to the next process */
			httpCgiStatus = HTTPCGI_READING_RESPONSE_HEADER;

			tempFileManager().removeTempFile(_tempFileData->tempFileNum);

			/* i think we can safely remove */
			_tempFileData.clear();
		}

	}
	catch (HttpThrowStatus &e)
	{
		httpCgiStatus = HTTPCGI_FINISHED;
		Logger::log(LC_INFO, "Http::CgiSocket#%d::response with status code %d::%s", currentSocket->getSocketFD().getFd(), e.statusCode(), e.message().c_str());
	}
	catch (std::exception &e)
	{
		httpCgiStatus = HTTPCGI_FINISHED;
		Logger::log(LC_ERROR, "HttpCgi::Exception: %s", e.what());
	}
	catch (...)
	{

	}

}

bool HttpCgi::isKeepConnection(const Socket* currentCgiSocket) const
{
	if (currentCgiSocket == NULL)
		throw WebservException("HttpCgi::isKeepConnection::cannot be NULL in argument");

	/* for cgi in socket */
	if (_cgiInSocket.hasData() == true && (*_cgiInSocket) == currentCgiSocket)
	{
		if (httpCgiStatus != HTTPCGI_SENDING_TO_CGI)
			return (false);
		else
			return (true);
	}

	/* this is for cgi out */
	if (currentCgiSocket == _cgiOutSocket)
	{
		if (_keepConnection == false)
			return (false);

		if (httpCgiStatus == HTTPCGI_CLOSED_CGI)
		{
			return (false);
		}
		else
		{
			return (true);
		}
	}

	return (false);
}

e_httpcgi_process_status& HttpCgi::status()
{
	return (httpCgiStatus);
}

void HttpCgi::processCGI(Socket* currentSocket, const epoll_event& epollEvent)
{
	if (cgiProcessData->clientSocketPtr.hasData() == false)
		httpCgiStatus = HTTPCGI_FINISHED;

	if (currentSocket == NULL)
		throw WebservException("HttpCgi::processCgi::don't accept NULL pointer");

	if (cgiProcessData->status != CGI_PROCESS_RUNNING)
	{
		if (cgiProcessData->status == CGI_PROCESS_NO_PROCESS)
			throw WebservException("HttpCgi::processCgi::CgiProcessData->status cannot be NO_STATUS");

		if (cgiProcessData->status == CGI_PROCESS_WAITING)
		{
			httpCgiStatus = HTTPCGI_FINISHED;
		}
	}

	if (httpCgiStatus != HTTPCGI_FINISHED)
	{
		if (currentSocket == _cgiOutSocket)
		{
			//std::cout << "    HTTPCGI::readFromCgi()  " << std::endl;
			readFromCGI(currentSocket, epollEvent);
		}
		else if (_cgiInSocket.hasData() == true && currentSocket == (*_cgiInSocket))
		{
			/* check with epoll_event first before write() */

			sendToCGI(currentSocket, epollEvent);
		}

		if (_clientResponseList->empty() == false && _clientResponseList->front().hasSomethingtoSend() == true)
		{
			epoll_event	event;
			std::memset(&event, 0, sizeof(event));
			event.events = EPOLLIN | EPOLLOUT;
			event.data.fd = cgiProcessData->clientSocketPtr->getSocketFD().getFd();

			if (epoll_ctl(_cgiOutSocket->getEpollFD().getFd(), EPOLL_CTL_MOD, cgiProcessData->clientSocketPtr->getSocketFD().getFd(), &event) == -1)
			{
				std::string msg = "HttpCgi::epoll_ctl()::error::";
				msg += std::strerror(errno);
				throw WebservException(msg);
			}
		}
	}

	if (httpCgiStatus == HTTPCGI_FINISHED)
	{
		if (_cgiInSocket.hasData() == true && (*_cgiInSocket) == currentSocket)
		{
			/* should announce to close socket in here */
			forceSigTerm();
			return ;
		}

		//std::cout << "      HTTPCGI_FINISHED   " << std::endl;
		/* remove the cgiout socket from epoll list entirely*/
		if (epoll_ctl(_cgiOutSocket->getEpollFD().getFd(), EPOLL_CTL_DEL, _cgiOutSocket->getSocketFD().getFd(), NULL) == -1)
		{
			_keepConnection = false;
		}

		/* here is for cgi out */
		if (cgiProcessData->status == CGI_PROCESS_RUNNING)
		{
			/* wanna gracefully exit by SIGTERM process */
			cgiProcessData->sigProcess(SIGTERM);
		}

		if (cgiProcessData->status == CGI_PROCESS_WAITING)
		{
			OptionalData<int> waitstatus = cgiProcessData->waitProcess();

			if (waitstatus.hasData() == true)
			{
				/* you can say the CGI process exit status here */
				cgiProcessData->status = CGI_PROCESS_FINISHED;
				httpCgiStatus = HTTPCGI_CLOSED_CGI;
			}
		}
	}

	return ;
}

void HttpCgi::forceSigTerm()
{
	if (cgiProcessData->status == CGI_PROCESS_RUNNING)
		cgiProcessData->sigProcess(SIGTERM);

	if (httpCgiStatus != HTTPCGI_FINISHED && httpCgiStatus != HTTPCGI_CLOSED_CGI)
	{
		try
		{
			if (httpCgiStatus != HTTPCGI_SENDING_RESPONSE_BUFFER)
				generate5xxCGIOUTresponseError(500, "Internal Error::CGI closed Timeout");
		}
		catch (HttpThrowStatus &e)
		{
			Logger::log(LC_INFO, "Http::CgiSocket#%d::response with status code %d::%s", _cgiOutSocket->getSocketFD().getFd(), e.statusCode(), e.message().c_str());
		}
		catch (std::exception &e)
		{
			Logger::log(LC_INFO, "CgiSocket#%d::Error Occurred when writing error response::%s", _cgiOutSocket->getSocketFD().getFd(), e.what());
		}
		catch (...)
		{
			Logger::log(LC_INFO, "CgiSocket#%d::unknown error", _cgiOutSocket->getSocketFD().getFd());
		}

			
		httpCgiStatus = HTTPCGI_FINISHED;

		OptionalData<int> waitStatus = cgiProcessData->waitProcess();

		if (waitStatus.hasData())
		{
			/* can tell the exit code of the process here*/
			httpCgiStatus = HTTPCGI_CLOSED_CGI;
		}
	}

	/* this function will make the httpCgiStatus either HTTPCGI_FINISHED or HTTPCGI_CLOSED_CGI */
}

CgiProcess& HttpCgi::getCgiProcess()
{
	return (*cgiProcessData);
}
