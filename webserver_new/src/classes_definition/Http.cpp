#include "../../include/classes/Http.hpp"
#include "../../include/classes/Socket.hpp"
#include "../../include/utility_function.hpp"
#include "../../include/classes/EnvpWrapper.hpp"
#include "../../include/classes/TempFileManager.hpp"

#include <cctype>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

Http::Http()
:
_clientSocketPtr(NULL),
_socketMapPtr(NULL),
_recvBuffer(HTTP_RECV_BUFFER),
_keepConnection(true),
_isEpollout(false)
{
}

Http::Http(Socket *clientSocketPtr, std::map<int, Socket>* socketMapPtr)
:
_clientSocketPtr(clientSocketPtr),
_socketMapPtr(socketMapPtr),
_recvBuffer(HTTP_RECV_BUFFER),
_keepConnection(true),
_isEpollout(false)
{
}

Http::Http(const Http& obj)
:
_clientSocketPtr(obj._clientSocketPtr),
_socketMapPtr(obj._socketMapPtr),
_recvBuffer(HTTP_RECV_BUFFER),
_keepConnection(true),
_isEpollout(false)
{
}

Http::~Http()
{
}

void Http::printHeaderField(HttpRequest& requestData) const
{
	if (requestData.processStatus != NO_STATUS
	&& requestData.processStatus != READING_HEADER
	&& requestData.processStatus != READING_REQUEST_LINE)
	{
		std::cout << "====================================" << std::endl;
		//print request line
		{
			std::cout << requestData.requestData.method << " " << requestData.requestData.requestTarget << " " << requestData.requestData.protocol << std::endl;
		}

		std::map<std::string, std::string>::const_iterator	it = requestData.requestData.headerField.begin();
		std::vector<std::string>::const_iterator vecIt;
		while (it != requestData.requestData.headerField.end())
		{
			std::cout << it->first << ": " << it->second << std::endl;
			++it;
		}
		std::cout << "====================================" << std::endl;
	}
}

bool Http::isKeepConnection() const
{
	return _keepConnection;
}

void Http::cgiChildProcessNoRequestBody(HttpRequest& requestData, int pipeForCgiStdOut[2])
{
	try 
	{
		TempFileManager().setIsChild(true);
		// for this particular
		// close the read end of pipe
		close(pipeForCgiStdOut[0]);

		// then dup2 to replace the stdout
		if (dup2(pipeForCgiStdOut[1], STDOUT_FILENO) == -1)
		{
			Logger::log(LC_DEBUG, "CGI_PROCESS dup2 failed::%s", std::strerror(errno));
			for (int i = 3; i < MAX_FD; i++)
				close(i);
			// cannot even sent the error message to parent proc
			throw int(42);
		}

		close(pipeForCgiStdOut[1]);
		close(STDIN_FILENO);


		// chdir()
		{
			size_t	pos = requestData.targetData.combinedPath.find_last_of('/');
			std::string scriptPathDir = requestData.targetData.combinedPath.substr(0, pos == std::string::npos ? requestData.targetData.combinedPath.size() : pos + 1);

			if (chdir(scriptPathDir.c_str()) != 0)
			{
				// just need to tell and may create error reponse
				// to output
				generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error::CGI chdir() failed");
			}

			/* change the PWD and OLDPWD */
			envData().assignEnv("OLDPWD", envData().findValue("PWD"));
			envData().assignEnv("PWD", scriptPathDir);
		}

		/* I don't wanna do the authrization so i just leave that*/
		/* if the header field contained "Authorization" then 
		we should pass that information to */
		// {
		// 	std::map<std::string, std::string>::const_iterator foundAuth = _headerField.find("authorization");
		// 	if (foundAuth != _headerField.end())
		// 	{
		// 		std::string tempstring;
		// 		if (httpFieldNormalSingletonTrim(foundAuth->second, tempstring) == false);
		// 			generate4xx5xxErrorReponse(400, false, "Internal Error::CGI::auth field building env failed");
				
		// 		envData().assignEnv("AUTH_TYPE", tempstring);
		// 	}
		// }

		/* The GATEWAY_INTERFACE variable MUST be set to the dialect of CGI being used by the server
		to communicate with the script. */
		envData().assignEnv("GATEWAY_INTERFACE", "CGI/1.1");

		{
			std::string tempString = in_addr_t_to_string(_clientSocketPtr->_client_addr_in);

			/* the REMOTE_ADDR variable MUST be set to the network address of the client sending the
			request to the server.*/
			envData().assignEnv("REMOTE_ADDR", tempString);

			/* For the REMOTE_HOST we need to reverse DNS lookup by using the addr of the client
			and get its actual domain name which ofc we cann't use those functions in this project
			however, the document state that we can just do the same value as REMOTE_ADDR */
			envData().assignEnv("REMOTE_HOST", tempString);
		}




		// then we would set up the environment here
		envData().assignEnv("REQUEST_METHOD", requestData.requestData.method);
		envData().assignEnv("QUERY_STRING", requestData.targetData.queryString);

		// GET method don't have body 
		// so skip CONTENT_LENGTH and CONTENT_TYPE

		envData().assignEnv("SCRIPT_NAME", requestData.cgiData.cgiScriptPath);
		if (!requestData.cgiData.cgiVirtualPath.empty())
		{
			envData().assignEnv("PATH_INFO", requestData.cgiData.cgiVirtualPath);
			envData().assignEnv("PATH_TRANSLATED", requestData.cgiData.cgiPathTranslated);
		}

		/* SERVER_NAME just take the host part in Host: from header field, i don't know
		if it is correct but the CGI documentation also state that we can use this so..*/
		envData().assignEnv("SERVER_NAME", requestData.serverData.serverName);

		/* SERVER_PORT is just put the port from URI Authority part here, which
		i already stored it*/
		envData().assignEnv("SERVER_PORT", requestData.serverData.portName);

		/* SERVER_PROTOCOL because we based from RFC9112 which is HTTP1.1 */
		envData().assignEnv("SERVER_PROTOCOL", "HTTP/1.1");

		/* SERVER_SOFTWARE is just our program which mean like webserv/1.0 looks good */
		envData().assignEnv("SERVER_SOFTWARE", "webserv/1.0");

		envData().assignEnv("REDIRECT_STATUS", "200");

		envData().assignEnv("SCRIPT_FILENAME", requestData.targetData.combinedPath);
		

		// convert the http header to env
		{
			std::map<std::string, std::string>::const_iterator headerIt = requestData.requestData.headerField.begin();
			std::string headerConvertedStr;

			while (headerIt != requestData.requestData.headerField.end())
			{
				// skip these header
				if (headerIt->first != "content-length"
				&& headerIt->first != "content-type"
				&& headerIt->first != "transfer-encoding"
				&& headerIt->first != "connection")
				{
					headerConvertedStr = "HTTP_" + headerIt->first;

					// convert to all capital letters and '-' to '_'
					for (size_t i = 0; i < headerConvertedStr.size(); i++)
					{
						if (headerConvertedStr[i] == '-')
							headerConvertedStr[i] = '_';
						else
						{
							headerConvertedStr[i] = static_cast<unsigned char>(std::toupper(headerConvertedStr[i]));
						}
					}

					// lastly assign it to env
					envData().assignEnv(headerConvertedStr, headerIt->second);
				}

				++headerIt;
			}

			// assume that we finish modifying env
			// what left if execve

			char* argv[3];
			argv[0] = const_cast<char *>(requestData.cgiData.cgiPath.c_str());
			argv[1] = const_cast<char *>(requestData.targetData.combinedPath.c_str());
			argv[2] = NULL;

			//int storeTemp = open("/home/muaykak/Coding/42webserv/webserver_new/StoreTemp.txt", O_CREAT | O_RDWR | O_TRUNC, 0644);

			//if (storeTemp == -1)
			//{
			//	Logger::log(LC_DEBUG, "StoreTemp fd open FAILED:: %s", std::strerror(errno));

			//}

			//if (dup2(storeTemp, STDOUT_FILENO) == -1)
			//{
			//	Logger::log(LC_DEBUG, "StoreTemp fd dup2 FAILED:: %s", std::strerror(errno));

			//}

			// then close all the unused fds
			//  leave only 0, 1, 2
			for (int i = 3; i < MAX_FD; i++)
				close(i);
			
			signal(SIGINT, SIG_DFL);
		    signal(SIGQUIT, SIG_DFL);
		    signal(SIGTERM, SIG_DFL);
		    signal(SIGPIPE, SIG_DFL);

			if (execve(argv[0], argv, envData().getEnvp()) == -1)
			{
				signal(SIGINT, serverStopHandler);
			    signal(SIGQUIT, serverStopHandler);
			    signal(SIGTERM, serverStopHandler);
			    signal(SIGPIPE, SIG_IGN);
				generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error:: GET to CGI:: failed execve()");
			}


			// should say something back to parent with reasons								
		}
	}
	catch (HttpThrowStatus &e)
	{
		HttpResponse& target = *httpRequest.responseTargetPtr;

		target.forcePrintAllResponse();
		close(STDOUT_FILENO);
		httpRequest.clear();
	}
	catch (int &e)
	{
		throw;
	}
	catch (...)
	{
		try 
		{
			generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error::Unknown Error Occurred");
		}
		catch (HttpThrowStatus &e)
		{
			HttpResponse& target = *httpRequest.responseTargetPtr;

			target.forcePrintAllResponse();
			close(STDOUT_FILENO);
			httpRequest.clear();
		}
	}

	throw int(1);
}

void	Http::parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize)
{
	if (httpRequest.processStatus == NO_STATUS)
		httpRequest.processStatus = READING_REQUEST_LINE;

	if (httpRequest.processStatus != READING_REQUEST_LINE)
		return ;

	/* we have to bind our request with response queue first */
	/* check if the request already bind with the response queue yet*/
	if (httpRequest.responseTargetPtr == NULL)
	{
		_httpResponseList.push_back(HttpResponse());
		httpRequest.responseTargetPtr = &_httpResponseList.back();
	}

	if (reqBuffSize > MAX_REQUEST_BUFFER_SIZE)
		generate4xx5xxErrorReponse(httpRequest, 400, false,"request buffer should not higher than MAX_REQUEST_BUFFER_SIZE");

	HttpRequest& httpReq = httpRequest;

	if (httpReq.requestData.method.empty())
	{

		// skip any empty line '\r\n"""
		while (currIndex + 1 < reqBuffSize){
			if (_requestBuffer[currIndex] == '\r') {
				// '\r' must always followed by '\n'
				if (_requestBuffer[currIndex + 1] != '\n')
					generate4xx5xxErrorReponse(httpRequest, 400, false, "the \'\\r\' must always followed by \'\\n\'");
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
		size_t	endLinePos = _requestBuffer.find("\r\n", currIndex);

		// doesn't find any endline delimiter
		// will process later
		if (endLinePos == _requestBuffer.npos){
			if (currIndex != 0)
				_requestBuffer = _requestBuffer.substr(currIndex);
			return ;
		}

		// find pos any whitespaces but not '\n'
		size_t	pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
		httpReq.requestData.method = pos == _requestBuffer.npos ? _requestBuffer.substr(currIndex) : _requestBuffer.substr(currIndex, pos - currIndex);

		if (httpReq.requestData.method.empty() || alphaAtoZ().isNotMatch(httpReq.requestData.method) == true)
			generate4xx5xxErrorReponse(httpRequest, 400, false, "the method must not empty. Or method must contain only A-Z in uppercase");

		// skip 1 pos which is the first whitespace
		currIndex = pos + 1;
		// should not reach endlinePos yet
		if (currIndex >= endLinePos)
			generate4xx5xxErrorReponse(httpRequest, 400, false, "bad spacing in request line");
		// now get the request target.
		pos = htabSp().findFirstCharset(_requestBuffer, currIndex, endLinePos - currIndex);
		// must can find next whitespace.. it IS the format
		if (pos == _requestBuffer.npos)
			generate4xx5xxErrorReponse(httpRequest, 400, false, "bad formating in request line");
		httpReq.requestData.requestTarget = _requestBuffer.substr(currIndex, pos - currIndex);

		// must not empty
		if (httpReq.requestData.requestTarget.empty())
			generate4xx5xxErrorReponse(httpRequest, 400, false, "request target in request line must not empty");

		// also check if contain any forbidden chars
		if (allowRequestTargetChar().isMatch(httpReq.requestData.requestTarget) == false)
			generate4xx5xxErrorReponse(httpRequest, 400, false, "request target must not contain any forbidden char");

		// then move our currIndex
		currIndex = pos + 1;
		// prevent edge case where now currIndex might right at the endLinepos
		// should not reach endlinePos yet
		if (currIndex >= endLinePos)
			generate4xx5xxErrorReponse(httpRequest, 400, false, "bad request line. the protocol version is missing");

		// now the last part is protocol version
		httpReq.requestData.protocol = _requestBuffer.substr(currIndex, endLinePos - currIndex);

		// check must not empty
		if (httpReq.requestData.protocol.empty())
			generate4xx5xxErrorReponse(httpRequest, 400, false, "request line protocol must not empty");
		// we get all the request line then we move to reading the header field
		currIndex = endLinePos + 2;
	}

	httpReq.processStatus = READING_HEADER;
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
	size_t	colonPos;

	while (httpRequest.processStatus == READING_HEADER)
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

			httpRequest.processStatus = VALIDATING_REQUEST;
			if (currIndex + 2 >= reqBuffSize)
				_requestBuffer.clear();
			else
				_requestBuffer.erase(0, currIndex + 2);
			currIndex = 0;
			//printHeaderField();
			break ;
		}


		// we found end line pos

		// must separate name and value by colon ':'
		colonPos = _requestBuffer.find_first_of(':', currIndex);
		// not separate by :  must reject
		if (colonPos == _requestBuffer.npos || colonPos >= endLinePos)
			generate4xx5xxErrorReponse(httpRequest, 400, false, "name and value in the header field must seperated by \':\'");
		// then get the field name in temp first
		tempFieldName = _requestBuffer.substr(currIndex, colonPos - currIndex);

		// must not empty
		if (tempFieldName.empty())
			generate4xx5xxErrorReponse(httpRequest, 400, false, "name in header field must not empty string");

		// We can check field name length here

		// simple checking that it must not contain any forbidden char
		{
			if (httpFieldNameChar().isMatch(tempFieldName) == false)
				generate4xx5xxErrorReponse(httpRequest, 400, false, "name in header field must not contain any forbidden char");
			
			/* first letter of field name must be an alpha ?*/
			if (allAlphaChar()[tempFieldName[0]] == false)
				generate4xx5xxErrorReponse(httpRequest, 400, false, "name in header field must start with a letter");
		}

		// now we got field name, next we want field value
		currIndex = colonPos + 1;
		tempSep = _requestBuffer.substr(currIndex, endLinePos - currIndex);
		tempSep = my_ft_trim(tempSep, " \t");

		if (!tempSep.empty())
		{
			if (forbiddenFieldValueChar().isNotMatch(tempSep) == false)
				generate4xx5xxErrorReponse(httpRequest, 400, false, "value in header field must not contain any forbidden char");
		}
		tempFieldName = stringToLower(tempFieldName);

		std::string &headerValueTarget = httpRequest.requestData.headerField[tempFieldName];

		// separated by the ", "
		if (!headerValueTarget.empty() && !tempSep.empty())
			headerValueTarget += ", ";
		
		headerValueTarget += tempSep;

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

void	Http::generate3xxRedirectResponse(HttpRequest& requestData, unsigned int statusCode)
{

	HttpResponse& targetResponse = *requestData.responseTargetPtr;

	targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

	// here normally you would
	// keep connection with 3xx response
	// but if it has body we need to read and discard
	// that body first
	targetResponse.keepAfterResponse = true;
	targetResponse.statusLine->first = statusCode;
	targetResponse.contentType = "text/html";

	// status message
	switch (statusCode)
	{
		case 301:
		{
			targetResponse.statusLine->second = "Moved Permanently";
			break;
		}
		case 302:
		{
			targetResponse.statusLine->second = "Found";
			break;
		}
		case 303:
		{
			targetResponse.statusLine->second = "See Other";
			break;
		}
		case 307:
		{
			targetResponse.statusLine->second = "Temporary Redirect";
			break;
		}
		case 308:
		{
			targetResponse.statusLine->second = "Permanent Redirect";
			break;
		}
		default:
		{
			break;
		}
	}

	std::string absoluteRedirectPath = "http://";
	{
		absoluteRedirectPath += requestData.targetData.authorityPart;
		absoluteRedirectPath += requestData.targetData.redirectPath;

		targetResponse.addHeader("Location", absoluteRedirectPath);
	}


	std::string tempHtml =
	"<html>\r\n"
	"<head><title>";

	tempHtml += targetResponse.statusLine->second;

	tempHtml +=
	"</title></head>\r\n"
	"<body><h1>";

	tempHtml += targetResponse.statusLine->second;

	tempHtml +=
	"</h1>\r\n<p>Redirect <a href=\"";

	tempHtml += absoluteRedirectPath;

	tempHtml +=
	"\">here</a>.</p>\r\n</body>\r\n</html>";


	targetResponse.fixedBodyStr = tempHtml;

	if (requestData.bodyData.body_type != 0)
	{
		requestData.processStatus = READ_BODY;
		requestData.bodyData.discardBody = true;
	}
	else
		requestData.processStatus = FINISHED_READ_BODY;

	//throw HttpThrowStatus(returnCode, (*return_redirect)[1]);

}

void	Http::generate4xx5xxErrorReponseChildProcess(HttpRequest& requestData, unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg)
{
	// first we check if there is already default error page
	bool	hasDefaultErrorPageFile = false;

	const std::vector<std::string>* foundErrorPageVec = NULL;
	if (requestData.serverData.targetServerPtr != NULL)
	{
		if (requestData.serverData.targetLocationBlockPtr != NULL)
		{
			foundErrorPageVec = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "error_page");
		}
		else
		{
			// found targetServer but not _target location
			foundErrorPageVec = requestData.serverData.targetServerPtr->getServerData("error_page");
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
	//FileDescriptor errorFileFD;
	size_t			fileSize = 0;
	Shared<std::ifstream> targetErrorFile;

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
						continue;
					}
					else
						break;
				}
				if (targetErrorFile->is_open() == true)
				{
					fileSize = fileStat.st_size;
					//errorFileFD = fd;
					hasDefaultErrorPageFile = true;
				}
			}
		}
	}

	HttpResponse& targetResponse = *requestData.responseTargetPtr;

	targetResponse.keepAfterResponse = keepAfterResponse;
	targetResponse.contentType = "text/html";

	targetResponse.addHeader("Status", toString(errorStatusCode));

	// set status message
	if (errorStatusCode == 405)
	{
		const std::vector<std::string>* foundAllowedMethodPtr = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "allowed_methods");

		for (size_t i = 0; i < foundAllowedMethodPtr->size(); i++)
		{
			targetResponse.addHeader("Allow", (*foundAllowedMethodPtr)[i]);
		}
	}

	std::string statusMessageTemp;
	{
		switch (errorStatusCode)
		{
			case (400):
			{
				statusMessageTemp = "Bad Request";
				break;
			}
			case (404):
			{
				statusMessageTemp = "Not Found";
				break;
			}
			case (500):
			{
				statusMessageTemp = "Internal Error";
				break;
			}
			case (403):
			{
				statusMessageTemp = "Forbidden";
				break;
			}
			case (405):
			{
				// the allowed method to the header
				{
					const std::vector<std::string>* foundAllowedMethodPtr = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "allowed_methods");

					for (size_t i = 0; i < foundAllowedMethodPtr->size(); i++)
					{
						targetResponse.addHeader("Allow", (*foundAllowedMethodPtr)[i]);
					}
				}

				statusMessageTemp = "Method Not Allowed";
				break;
			}
			case (417):
			{
				statusMessageTemp = "Expection Failed";
				break;
			}
			default:
			{
				statusMessageTemp = "";
				break;
			}

		}


	}

	if (hasDefaultErrorPageFile == true)
	{
		targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FILE;

		targetResponse.fileBody = targetErrorFile;

		targetResponse.fileSize = fileSize;
	}
	else
	{
		targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

		std::string htmlMsg = toString(errorStatusCode) + " " + statusMessageTemp;

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

		targetResponse.fixedBodyStr = bodyStr;
	}

	targetResponse.generateResponse();

	throw HttpThrowStatus(errorStatusCode, throwMsg);
}

void	Http::generate4xx5xxErrorReponse(HttpRequest& requestData, unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg)
{
	// first we check if there is already default error page
	bool	hasDefaultErrorPageFile = false;

	const std::vector<std::string>* foundErrorPageVec = NULL;
	if (requestData.serverData.targetServerPtr != NULL)
	{
		if (requestData.serverData.targetLocationBlockPtr != NULL)
		{
			foundErrorPageVec = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "error_page");
		}
		else
		{
			// found targetServer but not _target location
			foundErrorPageVec = requestData.serverData.targetServerPtr->getServerData("error_page");
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
				while (true)
				{
					targetErrorFile->open(errorPageFilePath.c_str());

					if (targetErrorFile->is_open())
						break ;
					
					if (errno == EINTR)
					{
						targetErrorFile->clear();
						continue;
					}
					else
						break;
				}
				//int fd = open(errorPageFilePath.c_str(), O_RDONLY);
				if (targetErrorFile->is_open() == true)
				{
					fileSize = fileStat.st_size;
					//errorFileFD = fd;
					hasDefaultErrorPageFile = true;
				}
			}
		}
	}

	HttpResponse& targetResponse = *requestData.responseTargetPtr;

	targetResponse.keepAfterResponse = keepAfterResponse;
	targetResponse.statusLine->first = errorStatusCode;
	targetResponse.contentType = "text/html";

	// set status message
	{
		switch (errorStatusCode)
		{
			case (400):
			{
				targetResponse.statusLine->second = "Bad Request";
				break;
			}
			case (404):
			{
				targetResponse.statusLine->second = "Not Found";
				break;
			}
			case (500):
			{
				targetResponse.statusLine->second = "Internal Error";
				break;
			}
			case (403):
			{
				targetResponse.statusLine->second = "Forbidden";
				break;
			}
			case (405):
			{
				// the allowed method to the header
				{
					const std::vector<std::string>* foundAllowedMethodPtr = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "allowed_methods");

					for (size_t i = 0; i < foundAllowedMethodPtr->size(); i++)
					{
						targetResponse.addHeader("Allow", (*foundAllowedMethodPtr)[i]);
					}
				}

				targetResponse.statusLine->second = "Method Not Allowed";
				break;
			}
			case (417):
			{
				targetResponse.statusLine->second = "Expection Failed";
				break;
			}
			default:
			{
				targetResponse.statusLine->second = "";
				break;
			}

		}


	}

	if (hasDefaultErrorPageFile == true)
	{
		targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FILE;

		targetResponse.fileBody = targetErrorFile;

		targetResponse.fileSize = fileSize;
	}
	else
	{
		targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

		std::string htmlMsg = toString(errorStatusCode) + " " + targetResponse.statusLine->second;

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

		targetResponse.fixedBodyStr = bodyStr;
	}

	targetResponse.generateResponse();

	throw HttpThrowStatus(errorStatusCode, throwMsg);
}

void	Http::validateRequestBufferSelectServer(HttpRequest& requestData, const Socket& clientSocket,const std::string& authStr)
{
	if (requestData.processStatus != VALIDATING_REQUEST)
		return ;
	printHeaderField(requestData);
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
				generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Authority::");

			portStr = authStr.substr(colonPos + 1);
			if (portStr.empty() || portStr.size() > 5 || digitChar().isMatch(portStr) == false)
				generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Authority::");
		}
		portNum = std::atoi(portStr.c_str());
	}

	// check port
	{
		// if the port is out of range we can define as
		// 400 bad request
		if (portNum > 65535)
		{
			generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Authority::Port is out of range");
		}

		// if the port is in the range but doesn't match with
		// the connection it was coming through
		// we can give 403 Forbidden because we are not
		// Proxy server
		if (portNum != clientSocket.getServerListenPort())
		{
			std::string msg = "Invalid::URI Authority::Port mismatch request_target:" + toString(portNum) + " server_port:" + toString(clientSocket.getServerListenPort());
			generate4xx5xxErrorReponse(requestData, 403, false, msg);
		}

		requestData.serverData.portName = toString(portNum);
	}

	// now we check the host, if it is ip or server_name (or whatever it's called)
	{

		// it is probably something like 172.233.21.34 some thing like that
		if (hostipChar().isMatch(host) == true)
		{
			in_addr_t	tempAddr;
			// conversion failed
			if (string_to_in_addr_t(host, tempAddr) == false)
				generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Authority::failed to convert IP");

			// correct syntax but should not allow, it is broadcast address
			if (tempAddr == 0xFFFFFFFF)
				generate4xx5xxErrorReponse(requestData, 403, false, "Invalid::URI Authority:: IP cannot be 255.255.255.255");

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
			//const std::set<in_addr_t>& serverIpHost = clientSocket.getServerIpHost();

			//if (serverIpHost.size() != 0)
			//{
			//	if (serverIpHost.find(tempAddr) == serverIpHost.end())
			//	{
			//		std::string msg = "Need fixing::Invalid::URI Authority::IP mismatch:: server_ip:" + in_addr_t_to_string(clientSocket._client_addr_in) + " request_target:" + in_addr_t_to_string(tempAddr);
			//		generate4xx5xxErrorReponse(403, false, msg);
			//	}
			//	else
			//	{

			//	}
			//}

			{
				const ServerConfig* fallback_server = NULL;

				const std::vector<ServerConfig>* ptr = clientSocket.getServersConfigPtr();
				std::vector<ServerConfig>::const_iterator serverVecIt = ptr->begin();
				const std::set<in_addr_t>* sethostipptr = NULL;

				requestData.serverData.targetServerPtr = NULL;
				while (requestData.serverData.targetServerPtr == NULL && serverVecIt != ptr->end())
				{
					sethostipptr = &serverVecIt->getHostIp();
					if (sethostipptr->size() == 0)
					{
						if (fallback_server == NULL)
							fallback_server = &(*serverVecIt);
					}
					else
					{
						// if found
						if (sethostipptr->find(tempAddr) != sethostipptr->end())
						{
							requestData.serverData.targetServerPtr = &(*serverVecIt);
							break;
						}
					}
					++serverVecIt;
				}

				if (requestData.serverData.targetServerPtr == NULL)
				{
					if (fallback_server)
						requestData.serverData.targetServerPtr = fallback_server;
					else
					{
						std::string msg = "Need fixing::Invalid::URI Authority::IP mismatch:: server_ip:" + in_addr_t_to_string(clientSocket._client_addr_in) + " request_target:" + in_addr_t_to_string(tempAddr);
						generate4xx5xxErrorReponse(requestData, 403, false, msg);
					}
				}

			}

			requestData.serverData.serverName = host;

			//if (tempAddr != clientSocket._client_addr_in)
			//{
			//	std::string msg = "Invalid::URI Authority::IP mismatch:: server_ip:" + in_addr_t_to_string(clientSocket._client_addr_in) + " request_target:" + in_addr_t_to_string(tempAddr);
			//	generate4xx5xxErrorReponse(403, false, msg);
			//}
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

			requestData.serverData.targetServerPtr = NULL;
			while (serverVecIt != ptr->end() && requestData.serverData.targetServerPtr == NULL)
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
							requestData.serverData.targetServerPtr = &(*serverVecIt);
							break ;
						}
						++tempNameVecIt;
					}
				}
				++serverVecIt;
			}
			if (requestData.serverData.targetServerPtr == NULL)
			{
				if (fallback_server)
					requestData.serverData.targetServerPtr = fallback_server;
				else
					requestData.serverData.targetServerPtr = &((*clientSocket.getServersConfigPtr())[0]);
			}

			requestData.serverData.serverName = host;
		}
	}

	requestData.targetData.authorityPart = authStr;
}

void Http::requestLineCheckProtocolVersion(HttpRequest& requestData)
{
	
	char maj;
	char min;
	// the string should be at least 8
	// HTTP/X.X
	// whether what version you are
	// must start with "HTTP/"
	if (requestData.requestData.protocol.size() != 8
	|| requestData.requestData.protocol.compare(0, 5, "HTTP/") != 0
	|| requestData.requestData.protocol[6] != '.')
		generate4xx5xxErrorReponse(requestData, 400, false, "ERROR::protocol version wrong format");

	maj = requestData.requestData.protocol[5];
	if (maj != '1')
	{
		if (maj >= '0' && maj <= '3')
		{
			// response with unsupported version
			generate4xx5xxErrorReponse(requestData, 505, false, "Error::version not supported");
		}
		else 
		{
			// some weird characters
			generate4xx5xxErrorReponse(requestData, 400, false, "ERROR::protocol version wrong format");
		}
	}

	min = requestData.requestData.protocol[7];
	if (min < '0' || min > '9')
	{
		// must be digit
		generate4xx5xxErrorReponse(requestData, 400, false, "ERROR::protocal version wrong format");
	}

	requestData.requestData.protocol.clear();
	requestData.requestData.protocol += maj;
	requestData.requestData.protocol += min;
	// finished processing protocol

}

void	Http::requestLineCheckRequestTargetAbsolute(HttpRequest& requestData, const Socket& clientSocket)
{
	//this request targen legth if in absolute form must
	// longer than  characters
	if (requestData.requestData.requestTarget.size() <= 7)
		generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Scheme::");


	// allow only this scheme
	if (requestData.requestData.requestTarget.compare(0, 7, "http://") != 0)
		generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Scheme::allowed only \"http://\"");

	// check the authority part
	{
		std::string authStr;

		size_t	pathPos = requestData.requestData.requestTarget.find_first_of('/', 7);
		// if cannot find then it is only /
		if (pathPos == requestData.requestData.requestTarget.npos)
			authStr = requestData.requestData.requestTarget.substr(7);
		else
			authStr = requestData.requestData.requestTarget.substr(7, pathPos - 7);

		// authority cannot empty
		if (authStr.empty())
			generate4xx5xxErrorReponse(requestData, 400, false, "Invalid::URI Scheme::");

		validateRequestBufferSelectServer(requestData, clientSocket, authStr);

		// now for authority part we check both host and ip
		// so now we can recreate _requestTarget string
		if (pathPos == requestData.requestData.requestTarget.npos)
			requestData.requestData.requestTarget = "/";
		else
			requestData.requestData.requestTarget = requestData.requestData.requestTarget.substr(pathPos);
	}

}

void	Http::requestLineCheckRequestTargetPathCheck(HttpRequest& requestData)
{

	// according to the standard, we must
	// separate path into segments separated by
	// the '/' as the delimitter first

	std::list<std::string> splitList;
	splitString(requestData.targetData.targetPath, "/", splitList);

	////split vector should not empty
	//if (splitList.size() == 0)
	//	generate4xx5xxErrorReponse(400, false, "Bad Path. Or my bad parser lol");

	bool isEndwithSlash = requestData.targetData.targetPath[requestData.targetData.targetPath.size() - 1] == '/' ? true : false;

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
				generate4xx5xxErrorReponse(requestData, 400, false, "request target::'%' encoding invalid::");
			}

			// we proved this string is valid path name
			// continue to next one
			++splitListIt;
		}
	}

	//finished processing the list
	// now we combined that list back to string
	requestData.targetData.targetPath.clear();
	splitListIt = splitList.begin();
	size_t	newSize = 0;
	while (splitListIt != splitList.end())
	{
		newSize += splitListIt->size() + 1;
		++splitListIt;
	}
	if (isEndwithSlash == true)
		requestData.targetData.targetPath.reserve(newSize + 1);
	else
		requestData.targetData.targetPath.reserve(newSize);
	splitListIt = splitList.begin();
	while (splitListIt != splitList.end())
	{
		requestData.targetData.targetPath += '/';
		requestData.targetData.targetPath += *splitListIt;
		++splitListIt;
	}
	if (isEndwithSlash == true)
		requestData.targetData.targetPath.append("/");
}

void	Http::requestLineCheckRequestTarget(HttpRequest& requestData, const Socket& clientSocket)
{
	// check the first character to see if it is
	// origin form or absolute form
	{
		// if first character is not '/'
		// then it might be absolute form
		// we need to check that scheme
		if (requestData.requestData.requestTarget[0] != '/')
			requestLineCheckRequestTargetAbsolute(requestData, clientSocket);
		else
		{
		/*
			In case of target that starts with '/' 
			meaning that it is URI origin form
			in this case we need to check the host:
			in header
		*/
			std::string fieldValue;

			if (httpFieldNormalSingletonTrim(requestData.requestData.headerField.find("host")->second, fieldValue) == false)
				generate4xx5xxErrorReponse(requestData, 400, false, "Http:Invalid field value: host is invalid");

			validateRequestBufferSelectServer(requestData, clientSocket, fieldValue);
		}

	}

	// separate query and path
	{
		size_t	pos = requestData.requestData.requestTarget.find_first_of('?');
		if (pos == requestData.requestData.requestTarget.npos)
			requestData.targetData.targetPath = requestData.requestData.requestTarget;

		else
		{
			requestData.targetData.targetPath = requestData.requestData.requestTarget.substr(0, pos);
			if (pos < requestData.requestData.requestTarget.size() - 1)
				requestData.targetData.queryString = requestData.requestData.requestTarget.substr(pos + 1);
		}
	}

	// this is to processing the path.
	requestLineCheckRequestTargetPathCheck(requestData);

	// path process done, now 
}

void	Http::requestLineCheck(HttpRequest& requestData, const Socket& clientSocket)
{
	// rough check for method first
	{
		if (requestData.requestData.method != "GET" && requestData.requestData.method != "POST" && requestData.requestData.method != "DELETE")
			generate4xx5xxErrorReponse(requestData, 501, false, "Http::Method not implemented: " + requestData.requestData.method);
	}

	// before anything, we check the protocol version first
	requestLineCheckProtocolVersion(requestData);

	// whether what we check, if host is missing from
	// header field, the server should not accept it.
	if (requestData.requestData.headerField.find("host") == requestData.requestData.headerField.end())
	{
		// treated as bad request
		generate4xx5xxErrorReponse(requestData, 400, false, "Bad request::cannot find \'host\' in header field");
	}

	// we need to check the 'request target' first
	// this is the tedious process
	requestLineCheckRequestTarget(requestData, clientSocket);

}

void	Http::targetLocationBlockGet(HttpRequest& requestData)
{
	// we should have target server now
	if (requestData.serverData.targetServerPtr == NULL)
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error:: _targetServer Not Found");


	requestData.serverData.targetLocationBlockPtr = requestData.serverData.targetServerPtr->findLocationBlock(requestData.targetData.targetPath);

	// must not be null also
	if (requestData.serverData.targetLocationBlockPtr == NULL)
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error:: _targetServer Not Found");

}

void	Http::checkRequestBodyType(HttpRequest& requestData)
{

	// check the client_max_body_size of the server block if exist first
	const std::vector<std::string>* clientMaxBodySizePtr = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "client_max_body_size");
	if (clientMaxBodySizePtr == NULL || clientMaxBodySizePtr->size() != 1)
	{
		// treat as internal error
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::cannot find client_max_body_size, Or it has no matching element");
	}

	// 
	std::map<std::string, std::string>::const_iterator content_length = requestData.requestData.headerField.find("content-length");
	std::map<std::string, std::string>::const_iterator tranfer_encoding = requestData.requestData.headerField.find("transfer-encoding");

	// I will not allow DELETE or GET method to have body
	if (requestData.requestData.method != "POST" && (tranfer_encoding != requestData.requestData.headerField.end() || content_length != requestData.requestData.headerField.end()))
	{
		generate4xx5xxErrorReponse(requestData, 400, false, "Bad request:: DELETE or GET method must not contain body");
	}

	if (tranfer_encoding != requestData.requestData.headerField.end() && content_length != requestData.requestData.headerField.end())
		generate4xx5xxErrorReponse(requestData, 400, false, "Bad request:: Transfer-Encoding and Content-Length cannot both exist in header");

	// check Expect the body
	{
		std::map<std::string, std::string>::const_iterator foundExpect = requestData.requestData.headerField.find("expect");
		
		// if found
		if (foundExpect != requestData.requestData.headerField.end())
		{
			std::string targetValue;
			if (httpFieldNormalSingletonTrim(foundExpect->second, targetValue) == false)
				generate4xx5xxErrorReponse(requestData, 417, false, "Http:: Expect: wrong value :: expects only \"100-continue\"");


			if (targetValue != "100-continue")
				generate4xx5xxErrorReponse(requestData, 417, false, "Http:: Expect: wrong value :: expects only \"100-continue\"");

			requestData.bodyData.checkExpectBody = true;
		}
	}

	// if Content-Length is found, check if it exceeds the client_max_body_size
	if (content_length != requestData.requestData.headerField.end())
	{

		std::string valueString;
		if (httpFieldNormalSingletonTrim(content_length->second, valueString) == false)
		{
			generate4xx5xxErrorReponse(requestData, 400, false, "Bad request:: Content-Length:: invalid value");
		}

		// must contain only digit
		if (digitChar().isMatch(valueString) == false)
			generate4xx5xxErrorReponse(requestData, 400, false, "Bad request:: Content-Length:: value must be numeric characters (0-9)");


		// conversion to number with myfunction
		if (string_to_size_t(valueString, requestData.bodyData.body_size) == false)
			generate4xx5xxErrorReponse(requestData, 400, false, "Bad request:: Content-Length::exceeds size_t value or something");

		// type 1 is body size that specified by Content-Length
		requestData.bodyData.body_type = 1;

		// now check the body size if it exceeds the client_max_body_size
		{
			const std::vector<std::string>* client_max_body_size_ptr = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "client_max_body_size");

			// internal error
			if (client_max_body_size_ptr == NULL || client_max_body_size_ptr->size() != 1 || digitChar().isMatch((*client_max_body_size_ptr)[0]) == false)
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::cannot find client_max_body_size value, or the value is not correct");

			requestData.bodyData.client_max_body_size = 0;
			if (string_to_size_t((*client_max_body_size_ptr)[0], requestData.bodyData.client_max_body_size) == false)
			{
				// cannot torelate because no boundary to check client request
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error:: client_max_body_size value conversion failed");
			}
			
			if (requestData.bodyData.body_size > requestData.bodyData.client_max_body_size)
				generate4xx5xxErrorReponse(requestData, 413, false, "HTTP::request Content-Length is Larger than client_max_body_size");

			// check the 
			
			requestData.bodyData.readBody = true;
		}
	}
	else if (tranfer_encoding != requestData.requestData.headerField.end())
	{
		// here need to check the Transfer Encoding

		std::vector<std::string> valueVec;

		if (httpFieldNormalCommaElement(tranfer_encoding->second, valueVec) == false)
			generate4xx5xxErrorReponse(requestData, 400, false, "Http::request Transfer-Encoding:: invalid value");

		// mean that chunked is not found in the list
		bool isFoundChunked = false;
		for (size_t i = 0; i < valueVec.size(); i++)
		{
			if (valueVec[i] == "chunked")
				isFoundChunked = true;
		}

		if (isFoundChunked == false)
		{
			std::string msg = "Http::request Transfer-Encoding:: \"chunked\" not found::key=";
			
			std::vector<std::string>::const_iterator it = valueVec.begin();
			while (it != valueVec.end())
			{
				if (it != valueVec.begin())
					msg += ", ";
				msg += *it;
				++it;
			}

			generate4xx5xxErrorReponse(requestData, 400, false, msg);
		}

		/* for chunked encoding, need to check trailer */
		{
			std::map<std::string, std::string>::const_iterator foundTrailer = requestData.requestData.headerField.find("trailer");
			if (foundTrailer != requestData.requestData.headerField.end())
			{
				/* check the element */
				
				std::vector<std::string> outVec;
				if (httpFieldNormalCommaElement(foundTrailer->second, outVec) == false)
					generate4xx5xxErrorReponse(requestData, 400, false, "Http:: Transfer-Encoding:: Trailer field invalid");

				if (outVec.size() < 1)
					generate4xx5xxErrorReponse(requestData, 400, false, "Http:: Transfer-Encoding:: Trailer field invalid");

				// iterate and check
				for (size_t i = 0; i < outVec.size(); i++)
				{
					if (httpFieldNameChar().isMatch(outVec[i]) == false || allAlphaChar()[outVec[i][0]] == false)
						generate4xx5xxErrorReponse(requestData, 400, false, "Http:: Transfer-Encoding:: Trailer field invalid");
					else
						requestData.bodyData.trailerHeader.insert(stringToLower(outVec[i]));
				}
				requestData.bodyData.chunkedBodyHasTrailerHeader = true;
			}
			else
			{
				requestData.bodyData.chunkedBodyHasTrailerHeader = false;
			}
		}

		// chunked found then this is chunked encoding.
		requestData.bodyData.body_type = 2;
		requestData.bodyData.body_size = 0;
		requestData.bodyData.readBody = true;
	}
	// both Content-Length and Transfer-Encoding not found
	else
	{
		requestData.bodyData.body_type = 0;
		requestData.bodyData.body_size = 0;
		requestData.bodyData.readBody = false;
	}
}

bool	Http::checkRedirection(HttpRequest& requestData)
{
	// check for 'return' in location block
	const std::vector<std::string>* return_redirect = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "return");
	// if found then we need to redirect
	if (return_redirect != NULL)
	{
		// it must have 2 elements or treat as internal error
		if (return_redirect->size() != 2)
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error:: 'return' redirect in config file must have 2 elements");
		
		const std::string& returnCodeStr = (*return_redirect)[0];

		size_t	returnCode = 0;
		// the string size must be 3 digit and dont start with '0'
		if (returnCodeStr.size() != 3 || string_to_size_t(returnCodeStr, returnCode) == false || returnCode < 300 || returnCode > 399)
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::config file::redirect contains wrong status code, or wrong format idk");
			
		if (returnCode >= 300 && returnCode < 400)
		{
			requestData.targetData.redirectPath = (*return_redirect)[1];
			generate3xxRedirectResponse(requestData, returnCode);
			return (true);
		}
		else
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::config file::redirect contain status code that is not 3xx");
	}

	return (false);
}

void	Http::checkMethod(HttpRequest& requestData)
{
	// _method = _method of this current request

	// allowed_methods in the targeted block of the config file
	const std::vector<std::string>* allowMethodPtr = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "allowed_methods");

	if (allowMethodPtr == NULL)
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::allowed_methods not found in the targeted location block");
	
	bool found = false;
	for (size_t i = 0; i < allowMethodPtr->size(); i++)
	{
		if (requestData.requestData.method == (*allowMethodPtr)[i])
		{
			found = true;
			break ;
		}
	}

	// the method is not allowed in this location block
	// return 405 Not Implemented
	if (found == false)
	{
		generate4xx5xxErrorReponse(requestData, 405, false, "Http::method::not implemented::" + requestData.requestData.method);
	}
}



void	Http::checkCgiPath(HttpRequest& requestData)
{

	const std::vector<std::string>* foundCgiPass = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "cgi_pass");
	// if found then we check
	if (foundCgiPass != NULL)
	{
		if (foundCgiPass->size() < 2 || foundCgiPass->size() % 2 != 0)
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CgiPass on this location block is misconfigured");

		// convert into temporary map
		std::map<std::string, std::string>	tempCgiPassMap;
		{
			size_t i = 0;
			while (i < foundCgiPass->size())
			{
				if ((*foundCgiPass)[i][0] != '.')
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CgiPass on this location block is misconfigured");
				
				tempCgiPassMap[(*foundCgiPass)[i]] = (*foundCgiPass)[i + 1];

				i += 2;
			}
		}

		std::string tempPath = (*requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "location_name"))[0];
		// devide into segment

		std::string tempTofil = requestData.targetData.targetPath.substr(tempPath.size());
		std::cout << "tempToFill: " << tempTofil << std::endl;
		size_t	slashPos;
		size_t	dotPos;
		std::string tempExtensionStr;
		std::map<std::string, std::string>::iterator it;
		while (!tempTofil.empty())
		{
			slashPos = tempTofil.find_first_of('/');
			if (slashPos == tempTofil.npos)
			{
				tempPath += tempTofil;
				tempTofil.clear();
			}
			else
			{
				tempPath += tempTofil.substr(0, slashPos);
				tempTofil.erase(0, slashPos + 1);
			}

			dotPos = tempPath.find_last_of('.');
			if (dotPos == tempPath.npos)
			{
				tempPath += '/';
				continue;
			}
			else 
			{
				tempExtensionStr = tempPath.substr(dotPos);
				// check if found mathing 
				it = tempCgiPassMap.find(tempExtensionStr);
				// not found then continue
				if (it == tempCgiPassMap.end())
				{
					tempPath += '/';
					continue;
				}
				else
				{
					requestData.cgiData.cgiPath = it->second;
					requestData.cgiData.cgiScriptPath = tempPath;
					Logger::log(LC_RES_NOK_LOG, "cgiScriptPath = %s", tempPath.c_str());
					requestData.cgiData.cgiVirtualPath = tempTofil;
					break ;
				}
			}

		}

	}

	/* access() is a good practice to check the cgi_path() first
		because forking new process and turns out the cgi_path is not working
		will cost a lot more, leave that error for only rare cases*/
	
	if (requestData.cgiData.cgiPath.empty())
		return ;


	// now we know if the request target is CGI or not by
	// checking _cgiPath.empty()
	struct stat fileStat;
	std::memset(&fileStat, 0, sizeof(fileStat));
	if (stat(requestData.cgiData.cgiPath.c_str(), &fileStat) != 0)
	{
		std::string ErrMsg = "Http::stat()::cgi_path " + requestData.targetData.targetPath + "::";
		ErrMsg += strerror(errno);
		if (errno == EACCES)
			generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
		generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
	}

	if (S_ISREG(fileStat.st_mode) == false)
	{
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::cgi path is invalid");
	}
	
	/* now we can check with access() function, like, F_OK and X_OK because were gonna execute with
		this path right?*/

	if (access(requestData.cgiData.cgiPath.c_str(), X_OK) != 0)
	{
		std::string ErrMsg = "Http::access()::cgi_path " + requestData.targetData.targetPath + "::";
		ErrMsg += strerror(errno);
		if (errno == EACCES)
			generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
		generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
	}
}

void Http::createSystemPath(HttpRequest& requestData, std::string& systemPath)
{
	if (requestData.requestData.method == "POST" && requestData.cgiData.cgiPath.empty())
	{



		// should use 'upload_store' may be we can give this as a must in configuration file ??
		// try with this method first
		const std::vector<std::string>* foundUploadStore = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "upload_store");

		/*
		i'm not sure but now i'm assume that if you upload a file 
		via post method, you must explicitly define 'upload_store' in that location
		block in configuration file	
		*/
		if (foundUploadStore == NULL)
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::using POST to upload file to server requires \'upload_store\' to be defined in configuration file");

		// just for sure checking
		if (foundUploadStore->size() != 1 || (*foundUploadStore)[0].empty())
		{
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::invalid \'root\' value");
		}

		requestData.targetData.uploadStorePath = (*foundUploadStore)[0];
		systemPath = (*foundUploadStore)[0];
	}
	else // GET or DELETE will use root always
	{

		// take 'root' as main path

		const std::vector<std::string>* foundRoot = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "root");

		// it should not be null but if so then treat as internal error
		if (foundRoot == NULL)
		{
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::require 'root' in this location block");
		}

		// just for sure checking
		if (foundRoot->size() != 1 || (*foundRoot)[0].empty())
		{
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::invalid \'root\' value");
		}

		systemPath = (*foundRoot)[0];
	}

	/* here if the systempath is relative path, we need to append it with the
	this program current directory to make it absolute path */
	{
		/* if it doesn't start with '/' then it is relative path */
		if (systemPath[0] != '/')
		{
			/* append it with PWD environment variable */

			std::string pwdString = envData().findValue("PWD");

			if (!pwdString.empty())
			{
				systemPath = pwdString + '/' + systemPath;
			}
		}
	}
}

void Http::handleDeleteRequest(HttpRequest& requestData)
{
	if (std::remove(requestData.targetData.combinedPath.c_str()) != 0)
	{
		if (errno == ENOENT)
			generate4xx5xxErrorReponse(requestData, 404, false, "Http::DELETE method::file not found");
		else if (errno == EACCES)
			generate4xx5xxErrorReponse(requestData, 403, false, "Http::DELETE method::permission denied");
		else
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::DELETE method::std::remove()::failed::" + std::string(strerror(errno)));
		
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
			HttpResponse& targetResponse = *requestData.responseTargetPtr;

			targetResponse.responseBodyType = HTTP_RESPONSE_NOBODY;
			targetResponse.keepAfterResponse = true;
			targetResponse.statusLine->first = 204;
			targetResponse.statusLine->second = "No Content";

			targetResponse.generateResponse();
			requestData.processStatus = FINISHED_READ_BODY;
			return ;
		}
	}
}

void	Http::handleGetRequest(HttpRequest& requestData, bool isEndWithSlash, const Socket& clientSocket, std::map<int, Socket>& socketMap)
{

	/*
	GET	method need to check if the path
	i wants is exist and whether it is
	a directory or not
	*/
	struct stat fileStat;
	std::memset(&fileStat, 0, sizeof(fileStat));
	if (stat(requestData.targetData.combinedPath.c_str(), &fileStat) != 0)
	{
		std::string ErrMsg = "Http::stat()::target_path " + requestData.targetData.targetPath + "::";
		ErrMsg += strerror(errno);
		if (errno == EACCES)
			generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
		generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
	}

	if (S_ISDIR(fileStat.st_mode))
	{
		// check if it target ends with '/' or not
		if (isEndWithSlash == true)
		{
			// check
			// check for directory listing (auto index)

			const std::vector<std::string>* foundAutoIndex = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "autoindex");

			// if not found then should return 403 forbidden or not found
			if (foundAutoIndex == NULL)
				generate4xx5xxErrorReponse(requestData, 403, true, "Http::\"autoindex\" is not found in this block.");

			if (foundAutoIndex->size() != 1)
				generate4xx5xxErrorReponse(requestData, 403, true, "Http::\"autoindex\" invalid value");

			if ((*foundAutoIndex)[0] == "on")
			{
				{

					DIR *dirPtr = opendir(requestData.targetData.combinedPath.c_str());

					if (dirPtr == NULL)
					{
						if (errno == EACCES)
							generate4xx5xxErrorReponse(requestData, 403, true, "Http::autoindex::opendir()::" + std::string(std::strerror(errno)));
						if (errno == ENOENT)
							generate4xx5xxErrorReponse(requestData, 404, true, "Http::autoindex::opendir()::" + std::string(std::strerror(errno)));
						else
							generate4xx5xxErrorReponse(requestData, 500, true, "Http::autoindex::opendir()::" + std::string(std::strerror(errno)));
					}

					/* bool is that it is a directory or not, second is the path */
					std::list<std::string> directoryNameList;
					dirent *tempDirStructPtr = readdir(dirPtr);
					while (tempDirStructPtr != NULL)
					{
						std::string tempFileName(tempDirStructPtr->d_name);

						if (tempFileName != ".")
						{
							struct stat fileStat;
							std::memset(&fileStat, 0, sizeof(fileStat));
							if (stat(tempFileName.c_str(), &fileStat) == 0)
							{
								if (S_ISDIR(fileStat.st_mode))
									tempFileName += '/';
							}

							directoryNameList.push_back(tempFileName);
						}
						tempDirStructPtr = readdir(dirPtr);
					}

					/* we finish doing with directory */
					closedir(dirPtr);


					/* now we create the html file */
					std::string tempBodyString;
					{
						tempBodyString = "<!DOCTYPE html>\r\n"
						"<html>\r\n"
						"<head>\r\n"
						"	<title>Index of ";
						tempBodyString += requestData.targetData.targetPath;
						tempBodyString += "</title>\r\n"
						"</head>\r\n"
						"<body>\r\n"
						"	<h1>Index of ";
						tempBodyString += requestData.targetData.targetPath;
						tempBodyString += "</h1>\r\n";

						if (directoryNameList.empty())
						{
							tempBodyString += "<h2>No file in this directory</h2>\r\n";
						}
						else
						{
							tempBodyString += "<hr>\r\n"
							"<ul>\r\n";

							std::list<std::string>::const_iterator listIt = directoryNameList.begin();
							while (listIt != directoryNameList.end())
							{
								tempBodyString += "<li><a href=\"";
								tempBodyString += *listIt;
								tempBodyString += "\">";
								tempBodyString += *listIt;
								tempBodyString += "</a></li>\r\n";

								++listIt;
							}

							tempBodyString += "</ul>\r\n"
							"</hr>\r\n";
						}

						tempBodyString += "</body>\r\n</html>\r\n";
					}

					HttpResponse& targetResponse = *requestData.responseTargetPtr;

					targetResponse.statusLine->first = 200;
					targetResponse.statusLine->second = "OK";
					targetResponse.contentType = "text/html";
					targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;
					targetResponse.fixedBodyStr = tempBodyString;

					requestData.processStatus = FINISHED_READ_BODY;
					targetResponse.generateResponse();
					return ;
				}

				// generate auto indexing 
				generate4xx5xxErrorReponse(requestData, 500, true, "Http::autoindex is allowed but Not implemented yet");
			}
			else
				generate4xx5xxErrorReponse(requestData, 403, true, "Http::\"autoindex\" is not on for directory listing");
		
		}
		else
		{
			// make redirections
			requestData.targetData.redirectPath = requestData.targetData.targetPath + '/';
			generate3xxRedirectResponse(requestData, 301);
			return ;

		}
	}

	// check if the file is regular file. must 
	// be regular file, right ?
	if (S_ISREG(fileStat.st_mode))
	{
		// GET will not have body because i want it that way
		
		// check if it is CGI or not
		if (requestData.cgiData.cgiPath.empty())
		{

			// try to open the targeted file
			//int fd = open(requestData.targetData.combinedPath.c_str(), O_RDONLY);

			Shared<std::ifstream> targetFile;

			while (true)
			{
				targetFile->open(requestData.targetData.combinedPath.c_str());

				if (targetFile->is_open())
					break ;
				
				if (errno == EINTR)
				{
					targetFile->clear();
					continue;
				}
				else
					break;
			}


			// if failed should response accordingly
			if (targetFile->is_open() == false)
			{
				if (errno == EACCES)
					generate4xx5xxErrorReponse(requestData, 403, false, "Http::GET to regular file failed::open() failed");

				else if (errno == EMFILE || errno == ENFILE)
				{
					generate4xx5xxErrorReponse(requestData, 500, false, "Http:: GET to regular file:: fd limit is reached");
				}
				else if (errno == ENOENT)
				{
					generate4xx5xxErrorReponse(requestData, 404, false, "Http:: Get to regular file:: file is missing");
				}
				// some unknown error
				else
					generate4xx5xxErrorReponse(requestData, 500, false, "Http:: Get to regular file::Internal Unknown Error");
			}

			//FileDescriptor tempFd = fd;

			HttpResponse& targetResponse = *requestData.responseTargetPtr;

			// set Last-Modified
			{
				// the st_mtim is time_t

				tm * timeGmt = std::gmtime(&fileStat.st_mtim.tv_sec);

				std::vector<char> tempTimeBuffer(100);

				std::strftime(&tempTimeBuffer[0], tempTimeBuffer.size(), "%a, %d %b %Y %H:%M:%S GMT", timeGmt);

				std::string tempModTime(tempTimeBuffer.data());

				targetResponse.addHeader("Last-Modified", tempModTime);
			}

			targetResponse.contentType = contentTypeTable().extensionToContentType(requestData.targetData.combinedPath);
			targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FILE;
			targetResponse.fileBody = targetFile;
			targetResponse.fileSize = fileStat.st_size;
			targetResponse.keepAfterResponse = true;
			targetResponse.statusLine->first = 200;
			targetResponse.statusLine->second = "OK";

			requestData.processStatus = FINISHED_READ_BODY;
			targetResponse.generateResponse();

			return ;
		}
		else
		{
			// pipe that let cgi write down and us to readstd::map<int, Socket>& socketMap
			int pipe_out[2];

			if (pipe(pipe_out) != 0)
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI:: pipe() failed::" + std::string(std::strerror(errno)));

			pid_t	pid = fork();

			if (pid < 0)
			{
				close(pipe_out[0]);
				close(pipe_out[1]);
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI:: fork() failed::" + std::string(std::strerror(errno)));
			}

			// child proc
			else if (pid == 0)
			{
				cgiChildProcessNoRequestBody(requestData, pipe_out);
			}
			else
			{
				close(pipe_out[1]);
				/*


				*/

				if (fcntl(pipe_out[0], F_SETFL, O_NONBLOCK) != 0)
				{
					// stop the process immediately
					kill(pid, SIGTERM);
					waitpid(pid, NULL, 0);
					close(pipe_out[0]);
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI::fcntl() error to set O_NONBLOCK");
				}

				// clear
				try
				{
					HttpResponse& targetResponse = *requestData.responseTargetPtr;


					Shared<HttpCgi> tempSharedHttpCgi;

					socketMap.insert(std::make_pair(pipe_out[0], Socket(pipe_out[0])));
					socketMap[pipe_out[0]].setupCGIOUTSocket(clientSocket.getServersConfigPtr(),
					clientSocket.getEventContoller(), &socketMap, tempSharedHttpCgi);

					Shared<CgiProcess> tempCgiProcess;

					tempCgiProcess->cgiPid = pid;
					tempCgiProcess->status = CGI_PROCESS_RUNNING;
					//tempCgiProcess->cgiOutSocketPtr = &socketMap[pipe_out[0]];
					tempCgiProcess->clientSocketPtr = _clientSocketPtr;
					tempCgiProcess->socketMapPtr = &socketMap;

					tempSharedHttpCgi->setHttpCgiNoCgiIn(&_httpResponseList,
						httpRequest.responseTargetPtr,
						&(socketMap[pipe_out[0]]),
						tempCgiProcess);


					targetResponse.cgiProcessData = tempCgiProcess;
					targetResponse.socketMapPtr = &socketMap;
					targetResponse.targetServer = requestData.serverData.targetServerPtr;
					targetResponse.targetLocationBlock = requestData.serverData.targetLocationBlockPtr;

					targetResponse.httpRequestData = requestData;

				}
				catch (...)
				{
					close(pipe_out[0]);
					throw;
				}

				// _isCgiOutSocketAlive = true;
				// _cgiOutSocket = pipe_out[0];
				// _isCgiProcessOpen = true;
				// _cgiProcessPid = pid;

			}

			requestData.processStatus = FINISHED_READ_BODY;
			return ;

			// GET to CGI didn't build yet so
			//generate4xx5xxErrorReponse(500, false, "Not CGI yet");
		}
	}
	else 
	{
		generate4xx5xxErrorReponse(requestData, 403, false, "target is not directory or regular file");
	}
}

void	Http::validateRequestData(HttpRequest& requestData, const Socket& clientSocket, std::map<int, Socket>& socketMap)
{

	if (requestData.processStatus != VALIDATING_REQUEST)
		return ;

	// checking the request line
	requestLineCheck(requestData, clientSocket);

	// get the targeted location block
	targetLocationBlockGet(requestData);

	/*
	check if found Transfer-Encoding or Content-Length
		if both are found , simply return error,
		we treat this as an error, however, another
		way to implement this is if Both are found
		'Transfer-Encoding' takes priority
	*/
	checkRequestBodyType(requestData);

	// check for redirection
	if (checkRedirection(requestData) == true)
		return ;

	/* check Connection: header */
	{
		/* if omitted, assume to keep alive*/
		std::map<std::string, std::string>::const_iterator foundConnection = requestData.requestData.headerField.find("connection");
		if (foundConnection != requestData.requestData.headerField.end())
		{
			/* check the string */
			std::string tempString;

			if (httpFieldNormalSingletonTrim(foundConnection->second, tempString) == false)
				generate4xx5xxErrorReponse(requestData, 400, false, "Bad Request::Invalid Connection Header");
			
			if (tempString == "close")
				httpRequest.responseTargetPtr->keepAfterResponse = false;
			else if (tempString == "keep-alive")
				httpRequest.responseTargetPtr->keepAfterResponse = true;
			else
				generate4xx5xxErrorReponse(requestData, 400, false, "Bad Request::Invalid Connection Header");

		}
		else
		{
			httpRequest.responseTargetPtr->keepAfterResponse = true;
		}
	}

	// check the method
	// return 405 if method is not allowed
	checkMethod(requestData);

	bool isEndWithSlash = requestData.targetData.targetPath[requestData.targetData.targetPath.size() - 1] == '/' ? true : false;

	if (requestData.requestData.method != "DELETE")
	{

		// should not delete file with 'index' seems to make sense to me
		if (isEndWithSlash == true)
		{
			// if the path end with slash
			// try to find the 'index' in that location block
			
			const std::vector<std::string>* foundIndex = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "index");

			// only append if found
			if (foundIndex)
			{
				if (foundIndex->size() != 1)
				{
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error:: invalid \'index\' directive in location block");
				}

				requestData.targetData.targetPath += (*foundIndex)[0];
				isEndWithSlash = false;
			}
		}

	}

	// need to check first if this location block
	// has the cgi_pass
	checkCgiPath(requestData);

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
		createSystemPath(requestData, systemPath);

		// we need to combine root of location block
		// with the URI that we resolved
		{
			const std::vector<std::string>* foundLocationName = requestData.serverData.targetServerPtr->getLocationData(requestData.serverData.targetLocationBlockPtr, "location_name");



			if (!requestData.cgiData.cgiPath.empty())
			{
				if (!requestData.cgiData.cgiVirtualPath.empty())
					requestData.cgiData.cgiPathTranslated = systemPath + requestData.cgiData.cgiVirtualPath;
				requestData.targetData.combinedPath = systemPath + requestData.cgiData.cgiScriptPath;
			}
			else
			{
				requestData.targetData.cutPath = requestData.targetData.targetPath.substr((*foundLocationName)[0].size());
				if (requestData.targetData.cutPath.empty() || requestData.targetData.cutPath[0] != '/')
					requestData.targetData.cutPath.insert(requestData.targetData.cutPath.begin(), '/');

				requestData.targetData.combinedPath = systemPath + requestData.targetData.cutPath;
			}

		}

		std::cout << "combined path: \"" << requestData.targetData.combinedPath << '\"' << std::endl;


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

		if (requestData.requestData.method == "DELETE")
		{
			handleDeleteRequest(requestData);
			return ;
		}
		else if (requestData.requestData.method == "GET")
		{
			handleGetRequest(requestData, isEndWithSlash, clientSocket, socketMap);
			return ;
		}
		// for POST method just leave it as error for now
		else
		{

			//struct stat fileStat;
			//std::memset(&fileStat, 0, sizeof(fileStat));
			//if (stat(requestData.targetData.combinedPath.c_str(), &fileStat) != 0)
			//{
			//	std::string ErrMsg = "Http::stat()::target_path " + requestData.targetData.targetPath + "::";
			//	ErrMsg += strerror(errno);
			//	if (errno == EACCES)
			//		generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
			//	generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
			//}

			if (requestData.cgiData.cgiPath.empty())
			{
				// need to check content type
				// get the _bodyContentType
				{
					std::map<std::string, std::string>::const_iterator foundContentTypeElement = requestData.requestData.headerField.find("content-type");

					if (foundContentTypeElement == requestData.requestData.headerField.end())
					{
						generate4xx5xxErrorReponse(requestData, 400, false, "The request contains body, but no Content-Type found in the request header");
					}

					const std::string& contentTypeString = foundContentTypeElement->second;

					// we need to extract the string
					std::pair<std::string, std::vector<std::pair<std::string, std::string> > > outPair;
					if (httpFieldContentTypeExtract(contentTypeString, outPair) == false)
						generate4xx5xxErrorReponse(requestData, 400, false, "Content-Type::invalid value");

					if (outPair.first == "multipart/form-data")
					{
						if (outPair.second.size() < 1)
							generate4xx5xxErrorReponse(requestData, 400, false, "Content-Type: multipart/form-data should have at least 1 attribute");
						
						std::pair<std::string, std::string>* foundBoundary = NULL;

						for (size_t i = 0; i < outPair.second.size(); i++)
						{
							if (outPair.second[i].first == "boundary")
							{
								if (foundBoundary == NULL)
									foundBoundary = &outPair.second[i];
								else
									generate4xx5xxErrorReponse(requestData, 400, false, "Content-Type:: multipart/form-data:: multiple boundary are not supposed to have");
							}
						}

						if (foundBoundary == NULL)
							generate4xx5xxErrorReponse(requestData, 400, false, "Content-Type:: multipart/form-data:: boundary not found");

						if (foundBoundary->second.size() < 1 || foundBoundary->second.size() > 70)
							generate4xx5xxErrorReponse(requestData, 400, false, "Content-Type:: boundary string size invalid");

						requestData.bodyData.bodyContentType = outPair.first;
						requestData.bodyData.multiformData->boundaryString = foundBoundary->second;

						struct stat fileStat;
						std::memset(&fileStat, 0, sizeof(fileStat));
						if (stat(requestData.targetData.combinedPath.c_str(), &fileStat) != 0)
						{
							std::string ErrMsg = "Http::stat()::target_path " + requestData.targetData.targetPath + "::";
							ErrMsg += strerror(errno);
							if (errno == EACCES)
								generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
							generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
						}

						if (S_ISDIR(fileStat.st_mode) != true)
						{
							/*
								Here, I would give the  403 error, the reason is that
								because multipart/form-data might contain information that 
								we may need to store in to the directory, therefore the 
								target route that the request targeted to must be a directory

								I assumed
							*/
							generate4xx5xxErrorReponse(requestData, 403, false, "the request targeted route for POST request with Content-Type as multipart/form-data must be a directory");
						}

						requestData.bodyData.tempRequestBodyFileNum = tempFileManager().generateNewTempFile();

						// open the file for writing now
						{
							std::string tempFilePath = TEMP_FILE_DIR + toString(requestData.bodyData.tempRequestBodyFileNum);

							int fd;
							while (true)
							{
								fd = open(tempFilePath.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);

								if (fd >= 0)
									break ;
								if (errno == EINTR)
									continue ;
								break;
							}


							if (fd < 0)
							{
								generate4xx5xxErrorReponse(requestData, 500, false, "POST method to non-cgi with Content-Type: multipart/form-data::open() error");
							}

							FileDescriptor tempFd(fd);
							//requestData.bodyData.bodyFd.push_back(fd);
							requestData.bodyData.writeBodyFile = tempFd;
						}


						// some how here you need to determine how to read the body part of this specific content type
						requestData.bodyData.isUseTempFile = true;
						requestData.bodyData.readBody = true;
						requestData.bodyData.discardBody = false;

						if (requestData.bodyData.checkExpectBody == true)
						{
							HttpResponse& target = *requestData.responseTargetPtr;

							s_response_buff	tempBuff;

							std::string expectResponse = "HTTP/1.1 100 Continue\r\n\r\n";

							tempBuff.currentIndex = 0;
							tempBuff.buffer.insert(tempBuff.buffer.end(), expectResponse.begin(), expectResponse.end());

							target.pushNewResponseBuff(tempBuff);
						}
					
					}
					else if (outPair.first == "application/octet-stream")
					{
						/*
							Don't need to care about file extension of the request target route
							for me, the best check is to just simply
							open() with the target path
						*/

						//int fd = open(requestData.targetData.combinedPath.c_str(), O_TRUNC | O_CREAT | O_RDWR);
						int fd;
						while (true)
						{
							fd = open(requestData.targetData.combinedPath.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);

							if (fd >= 0)
								break ;
							if (errno == EINTR)
								continue ;
							break;
						}


						if (fd < 0)
						{
							// i would insider this as internal error ?
							generate4xx5xxErrorReponse(requestData, 500, false, "POST method to non-cgi with Content-Type: application/octet-stream::open() error");
						}

						FileDescriptor tempFd(fd);

						//requestData.bodyData.bodyFd.push_back(fd);
						requestData.bodyData.writeBodyFile = tempFd;

						requestData.bodyData.isUseTempFile = false;
						requestData.bodyData.readBody = true;
						requestData.bodyData.discardBody = false;

						if (requestData.bodyData.checkExpectBody == true)
						{
							HttpResponse& target = *requestData.responseTargetPtr;

							s_response_buff	tempBuff;

							std::string expectResponse = "HTTP/1.1 100 Continue\r\n\r\n";

							tempBuff.currentIndex = 0;
							tempBuff.buffer.insert(tempBuff.buffer.end(), expectResponse.begin(), expectResponse.end());

							target.pushNewResponseBuff(tempBuff);
						}

					}
					else
					{
						const std::set<std::string>& foundExtensionTable = contentTypeTable().contentTypeToExtension(outPair.first);
						// here is the allowed Content-Type and we need to check corresponding file extension

						if (foundExtensionTable.size() != 0)
						{
							//
							std::string routeExtension;
							{
								size_t pos = requestData.targetData.combinedPath.find_last_of('.');

								if (pos != std::string::npos && pos + 1 < requestData.targetData.combinedPath.size())
								{
									routeExtension = requestData.targetData.combinedPath.substr(pos + 1);
								}
							}

							if (routeExtension.empty())
							{
								generate4xx5xxErrorReponse(requestData, 403, false, "POST to non-cgi::the Content-Type and the target file extension are not matched");
							}

							if (foundExtensionTable.find(routeExtension) != foundExtensionTable.end())
							{
								//int fd = open(requestData.targetData.combinedPath.c_str(), O_TRUNC | O_CREAT | O_RDWR);

								//struct stat fileStat;
								//std::memset(&fileStat, 0, sizeof(fileStat));
								//if (stat(requestData.targetData.combinedPath.c_str(), &fileStat) != 0)
								//{
								//	std::string ErrMsg = "Http::stat()::target_path " + requestData.targetData.targetPath + "::";
								//	ErrMsg += strerror(errno);
								//	if (errno == EACCES)
								//		generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
								//	generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
								//}

								int fd;
								while (true)
								{
									fd = open(requestData.targetData.combinedPath.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);

									if (fd >= 0)
										break ;
									if (errno == EINTR)
										continue ;
									break;
								}

								if (fd < 0)
								{
									// i would insider this as internal error ?
									generate4xx5xxErrorReponse(requestData, 500, false, "POST method to non-cgi with Content-Type: " + outPair.first + "::open() failed");
								}

								FileDescriptor tempFd(fd);
							
								//requestData.bodyData.bodyFd.push_back(fd);
								requestData.bodyData.writeBodyFile = tempFd;

								requestData.bodyData.isUseTempFile = false;
								requestData.bodyData.readBody = true;
								requestData.bodyData.discardBody = false;
							
								if (requestData.bodyData.checkExpectBody == true)
								{
									HttpResponse& target = *requestData.responseTargetPtr;

									s_response_buff	tempBuff;

									std::string expectResponse = "HTTP/1.1 100 Continue\r\n\r\n";

									tempBuff.currentIndex = 0;
									tempBuff.buffer.insert(tempBuff.buffer.end(), expectResponse.begin(), expectResponse.end());

									target.pushNewResponseBuff(tempBuff);
								}

							}
							else
							{
								generate4xx5xxErrorReponse(requestData, 403, false, "POST to non-cgi::the Content-Type and the target file extension are not matched");
							}

						}
						else
						{
							// create response for un supported media type
							HttpResponse& target = *requestData.responseTargetPtr;

							target.responseBodyType = HTTP_RESPONSE_NOBODY;
							target.statusLine->first = 415;
							target.keepAfterResponse = false;
							target.statusLine->second = "Unsupported Media Type";

							// get all the content type to the response header
							{
								const std::map<std::string, std::set<std::string> >& contentTypetoExtentionMap = contentTypeTable().getContentTypetoExtensionMap();
								std::map<std::string, std::set<std::string> >::const_iterator it = contentTypetoExtentionMap.begin();

								while (it != contentTypetoExtentionMap.end())
								{
									target.addHeader("Accept-Post", it->first);
									++it;
								}
							}

							target.generateResponse();
							throw HttpThrowStatus(415, "Unsupported Media Type");
						}


					}
				}

				requestData.processStatus = READ_BODY;
				return ;
			}
			else
			{

				struct stat fileStat;
				std::memset(&fileStat, 0, sizeof(fileStat));
				if (stat(requestData.targetData.combinedPath.c_str(), &fileStat) != 0)
				{
					std::string ErrMsg = "Http::stat()::target_path " + requestData.targetData.targetPath + "::";
					ErrMsg += strerror(errno);
					if (errno == EACCES)
						generate4xx5xxErrorReponse(requestData, 403, true, ErrMsg);
					generate4xx5xxErrorReponse(requestData, 404, true, ErrMsg);
				}
				/* need to read the body first before starting the CGI */

				/* access() is a good practice before doing anything big because
				forking new process() take a lot of memory*/

				/* we need to check the _combinedPath 
					- it must be a regular file, and readable */
				if (S_ISREG(fileStat.st_mode) == false)
				{
					generate4xx5xxErrorReponse(requestData, 400, false,"The cgi script path must be a regular file");
				}

				/*now access*/
				if (access(requestData.targetData.combinedPath.c_str(), R_OK) != 0)
				{
					std::string throwMsg = "Error:: cgi script path access()::" + std::string(std::strerror(errno));
					if (errno == EACCES)
						generate4xx5xxErrorReponse(requestData, 403, false, throwMsg);
					generate4xx5xxErrorReponse(requestData, 404, false, throwMsg);
				}

				/* now we can do the thing*/

				requestData.bodyData.tempRequestBodyFileNum = tempFileManager().generateNewTempFile();

				// open the file for writing now
				{
					std::string tempFilePath = TEMP_FILE_DIR + toString(requestData.bodyData.tempRequestBodyFileNum);

					//int fd = open(tempFilePath.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
					int fd;
					while (true)
					{
						fd = open(tempFilePath.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);

						if (fd >= 0)
							break ;
						if (errno == EINTR)
							continue ;
						break;
					}

					if (fd < 0)
					{
						generate4xx5xxErrorReponse(requestData, 500, false, "HTTP::validating::POST method to non-cgi with Content-Type: multipart/form-data::open() error::" + std::string(std::strerror(errno)));
					}

					FileDescriptor tempFd(fd);

					//requestData.bodyData.bodyFd.push_back(fd);
					requestData.bodyData.writeBodyFile = tempFd;
				}

				requestData.bodyData.isUseTempFile = true;
				requestData.bodyData.readBody = true;
				requestData.bodyData.discardBody = false;

				if (requestData.bodyData.checkExpectBody == true)
				{
					HttpResponse& target = *requestData.responseTargetPtr;

					s_response_buff	tempBuff;

					std::string expectResponse = "HTTP/1.1 100 Continue\r\n\r\n";

					tempBuff.currentIndex = 0;
					tempBuff.buffer.insert(tempBuff.buffer.end(), expectResponse.begin(), expectResponse.end());

					target.pushNewResponseBuff(tempBuff);
				}

				requestData.processStatus = READ_BODY;
				return ;

				/* we need to read to temporary file first for checking?*/

				//int pipe_in[2];
				//int	pipe_out[2];

				//if (pipe(pipe_in) != 0)
				//	generate4xx5xxErrorReponse(500, false, "Internal Error::CGI:: pipe() failed::" + std::string(std::strerror(errno)));
				//if (pipe(pipe_out) != 0)
				//{
				//	close(pipe_in[0]);
				//	close(pipe_in[1]);
				//	generate4xx5xxErrorReponse(500, false, "Internal Error::CGI:: pipe() failed::" + std::string(std::strerror(errno)));
				//}

				//pid_t pid = fork();

				//if (pid < 0)
				//{
				//	close(pipe_in[0]);
				//	close(pipe_in[1]);
				//	close(pipe_out[0]);
				//	close(pipe_out[1]);

				//	generate4xx5xxErrorReponse(500, false, "Internal Error::CGI:: fork() failed::" + std::string(std::strerror(errno)));
				//}

				//else if (pid == 0)
				//{
				//	try
				//	{
				//		close(pipe_in[1]);
				//		close(pipe_out[0]);

				//		if (dup2(pipe_out[1], STDOUT_FILENO) != 0)
				//		{
				//			for (int i = 3; i < MAX_FD; i++)
				//				close(i);

				//			throw(42);
				//		}

				//		if (dup2(pipe_in[0], STDIN_FILENO) != 0)
				//		{
				//			for (int i = 3; i < MAX_FD; i++)
				//				close(i);

				//			generate4xx5xxErrorReponse(500, false, "Internal Error:: dup2 to stdin failed");
				//		}

				//		for (int i = 3; i < MAX_FD; i++)
				//			close(i);
						
				//		{

				//		}
				//	}
				//	catch (HttpThrowStatus &e)
				//	{
				//		// should generate response on the list
				//		HttpResponse& target = _httpResponseList.back();

				//		// here print all of that to STDOUT
				//		target.forcePrintAllResponse();

				//		throw int(1);
				//	}
				//	catch (int &e)
				//	{
				//		throw ;
				//	}
				//	catch (...)
				//	{
				//		throw int(1);
				//	}

				//}
				//else
				//{
				//	close(pipe_out[1]);
				//	close(pipe_in[0]);
				//}


				generate4xx5xxErrorReponse(requestData, 500, false, "Post to CGI didn't finished yet");
			}
		}

	}




}

void Http::readingRequestBody(HttpRequest& requestData)
{
	// read the body using
	if (requestData.processStatus != READ_BODY)
		return ;

	if (requestData.bodyData.body_type == 0)
	{
		requestData.processStatus = FINISHED_READ_BODY;
		return ;
	}
	else if (requestData.bodyData.body_type == 1)
	{

		size_t	readBodyAmount;
		{
			// calculate the read body amount here
		
			// the size of _requestBuffer is the most the the body can read right now
		
			// content-length body type has _body_size which specified its fixed size
			readBodyAmount = requestData.bodyData.body_size - requestData.bodyData.curr_body_read;
		
			// then if the readBody amount is more than the _requestBuffer.size() then
			if (readBodyAmount > _requestBuffer.size())
			{
				readBodyAmount = _requestBuffer.size();
			}
		}
	
		if (requestData.bodyData.discardBody == true)
		{
			// we just discard from the buffer by that calculated amount
			_requestBuffer.erase(0, readBodyAmount);
			requestData.bodyData.curr_body_read += readBodyAmount;
			
			if (requestData.bodyData.curr_body_read == requestData.bodyData.body_size)
				requestData.processStatus = FINISHED_READ_BODY;
		
			return ;
		}
		else
		{
			// here we need to perform the write() operation
			while (true)
			{
				ssize_t	writeAmount = write(requestData.bodyData.writeBodyFile->getFd(), &_requestBuffer[0], readBodyAmount);
				if (writeAmount < 0)
				{
					if (errno == EINTR)
						continue;
					else
					{
						// try to correctly remove file if write operation is failed 
						requestData.bodyData.writeBodyFile.clear();
						if (requestData.bodyData.isUseTempFile)
							tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
						else
							std::remove(requestData.targetData.combinedPath.c_str());
					
						generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::write() operation failed during reading the request body");
					}
				}
				else
				{
					_requestBuffer.erase(0, writeAmount);
					requestData.bodyData.curr_body_read += writeAmount;
					break ;
				}
			}
		
			if (requestData.bodyData.curr_body_read == requestData.bodyData.body_size)
				requestData.processStatus = FINISHED_READ_BODY;
			return ;
		}

	}
	else if (requestData.bodyData.body_type == 2)
	{
		size_t	endLinePos;

		while (true)
		{
			if (requestData.bodyData.chunkedBodyIsFinished == true)
			{
				// we also skipped the first \r\n from 0\r\n
				// but normal chunked with no trailer header
				// is 0\r\n\r\n
				endLinePos = _requestBuffer.find("\r\n");

				if (endLinePos == std::string::npos)
				{
					if (requestData.bodyData.chunkedBodyHasTrailerHeader == true)
					{
						/*
						though here we have to wait the buffer to find next \r\n
						but should also have limit of how much length should a line be

						we have MAX_HTTP_HEADER_LINE_LENGTH to protect now !

						though the MAX_HTTP_HEADER_LINE_LENGTH has size
						if not found the endLinePos can also mean that
						the buffer string now contained only \r

						*/
						if (_requestBuffer.size() > MAX_HTTP_HEADER_LINE_LENGTH + 1)
						{
							requestData.bodyData.writeBodyFile.clear();
							if (requestData.bodyData.isUseTempFile)
								tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
							else
								std::remove(requestData.targetData.combinedPath.c_str());

							generate4xx5xxErrorReponse(requestData, 403, false, "Reading request body::chunked::trailer header:: line too long");
						}
						else
						{
							// so we wait for next round
							return ;
						}
					}
					else
					{
						/*
							For no trailer header, the \r\n would need to
							close to 0\r\n so ican only be 0\r\n\r\n

							So if the request buffer is longer or equal 2
							but you haven't found i would throw 400 error
						*/

						if (_requestBuffer.size() >= 2)
						{
							requestData.bodyData.writeBodyFile.clear();
							if (requestData.bodyData.isUseTempFile)
								tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
							else
								std::remove(requestData.targetData.combinedPath.c_str());
							
							generate4xx5xxErrorReponse(requestData, 400, false, "Reading request body::chunked::no trailer header::i would say wrong ending for this chunked request. It should be \"0\\r\\n\\r\\n\"");

						}
						else
						{
							// here just wait for next buffer next round;
							return ;
						}

					}
				}
				else
				{
					/* here is where we found the endLinePos 
					*/

					if (requestData.bodyData.chunkedBodyHasTrailerHeader)
					{
						if (endLinePos == 0)
						{
							requestData.processStatus = FINISHED_READ_BODY;
							return ;
						}
						else
						{
							std::string headerLineStr = _requestBuffer.substr(0, endLinePos);

							// check the limit
							if (headerLineStr.empty() || headerLineStr.size() > MAX_HTTP_HEADER_LINE_LENGTH)
							{
								requestData.bodyData.writeBodyFile.clear();
								if (requestData.bodyData.isUseTempFile)
									tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
								else
									std::remove(requestData.targetData.combinedPath.c_str());
								
								generate4xx5xxErrorReponse(requestData, 400, false, "Reading request body::chunked::trailer header line size wrong");
							}

							size_t colonPos = headerLineStr.find_first_of(':');
							if (colonPos == std::string::npos)
							{
								requestData.bodyData.writeBodyFile.clear();
								if (requestData.bodyData.isUseTempFile)
									tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
								else
									std::remove(requestData.targetData.combinedPath.c_str());

								generate4xx5xxErrorReponse(requestData, 400, false, "chunked trailer header::name and value in the header field must seperated by \':\'");
							}

							std::string tempFieldName = headerLineStr.substr(0, colonPos);

							if (tempFieldName.empty())
							{
								requestData.bodyData.writeBodyFile.clear();
								if (requestData.bodyData.isUseTempFile)
									tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
								else
									std::remove(requestData.targetData.combinedPath.c_str());

								generate4xx5xxErrorReponse(requestData, 400, false, "chunked trailer header::name in header field must not empty string");
							}

							if (httpFieldNameChar().isMatch(tempFieldName) == false || allAlphaChar()[tempFieldName[0]] == false)
								generate4xx5xxErrorReponse(requestData, 400, false, "name in header field must not contain any forbidden char");

							/* we must check first if the header name matched the Trailer: header */
							{
								std::set<std::string>::const_iterator foundHeader = requestData.bodyData.trailerHeader.find(stringToLower(tempFieldName));
								if (foundHeader == requestData.bodyData.trailerHeader.end())
								{
									// if not match then would error
									requestData.bodyData.writeBodyFile.clear();
									if (requestData.bodyData.isUseTempFile)
										tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
									else
										std::remove(requestData.targetData.combinedPath.c_str());

									generate4xx5xxErrorReponse(requestData, 400, false, "chunked trailer header::the header is not matched");
								}
							}

							std::string tempSep = headerLineStr.substr(colonPos + 1);
							tempSep = my_ft_trim(tempSep, " \t");

							if (!tempSep.empty())
							{
								if (forbiddenFieldValueChar().isNotMatch(tempSep) == false)
									generate4xx5xxErrorReponse(httpRequest, 400, false, "value in trailer header field must not contain any forbidden char");
							}


							tempFieldName = stringToLower(tempFieldName);

							std::string &headerValueTarget = httpRequest.requestData.headerField[tempFieldName];

							// separated by the ", "
							if (headerValueTarget.empty() == false)
								headerValueTarget += ", ";

							headerValueTarget += tempSep;

							tempSep.clear();
						}
					}
					else
					{
						/*
							if we found the \r\n, for no trailer chunked request.
							it must be on position 0	
						*/
						if (endLinePos == 0)
						{
							requestData.processStatus = FINISHED_READ_BODY;
							return ;
						}
						else
						{
							requestData.bodyData.writeBodyFile.clear();
							if (requestData.bodyData.isUseTempFile)
								tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
							else
								std::remove(requestData.targetData.combinedPath.c_str());
							
							generate4xx5xxErrorReponse(requestData, 400, false, "Reading request body::chunked::no trailer header::i would say wrong ending for this chunked request. It should be \"0\\r\\n\\r\\n\"");
						}
					}
				}
			}
			else
			{
				if (requestData.bodyData.curr_body_read >= requestData.bodyData.body_size)
				{
					endLinePos = _requestBuffer.find("\r\n");

					if (endLinePos == std::string::npos)
					{
						// maybe next read
						return ;
					}
					std::string hexString = _requestBuffer.substr(0, endLinePos);
				
					size_t hexNum = 0;
				
					// convert that to size_t
					if (hex_to_size_t(hexString, hexNum) == false)
					{
						// wrong chunked format, blame the client with 4xx error
					
						requestData.bodyData.writeBodyFile.clear();
						if (requestData.bodyData.isUseTempFile)
							tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
						else
							std::remove(requestData.targetData.combinedPath.c_str());
					
						generate4xx5xxErrorReponse(requestData, 400, false, "Request Body Chunked::wrong hex format");
					}
				
					// now we have hexnum converted
					/*
						next is to check if it will exceed the _client_max_body_size	
					*/
					{
						// both number must not exceed the _client_max_body_size
						if (requestData.bodyData.body_size > requestData.bodyData.client_max_body_size || hexNum > requestData.bodyData.client_max_body_size)
						{
							requestData.bodyData.writeBodyFile.clear();
							if (requestData.bodyData.isUseTempFile)
								tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
							else
								std::remove(requestData.targetData.combinedPath.c_str());
						
							generate4xx5xxErrorReponse(requestData, 400, false, "Request Body Chunked::wrong hex format");
						}
					
						/*
							because using [hexNum + _body_size > _client_max_body_size

							size_t + size_t might result as overflow so i have to
							work around
						*/
						if (hexNum > requestData.bodyData.client_max_body_size - requestData.bodyData.body_size)
						{
							requestData.bodyData.writeBodyFile.clear();
							if (requestData.bodyData.isUseTempFile)
								tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
							else
								std::remove(requestData.targetData.combinedPath.c_str());
						
							generate4xx5xxErrorReponse(requestData, 413, false, "Request Body Chunked::Content Too Large");
						}
					
						// 
						if (hexNum == 0)
						{
							_requestBuffer.erase(0, endLinePos + 2);
						
							requestData.bodyData.chunkedBodyIsFinished = true;
							continue;
						}
						requestData.bodyData.chunkedBodyIsFinished = false;
					
						// increase the body size to this chunk
						requestData.bodyData.body_size += hexNum;
					}
				
					// now we skip the hex num part
					_requestBuffer.erase(0, endLinePos + 2);
				}

				size_t	readBodyAmount;
				{
					// calculate the read body amount here
				
					// the size of _requestBuffer is the most the the body can read right now
				
					// content-length body type has _body_size which specified its fixed size
					readBodyAmount = requestData.bodyData.body_size - requestData.bodyData.curr_body_read;
				
					// then if the readBody amount is more than the _requestBuffer.size() then
					if (readBodyAmount > _requestBuffer.size())
					{
						readBodyAmount = _requestBuffer.size();
					}
				}
			
				if (requestData.bodyData.discardBody == true)
				{
					// we just discard from the buffer by that calculated amount
					_requestBuffer.erase(0, readBodyAmount);
					requestData.bodyData.curr_body_read += readBodyAmount;

					if (requestData.bodyData.curr_body_read == requestData.bodyData.body_size)
						continue;
				}
				else
				{
					// here we need to perform the write() operation
					while (true)
					{
						ssize_t	writeAmount = write(requestData.bodyData.writeBodyFile->getFd(), &_requestBuffer[0], readBodyAmount);
						if (writeAmount < 0)
						{
							if (errno == EINTR)
								continue;
							else
							{
								// try to correctly remove file if write operation is failed 
								requestData.bodyData.writeBodyFile.clear();
								if (requestData.bodyData.isUseTempFile)
									tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
								else
									std::remove(requestData.targetData.combinedPath.c_str());
							
								generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::write() operation failed during reading the request body");
							}
						}
						else
						{
							_requestBuffer.erase(0, writeAmount);
							requestData.bodyData.curr_body_read += writeAmount;
							break ;
						}
					}
				
					if (requestData.bodyData.curr_body_read == requestData.bodyData.body_size)
						continue ;
				}
			}
		}
	}
	else
	{
		requestData.bodyData.writeBodyFile.clear();
		if (requestData.bodyData.isUseTempFile)
			tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
		else
			std::remove(requestData.targetData.combinedPath.c_str());
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::_body_type should be only 0, 1, 2");
	}
}

void Http::processingRequestBody(HttpRequest& requestData, const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	if (requestData.processStatus == FINISHED_READ_BODY)
	{
		if (!requestData.targetData.redirectPath.empty())
		{
			requestData.responseTargetPtr->generateResponse();
			requestData.clear();
			return ;
		}
		else if (requestData.requestData.method == "GET" || requestData.requestData.method == "DELETE")
		{
			requestData.clear();
			return ;
		}
		else // must be post right??
		{
			// then if POST is to CGI or not
			if (requestData.cgiData.cgiPath.empty())
			{
				// no cgi means to upload normally
				// but we need to handle the multipart form data here
				if (requestData.bodyData.multiformData.hasData())
				{
					// i don't know how to deal with multipart / form-data yet so,
					requestData.bodyData.writeBodyFile.clear();

					processMultiFormData(requestData);

					tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);

					HttpResponse& targetResponse = *requestData.responseTargetPtr;

					targetResponse.keepAfterResponse = true;
					targetResponse.statusLine->first = 201;
					targetResponse.statusLine->second = "Created";
					targetResponse.contentType = "text/plain";
					targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

					targetResponse.fixedBodyStr = "Files successfully uploaded and created.";
					targetResponse.addHeader("Location", requestData.targetData.targetPath);

					targetResponse.generateResponse();

					requestData.clear();

					//generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::I don't know what to do with multipart-formdata");
				}
				else
				{
					/* didn't use the temp file and i think it's complete, and it is normal download. so */
					HttpResponse& targetResponse = *requestData.responseTargetPtr;

					targetResponse.keepAfterResponse = true;
					targetResponse.statusLine->first = 201;
					targetResponse.statusLine->second = "Created";
					targetResponse.contentType = "text/plain";
					targetResponse.responseBodyType = HTTP_RESPONSE_BODY_FIXED_STR;

					targetResponse.fixedBodyStr = "File successfully uploaded and created.";
					targetResponse.addHeader("Location", requestData.targetData.targetPath);

					targetResponse.generateResponse();

					requestData.clear();
					return ;
				}
			}
			else
			{
				int fd;

				/* we need to re open this temporary file so we can send it so cgi*/			
				{
					requestData.bodyData.writeBodyFile.clear();
					std::string tempFilePath = TEMP_FILE_DIR + toString(requestData.bodyData.tempRequestBodyFileNum);

					fd = open(tempFilePath.c_str(), O_RDONLY);

					/* if failed to open the file with any reason, just delete the temporary file */
					if (fd < 0)
					{
						tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
						generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error: POST to CGIL::failed to open() the temporary file::" + std::string(std::strerror(errno)));
						return ;
					}

				}

				FileDescriptor fdFile(fd);

				int pipe_in[2];
				int pipe_out[2];

				if (pipe(pipe_in) != 0)
				{
					tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI:: pipe() failed::" + std::string(std::strerror(errno)));
				}
				if (pipe(pipe_out) != 0)
				{
					close(pipe_in[0]);
					close(pipe_in[1]);
					tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI:: pipe() failed::" + std::string(std::strerror(errno)));
				}

				pid_t pid = fork();

				if (pid < 0)
				{
					close(pipe_in[0]);
					close(pipe_in[1]);
					close(pipe_out[0]);
					close(pipe_out[1]);
					tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI:: fork() failed::" + std::string(std::strerror(errno)));
				}

				else if (pid == 0)
				{
					try
					{
						TempFileManager().setIsChild(true);
					
						if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
						{
							for (int i = 3; i < MAX_FD; i++)
								close(i);
						
							throw(42);
						}
					
						if (dup2(pipe_in[0], STDIN_FILENO) == -1)
						{
							for (int i = 3; i < MAX_FD; i++)
								close(i);
						
							generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error:: dup2 to stdin failed");
						}
					
						for (int i = 3; i < MAX_FD; i++)
							close(i);
					
						{
							/* prepare before execve() */

							// chdir() to change directory
							{
								size_t pos = requestData.targetData.combinedPath.find_last_of('/');
								std::string scriptPathDir = requestData.targetData.combinedPath.substr(0, pos == std::string::npos ? requestData.targetData.combinedPath.size() : pos + 1);

								if (chdir(scriptPathDir.c_str()) != 0)
									generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error::CGI chdir() failed");
								
								envData().assignEnv("OLDPWD", envData().findValue("PWD"));
								envData().assignEnv("PWD", scriptPathDir);
							}

							/* The GATEWAY_INTERFACE variable MUST be set to the dialect of CGI being used by the server
							to communicate with the script. */
							envData().assignEnv("GATEWAY_INTERFACE", "CGI/1.1");

							{
								std::string tempString = in_addr_t_to_string(_clientSocketPtr->_client_addr_in);
							
								/* the REMOTE_ADDR variable MUST be set to the network address of the client sending the
								request to the server.*/
								envData().assignEnv("REMOTE_ADDR", tempString);
							
								/* For the REMOTE_HOST we need to reverse DNS lookup by using the addr of the client
								and get its actual domain name which ofc we cann't use those functions in this project
								however, the document state that we can just do the same value as REMOTE_ADDR */
								envData().assignEnv("REMOTE_HOST", tempString);
							}

							envData().assignEnv("REQUEST_METHOD", requestData.requestData.method);
							envData().assignEnv("QUERY_STRING", requestData.targetData.queryString);

							/* here for post to CGI will be different from GET because it has body */

							envData().assignEnv("CONTENT_LENGTH", toString(requestData.bodyData.body_size));

							/* for the Content-Type, we can just trim the whitespaces*/
							{
								std::string temp;
								if (httpFieldNormalSingletonTrim(requestData.requestData.headerField.find("content-type")->second, temp) == false)
								{
									generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error:: Post to CGI:: error occured when building env");
								}

								envData().assignEnv("CONTENT_TYPE", temp);
							}

							envData().assignEnv("SCRIPT_NAME", requestData.cgiData.cgiScriptPath);

							if (!requestData.cgiData.cgiVirtualPath.empty())
							{
								envData().assignEnv("PATH_INFO", requestData.cgiData.cgiVirtualPath);
								envData().assignEnv("PATH_TRANSLATED", requestData.cgiData.cgiPathTranslated);
							}

							/* SERVER_NAME just take the host part in Host: from header field, i don't know
							if it is correct but the CGI documentation also state that we can use this so..*/
							envData().assignEnv("SERVER_NAME", requestData.serverData.serverName);

							/* SERVER_PORT is just put the port from URI Authority part here, which
							i already stored it*/
							envData().assignEnv("SERVER_PORT", requestData.serverData.portName);

							/* SERVER_PROTOCOL because we based from RFC9112 which is HTTP1.1 */
							envData().assignEnv("SERVER_PROTOCOL", "HTTP/1.1");

							/* SERVER_SOFTWARE is just our program which mean like webserv/1.0 looks good */
							envData().assignEnv("SERVER_SOFTWARE", "webserv/1.0");

							envData().assignEnv("REDIRECT_STATUS", "200");

							envData().assignEnv("SCRIPT_FILENAME", requestData.targetData.combinedPath);

							/* now we can convert http header to env */
							{
								std::map<std::string, std::string>::const_iterator headerIt = requestData.requestData.headerField.begin();
								std::string headerConvertedStr;

								while (headerIt != requestData.requestData.headerField.end())
								{
									// skip these header
									if (headerIt->first != "content-length"
									&& headerIt->first != "content-type"
									&& headerIt->first != "transfer-encoding"
									&& headerIt->first != "connection"
									&& headerIt->first != "trailer")
									{
										headerConvertedStr = "HTTP_" + headerIt->first;

										// convert to all capital letters and '-' to '_'
										for (size_t i = 0; i < headerConvertedStr.size(); i++)
										{
											if (headerConvertedStr[i] == '-')
												headerConvertedStr[i] = '_';
											else
											{
												headerConvertedStr[i] = static_cast<unsigned char>(std::toupper(headerConvertedStr[i]));
											}
										}

										// lastly assign it to env
										envData().assignEnv(headerConvertedStr, headerIt->second);
									}

									++headerIt;
								}

							}

							/* now we can execve */

							char* argv[3];
							argv[2] = NULL;
							argv[0] = const_cast<char *>(requestData.cgiData.cgiPath.c_str());
							argv[1] = const_cast<char *>(requestData.targetData.combinedPath.c_str());

							for (int i = 3; i < MAX_FD; i++)
								close(i);

							signal(SIGINT, SIG_DFL);
						    signal(SIGQUIT, SIG_DFL);
						    signal(SIGTERM, SIG_DFL);
						    signal(SIGPIPE, SIG_DFL);

							if (execve(argv[0], argv, envData().getEnvp()) == -1)
							{
								signal(SIGINT, serverStopHandler);
							    signal(SIGQUIT, serverStopHandler);
							    signal(SIGTERM, serverStopHandler);
							    signal(SIGPIPE, SIG_IGN);

								generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error:: Post to CGI:: failed execve()");
							}

						}
					}
					catch (HttpThrowStatus &e)
					{
						// should generate response on the list
						HttpResponse& target = *httpRequest.responseTargetPtr;

						// here print all of that to STDOUT
						target.forcePrintAllResponse();
						close(STDOUT_FILENO);
						httpRequest.clear();
					}
					catch (int &e)
					{
						throw ;
					}
					catch (...)
					{
						try 
						{
							generate4xx5xxErrorReponseChildProcess(requestData, 500, false, "Internal Error::Unknown Error Occurred");
						}
						catch (HttpThrowStatus &e)
						{
							HttpResponse& target = *httpRequest.responseTargetPtr;
						
							target.forcePrintAllResponse();
							close(STDOUT_FILENO);
							httpRequest.clear();
						}
					}

					throw int(1);
				}

				else
				{
					/* close unused end pipe */
					close(pipe_out[1]);
					close(pipe_in[0]);

					/* fcntl to both pipe to set to nonblock stream */

					if (fcntl(pipe_out[0], F_SETFL, O_NONBLOCK) != 0)
					{
						tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
						kill(pid, SIGTERM);
						waitpid(pid, NULL, 0);
						close(pipe_out[0]);
						close(pipe_in[1]);
						generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI::fcntl() error to set O_NONBLOCK");
					}

					if (fcntl(pipe_in[1], F_SETFL, O_NONBLOCK) != 0)
					{
						tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
						kill(pid, SIGTERM);
						waitpid(pid, NULL, 0);
						close(pipe_out[0]);
						close(pipe_in[1]);
						generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::CGI::fcntl() error to set O_NONBLOCK");
					}

					try
					{
						HttpResponse& targetResponse = *requestData.responseTargetPtr;


						Shared<HttpCgi> tempShareHttpCgi;

						socketMap.insert(std::make_pair(pipe_out[0], Socket(pipe_out[0])));
						socketMap[pipe_out[0]].setupCGIOUTSocket(clientSocket.getServersConfigPtr(),
						clientSocket.getEventContoller(), &socketMap,
						tempShareHttpCgi);

						socketMap.insert(std::make_pair(pipe_in[1], Socket(pipe_in[1])));
						socketMap[pipe_in[1]].setupCGIINSocket(clientSocket.getServersConfigPtr(),
						clientSocket.getEventContoller(), &socketMap,
						tempShareHttpCgi);

						Shared<CgiProcess>	tempCgiData;

						tempCgiData->cgiPid = pid;
						tempCgiData->status = CGI_PROCESS_RUNNING;
						//tempCgiData->cgiOutSocketPtr = &socketMap[pipe_out[0]];
						//tempCgiData->cgiInSocketPtr = &socketMap[pipe_in[1]];
						tempCgiData->socketMapPtr = &socketMap;
						tempCgiData->clientSocketPtr = _clientSocketPtr;

						s_http_cgi_temp_file_data tempFileData;

						tempFileData.tempReadFileFd = fdFile;
						tempFileData.tempFileNum = requestData.bodyData.tempRequestBodyFileNum;
						tempFileData.isReachEOF = false;
						
						tempShareHttpCgi->setHttpCgiHasCgiIn(&_httpResponseList,
							httpRequest.responseTargetPtr,
							&(socketMap[pipe_out[0]]),
							&(socketMap[pipe_in[1]]),
							tempFileData, tempCgiData);


						targetResponse.cgiProcessData = tempCgiData;
						targetResponse.socketMapPtr = &socketMap;
						targetResponse.targetServer = requestData.serverData.targetServerPtr;
						targetResponse.targetLocationBlock = requestData.serverData.targetLocationBlockPtr;

						targetResponse.httpRequestData = requestData;
					}
					catch (...)
					{
						tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
						close(pipe_in[1]);
						close(pipe_out[0]);
						
						throw;
					}

				}
			}

			requestData.clear();
			return ;
		}
	}
}


void Http::processMultiFormData(HttpRequest& requestData)
{
	try
	{
		/* the boundary string should add with "--" at the front */
		requestData.bodyData.multiformData->boundaryString = "--" + requestData.bodyData.multiformData->boundaryString;
		

		std::string tempFilePath = TEMP_FILE_DIR + toString(requestData.bodyData.tempRequestBodyFileNum);
		std::ifstream multipartFile;
		while (true)
		{
			multipartFile.open(tempFilePath.c_str());
			if (multipartFile.is_open())
				break ;
			if (errno == EINTR)
			{
				multipartFile.clear();
				continue ;
			}
			break ;
		}
		if (multipartFile.is_open() == false)
		{
			tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::Multipart/form-data::can\'t open temp file");
		}

		std::vector<char> multipartReadBuffer(HTTP_RECV_BUFFER);
		std::streamsize readCount;
		std::string multipartBufferString;
		requestData.bodyData.multiformData->state = FORMDATA_STATUS_FINDING_BOUNDARY;

		while (true)
		{
			readCount = 0;
			multipartFile.read(multipartReadBuffer.data(), HTTP_RECV_BUFFER);
			readCount = multipartFile.gcount();
			if (readCount > 0)
			{
				/* do some operation here */
				multipartBufferString.append(multipartReadBuffer.data(), readCount);
				multiformDataProcessBuffer(requestData, multipartBufferString);
			}

			if (multipartFile.fail())
			{
				if (multipartFile.eof())
				{
					/* end of file, may be we should check if it all correctly configured */
					/* but now treated as complete*/

					/* here if eof() and the state is READING HEADER */
					if (requestData.bodyData.multiformData->state != FORMDATA_STATUS_FINISHED)
					{
						generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::wrong-format");
					}

					break ;
				}
				else if (multipartFile.bad() || errno != EINTR)
				{
					/* here is fatal errors happen, we must response and clean properly */
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::read()::fatal Error::" + std::string(std::strerror(errno)));
					break ;
				}

				multipartFile.clear();
				continue ;
			}
		}

	}
	catch (HttpThrowStatus &e)
	{
		/* would try to clean the file when error occur here */
		//std::cout << "remove temp file : " <<  requestData.bodyData.tempRequestBodyFileNum << std::endl;
		tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);

		requestData.bodyData.multiformData->combinedPathFileName.clear();
		requestData.bodyData.multiformData->fileFd.clear();
		requestData.bodyData.multiformData->allCreatedFileList.clear();

		std::list<std::string>& targetFilenameList = requestData.bodyData.multiformData->allCreatedFileList;
		std::list<std::string>::iterator listIt = targetFilenameList.begin();
		while (listIt != targetFilenameList.end())
		{
			std::remove((*(listIt++)).c_str());
		}

		throw ;
	}
	catch (std::exception &e)
	{
		tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
		/* would try to clean the file when error occur here */
		requestData.bodyData.multiformData->combinedPathFileName.clear();
		requestData.bodyData.multiformData->fileFd.clear();
		requestData.bodyData.multiformData->allCreatedFileList.clear();

		std::list<std::string>& targetFilenameList = requestData.bodyData.multiformData->allCreatedFileList;
		std::list<std::string>::iterator listIt = targetFilenameList.begin();
		while (listIt != targetFilenameList.end())
		{
			std::remove((*(listIt++)).c_str());
		}

		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::fatal Error::" + std::string(e.what()));
	}
	catch (...)
	{
		tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
		requestData.bodyData.multiformData->combinedPathFileName.clear();
		requestData.bodyData.multiformData->fileFd.clear();
		requestData.bodyData.multiformData->allCreatedFileList.clear();

		std::list<std::string>& targetFilenameList = requestData.bodyData.multiformData->allCreatedFileList;
		std::list<std::string>::iterator listIt = targetFilenameList.begin();
		while (listIt != targetFilenameList.end())
		{
			std::remove((*(listIt++)).c_str());
		}

		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::Unknown Error");
	}

}

void Http::multiformDataFindingBoundary(HttpRequest& requestData, std::string& multipartBufferString, size_t& currBufPos)
{
	if (requestData.bodyData.multiformData->state == FORMDATA_STATUS_FINDING_BOUNDARY)
	{
		/* can be any amount of preceding CRLF */


		while (currBufPos + 1 < multipartBufferString.size())
		{
			if (multipartBufferString[currBufPos] == '\r')
			{
				if (multipartBufferString[currBufPos + 1] != '\n')
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::does not precede the body with CRLF");
				
				currBufPos += 2;
				continue ;
			}
			else
				break ;
		}

		if (currBufPos >= multipartBufferString.size())
		{
			multipartBufferString.clear();
			return ;
		}

		/* now it should sits on the first charactor of boundary line */

		/* need whole boundary line */

		size_t endLinePos = multipartBufferString.find("\r\n", currBufPos);

		if (endLinePos == std::string::npos)
		{
			if (currBufPos != 0)
				multipartBufferString = multipartBufferString.substr(currBufPos);
			return ;
		}

		std::string foundBoundaryString = multipartBufferString.substr(currBufPos, endLinePos - currBufPos);

		/* then we can compare with our boundary string if not matched then should error */
		if (foundBoundaryString != requestData.bodyData.multiformData->boundaryString)
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::boundary string doesn't match");

		/* if boundary string is found and matched, then we move cursor and move to the next process */

		/* because endLinePos is the "\r\n" then we skip 2 characters */
		currBufPos = endLinePos + 2;

		requestData.bodyData.multiformData->state = FORMDATA_STATUS_READING_HEADER;
		/* if currPos exceeds the buffer then clear() and wait for next read() */
		if (currBufPos >= multipartBufferString.size())
		{
			multipartBufferString.clear();
			return ;
		}
	}
}

void Http::multiformDataReadingHeader(HttpRequest& requestData, std::string& multipartBufferString, size_t& currBufPos)
{
	if (requestData.bodyData.multiformData->state == FORMDATA_STATUS_READING_HEADER)
	{
		size_t endLinePos;
		size_t colonPos;
		std::string tempFieldName;
		std::string tempSep;

		while (requestData.bodyData.multiformData->state == FORMDATA_STATUS_READING_HEADER)
		{
			endLinePos = multipartBufferString.find("\r\n", currBufPos);

			/* need to read line by line, if not found the endlinepos yet 
			we should wait for another round of read() */
			if (endLinePos == std::string::npos)
			{
				if (currBufPos != 0)
					multipartBufferString.erase(0, currBufPos);
				return ;
			}

			/* if endLindPos matches with currBufpos it is \r\n indicating end of header*/
			if (endLinePos == currBufPos)
			{
				requestData.bodyData.multiformData->state = FORMDATA_STATUS_VALIDATING_HEADER;
				if (currBufPos + 2 >= multipartBufferString.size())
					multipartBufferString.clear();
				else
					multipartBufferString.erase(0, currBufPos + 2);
				
				break ;
			}

			/* here endline doesn't match with currPos meaning this is header line */
			colonPos = multipartBufferString.find_first_of(':', currBufPos);
			/*header must separated by ':' if not must reject */
			if (colonPos == std::string::npos || colonPos > endLinePos)
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::Header::name and value must separated by the \':\'");

			tempFieldName = multipartBufferString.substr(currBufPos, colonPos - currBufPos);

			/* must not empty */
			if (tempFieldName.empty())
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::name in header field cannot be empty");

			/* check if not contain any forbidden char ? */
			{
				if (httpFieldNameChar().isMatch(tempFieldName) == false)
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::name in header field must not contain any forbidden chars");

				/* first letter of field name must be an alpha */
				if (allAlphaChar()[tempFieldName[0]] == false)
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::name in header field must start with a letter");
			}

			/* now the field value */
			currBufPos = colonPos + 1;
			tempSep = multipartBufferString.substr(currBufPos, endLinePos - currBufPos);
			tempSep = my_ft_trim(tempSep, " \t");

			if (!tempSep.empty())
			{
				if (forbiddenFieldValueChar().isNotMatch(tempSep) == false)
					generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::value in header field must not contain any forbidden char");

			}

			tempFieldName = stringToLower(tempFieldName);

			std::string& headerValueTarget = requestData.bodyData.multiformData->headerField[tempFieldName];

			if (!headerValueTarget.empty() && !tempSep.empty())
				headerValueTarget += ", ";

			headerValueTarget += tempSep;

			currBufPos = endLinePos + 2;
		}
	}
}

void Http::multiformDataValidating(HttpRequest& requestData)
{
	if (requestData.bodyData.multiformData->state == FORMDATA_STATUS_VALIDATING_HEADER)
	{
		/* would not except if no header at all*/
		std::map<std::string, std::string>& targetHeader = requestData.bodyData.multiformData->headerField;

		if (targetHeader.size() < 1)
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::must have atleast 1 header");

		/* print header */
		{
			std::cout << "##################################################" << std::endl;

			std::map<std::string, std::string>::const_iterator headIt = targetHeader.begin();
			while (headIt != targetHeader.end())
			{
				std::cout << headIt->first << ": " << headIt->second << std::endl;
				++headIt;
			}

			std::cout << "##################################################" << std::endl;
		}

		/* then i must assume that it has Content-Disposition to be able to upload on where */
		{
			std::map<std::string, std::string>::const_iterator foundContentDisposition = targetHeader.find("content-disposition");

			if (foundContentDisposition == targetHeader.end())
			{
				/* i think that this header is must have */

				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::cannot handle part that doesn't have Content-Disposition");
			}

			/* this field kinda have similar parsing as Content-Type */

			std::pair<std::string, std::vector<std::pair<std::string, std::string> > > extractOut;
			if (httpFieldContentTypeExtract(foundContentDisposition->second, extractOut) == false)
			{
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::extract Content-Disposition::failed");
			}

			/* it must be form-data or else i dont know what to do */
			if (extractOut.first.empty() || extractOut.first != "form-data")
			{
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::Content-Disposition::must be form-data only");
			}

			/* i will based from RFC7578 for multipart/form-data */

			if (extractOut.second.empty())
			{
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::Content-Disposition:: must have attributes");
			}

			/* must have 'name' */
			std::vector<std::pair<std::string, std::string> >::const_iterator foundNameAttrib = extractOut.second.end();
			std::vector<std::pair<std::string, std::string> >::const_iterator foundFilenameAttrib = extractOut.second.end();
			std::vector<std::pair<std::string, std::string> >::const_iterator vecIt = extractOut.second.begin();

			while (vecIt != extractOut.second.end())
			{
				if (vecIt->first == "name" && foundNameAttrib == extractOut.second.end())
					foundNameAttrib = vecIt;
				else if (vecIt->first == "filename" && foundFilenameAttrib == extractOut.second.end())
					foundFilenameAttrib = vecIt;
				++vecIt;
			}

			if (foundNameAttrib == extractOut.second.end() || foundNameAttrib->second.empty())
			{
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::Content-Disposition::name attribute not found or empty");
			}

			/* dont know how to deal with this one */
			//if (foundNameAttrib->second == "_charset_")

			if (foundFilenameAttrib != extractOut.second.end())
			{
				/* now we are dealing with filename 
				the RFC said that we should strip any path identifier out at all
				like thos '/' or '\\'. as i'm working on linux */
				std::string strippedFilename;
				if (foundFilenameAttrib->second.empty() == false)
				{
					strippedFilename = foundFilenameAttrib->second;

					size_t lastSlashPos = strippedFilename.find_last_of('/');
					if (lastSlashPos != std::string::npos)
					{
						strippedFilename = strippedFilename.substr(lastSlashPos + 1);
					}
				}

				if (!strippedFilename.empty())
				/* combine with the combined path */
				{
					if (requestData.targetData.combinedPath[requestData.targetData.combinedPath.size() - 1] == '/')
					{
						/* if its last char is already the '/' */
						requestData.bodyData.multiformData->combinedPathFileName = requestData.targetData.combinedPath + strippedFilename;
					}
					else
						requestData.bodyData.multiformData->combinedPathFileName = requestData.targetData.combinedPath + '/' + strippedFilename;
				}
			}

		}

		/* the RFC said that Content-Type is optional even if it is a file to upload
		or not so right now i think i should not handle yet */

		/* the Content-Transfer-Encoding is already deprecated, so it should not exist*/
		{
			std::map<std::string, std::string>::const_iterator foundContentTransferEncoding = targetHeader.find("content-transfer-encoding");

			if (foundContentTransferEncoding != targetHeader.end())
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::Content-Transfer-Encoding must not exist in any part of the form");
		}


		/* IF the combinedPathFilename exist then we must do some check */

		/* first is to check that this file must not exist before upload*/
		/* we can do with stat */

		s_http_request_body_multiform_data& multiFormData = *requestData.bodyData.multiformData;

		if (multiFormData.combinedPathFileName.hasData())
		{
			struct stat fileStat;
			std::memset(&fileStat, 0, sizeof(fileStat));
			if (stat(multiFormData.combinedPathFileName->c_str(), &fileStat) == 0)
			{
				/* if == 0 meaning that stat() can open this part, which it should not*/
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::file already exists!");
			}
			/* after checking with stat() proved that the target path is not exist then we open the file
			to write the data*/


			int fd;
			while (true)
			{
				fd = open(multiFormData.combinedPathFileName->c_str(), O_CREAT | O_TRUNC | O_RDWR);
				if (fd >= 0)
					break ;
				if (errno == EINTR)
					continue ;
				else
					break ;
			}

			if (fd < 0)
			{
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::open() failed::" + std::string(std::strerror(errno)));
			}

			FileDescriptor t(fd);

			multiFormData.fileFd = t;
			/* put it to the list that should be delete in case of error */
			multiFormData.allCreatedFileList.push_back(*multiFormData.combinedPathFileName);
		}

		multiFormData.state = FORMDATA_STATUS_READING_BODY;
	}
}

void Http::multiformDataReadBody(HttpRequest& requestData, std::string& multipartBufferString)
{
	if (requestData.bodyData.multiformData->state == FORMDATA_STATUS_READING_BODY)
	{
		/* because i deal nothing with the data, and will discard all the content
		if 'filename' does not exist in Content-Disposition header */
		OptionalData<FileDescriptor>& targetFileFd = requestData.bodyData.multiformData->fileFd;

		std::string normalBoundaryString = "\r\n" + requestData.bodyData.multiformData->boundaryString + "\r\n";
		std::string finalBoundaryString = "\r\n" + requestData.bodyData.multiformData->boundaryString + "--";


		std::vector<char> writeBuffer;
		writeBuffer.reserve(HTTP_SEND_BUFFER);

		ssize_t writeAmount;
		bool isFoundBoundary = false;
		bool isFinalBoundaty = false;
		int findRet = 0;
		size_t nextBlockPos = std::string::npos;

		while (true)
		{
			if (multipartBufferString.empty() == false)
			{
				size_t normalPos = 0;
				size_t finalPos = 0;
				int normalRet = my_find(multipartBufferString, normalBoundaryString, normalPos);
				int finalRet = my_find(multipartBufferString, finalBoundaryString, finalPos);

				if (normalRet == 2 || finalRet == 2)
				{
					findRet = 2;
					if (normalRet == 2 && finalRet != 2)
					{
						nextBlockPos = normalPos;
						isFinalBoundaty = false;
					}
					else if (finalRet == 2 && normalRet != 2)
					{
						nextBlockPos = finalPos;
						isFinalBoundaty = true;
					}
					else
					{
						if (normalPos <= finalPos)
						{
							nextBlockPos = normalPos;
							isFinalBoundaty = false;
						}
						else
						{
							nextBlockPos = finalPos;
							isFinalBoundaty = true;
						}
					}
				}
				else if (normalRet == 1 || finalRet == 1)
				{
					findRet = 1;
					if (normalRet == 1 && finalRet != 1)
						nextBlockPos = normalPos;
					else if (finalRet == 1 && normalRet != 1)
						nextBlockPos = finalPos;
					else
						nextBlockPos = normalPos <= finalPos ? normalPos : finalPos;
				}
				else
				{
					findRet = 0;
				}

				if (findRet == 0)
				/* if not found yet we take the whole buffer */
				{
					if (targetFileFd.hasData() == false)
					{
						multipartBufferString.clear();
						break ;
					}

					writeBuffer.insert(writeBuffer.end(), multipartBufferString.begin(), multipartBufferString.end());
					multipartBufferString.clear();
				}
				else
				{
					if (targetFileFd.hasData() == false)
					{
						multipartBufferString.erase(0, nextBlockPos);
						isFoundBoundary = true;
						break ;
					}
					writeBuffer.insert(writeBuffer.end(), multipartBufferString.begin(), multipartBufferString.begin() + nextBlockPos);
					multipartBufferString.erase(0, nextBlockPos);
					isFoundBoundary = true;
				}
			}

			if (writeBuffer.empty() == false)
			{
				/* write operation */
				while (true)
				{
					writeAmount = write(targetFileFd->getFd(), writeBuffer.data(), writeBuffer.size());
					if (writeAmount < 0)
					{
						if (errno == EINTR)
							continue ;
						else
						{
							/* fatal error */
							generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::write() failed::" + std::string(std::strerror(errno)));
						}
					}
					break ;
				}

				if (writeAmount >= writeBuffer.size())
				{
					writeBuffer.clear();
				}
				else
				{
					writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + writeAmount);
				}
			}

			/**/

			if ((isFoundBoundary == true || multipartBufferString.empty()) && writeBuffer.empty())
				break ;
		}

		/* if already found isFoundBoundary then*/
		if (isFoundBoundary == true)
		{
			/* found only partial part and need to wait for next found*/
			if (findRet == 1)
			{

			}
			else if (findRet == 2)
			{
				if (isFinalBoundaty)
				{
					multipartBufferString.erase(0, finalBoundaryString.size());

					requestData.bodyData.multiformData->state = FORMDATA_STATUS_FINISHED;
				}
				else
				{
					multipartBufferString.erase(0, normalBoundaryString.size());

					requestData.bodyData.multiformData->state = FORMDATA_STATUS_NEXT_BLOCK;
				}
				/* found whole we erase and ready for next block */
				requestData.bodyData.multiformData->fileFd.clear();
				requestData.bodyData.multiformData->combinedPathFileName.clear();
				requestData.bodyData.multiformData->headerField.clear();
				return ;
			}
			else
			{
				generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::my_find() Fatal Error!");
			}
		}
	}

}

void Http::multiformDataProcessBuffer(HttpRequest& requestData, std::string& multipartBufferString)
{
	while (true)
	{
		size_t currBufPos = 0;

		multiformDataFindingBoundary(requestData, multipartBufferString, currBufPos);

		if (requestData.bodyData.multiformData->state == FORMDATA_STATUS_NEXT_BLOCK)
			requestData.bodyData.multiformData->state = FORMDATA_STATUS_READING_HEADER;

		multiformDataReadingHeader(requestData, multipartBufferString, currBufPos);

		multiformDataValidating(requestData);

		multiformDataReadBody(requestData, multipartBufferString);

		if (requestData.bodyData.multiformData->state != FORMDATA_STATUS_NEXT_BLOCK)
			break ;
	}
}

// use the _requestBuffer
void	Http::processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap)
{
	try
	{
		while (true)
		{
			size_t	currIndex = 0;
			size_t	reqBuffSize = _requestBuffer.size();
			
			/* let's try to think about this
			we had changed the http request to a kind of list
			and we need to change everything completely 
			let's look at parsing http request line, i think this one u can
			generate new HttpRequest on the list, probablye push on the back of the list
			*/

			// put into structure
			parsingHttpRequestLine(currIndex, reqBuffSize);
			parsingHttpHeader(currIndex, reqBuffSize);

			validateRequestData(httpRequest, clientSocket, socketMap);
			readingRequestBody(httpRequest);

			processingRequestBody(httpRequest, clientSocket, socketMap);

			if (httpRequest.processStatus == NO_STATUS && _keepConnection == true)
				continue ;
			else
				break ;
		}
	}
	catch (std::exception &e)
	{
		Logger::log(LC_ERROR, "something weird, Consider as server error ::%s", e.what());
		generate4xx5xxErrorReponse(httpRequest, 500, false, e.what());
		//throw HttpThrowStatus(500, e.what());
	}

	return ;
}


void Http::directRequestProcess(HttpRequest requestData)
{
	try 
	{
		if (requestData.localRedirectCount >= MAX_HTTP_LOCAL_REDIRECT_COUNT)
		{
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::MAX_HTTP_LOCAL_REDIRECT_COUNT reached");
		}
		requestData.localRedirectCount += 1;

		validateRequestData(requestData, *_clientSocketPtr, *_socketMapPtr);

		/* skip the reading body */
		if (requestData.processStatus == READ_BODY)
			requestData.processStatus = FINISHED_READ_BODY;

		processingRequestBody(requestData, *_clientSocketPtr, *_socketMapPtr);
	}
	catch (int &e)
	{
		/* this one bubble out to main*/
		throw ;
	}
	catch (HttpThrowStatus &status)
	{
		Logger::log(LC_INFO, "%d::reponse with status code %d::%s", _clientSocketPtr->getSocketFD().getFd(), status.statusCode(), status.message().c_str());
	}
	catch (std::exception &e)
	{
		Logger::log(LC_INFO, "Http::socket#%d::Internal Error::%s", e.what());
	}
	catch (...)
	{
		Logger::log(LC_INFO, "Http::socket#%d::Unknown Internal Error");
	}

}

void Http::readFromClient()
{
	//std::cout << "READ FROM CLIENT" << std::endl;
	ssize_t	readAmount;

	readAmount = recv(_clientSocketPtr->getSocketFD().getFd(), _recvBuffer.data(), HTTP_RECV_BUFFER, 0);
	if (readAmount == 0)
	{
		Logger::log(LC_CONN_LOG, "Disconnecting client Socket#%d", _clientSocketPtr->getSocketFD().getFd());
		_keepConnection = false;
		return ;
	}
	else if (readAmount < 0)
	{
		_keepConnection = false;
		std::cout << "reach here" << std::endl;
		std::cout << "fd to recv: " << _clientSocketPtr->getSocketFD().getFd() << std::endl;
		std::cout << "is keep connection: " << _keepConnection << std::endl;
		return ;
	}
	else 
	{
		_requestBuffer.append(_recvBuffer.data(), readAmount);
		try
		{
			processingRequestBuffer(*_clientSocketPtr, *_socketMapPtr);
		}
		catch (HttpThrowStatus &status)
		{
			Logger::log(LC_INFO, "Http::socket#%d::reponse with status code %d::%s", _clientSocketPtr->getSocketFD().getFd(), status.statusCode(), status.message().c_str());
			httpRequest.clear();
		}
		catch (std::exception &e)
		{
			Logger::log(LC_ERROR, "Http::socket#%d::Exception: %s", _clientSocketPtr->getSocketFD().getFd(), e.what());
			_keepConnection = false;
		}
	}
	
	if (_isEpollout == false && _httpResponseList.empty() == false && _httpResponseList.front().hasSomethingtoSend() == true)
	{
		epoll_event	event;
		std::memset(&event, 0, sizeof(event));
		event.events = EPOLLIN | EPOLLOUT;
		event.data.fd = _clientSocketPtr->getSocketFD().getFd();

		if (epoll_ctl(_clientSocketPtr->getEpollFD().getFd(), EPOLL_CTL_MOD, _clientSocketPtr->getSocketFD().getFd(), &event) == -1)
		{
			std::string msg = "Http::readFromClient()::epoll_ctl()::error::";
			msg += std::strerror(errno);
			throw WebservException(msg);
		}
		_isEpollout = true;
	}
	//_keepConnection = false;
}

void	Http::writeToClient()
{
	//std::cout << "WRITE TO CLIENT" << std::endl;
	HttpResponse *response_block = NULL;

	_isEpollout = true;

	// check if it is empty or what
	if (_httpResponseList.empty() || _httpResponseList.front().isComplete())
	{
		Logger::log(LC_DEBUG, "WHAAAAAAT");
		return ;
	}
	response_block = &_httpResponseList.front();

	ssize_t	sendReturn = response_block->sendHttpResponse(*_clientSocketPtr);

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
		_keepConnection = response_block->keepAfterResponse;
		_httpResponseList.pop_front();
	}

	// then if the response list is empty now
	// we close the EPOLLOUT
	if (_httpResponseList.empty() || _httpResponseList.front().hasSomethingtoSend() == false)
	{
		epoll_event	event;
		std::memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = _clientSocketPtr->getSocketFD().getFd();

		if (epoll_ctl(_clientSocketPtr->getEpollFD().getFd(), EPOLL_CTL_MOD, _clientSocketPtr->getSocketFD().getFd(), &event) == -1)
		{
			std::string msg = "Http::writeToClient::epoll_ctl()::error::";
			msg += std::strerror(errno);
			throw WebservException(msg);
		}
		_isEpollout = false;
	}
}

/*
	Explaination of the outVec

	So the Content-Type value field have structure some how like this
	1. this field can have many element and separate with ',' so it is a vector
	2. for each element has its name and can its optional attributes, so it is a std::pair (its name and its attributes)
	3. Each of element can have more than one attributes so it is a vector
	4. each attributes is its name and value separated by '=' so it is a std::pair
*/
bool Http::httpFieldContentTypeExtract(const std::string& toExtract, std::pair<std::string, std::vector<std::pair<std::string, std::string> > > & outPair)
{
	std::deque<s_http_field_value_token> tempTokenList;

	if (extractHttpFieldValueString(toExtract, tempTokenList, ";=\"", " \t") == false)
	{
		return (false);
	}

	/* Now we go and read to token List*/

	std::deque<s_http_field_value_token>::const_iterator listIt = tempTokenList.begin();

	std::deque<s_http_field_value_token>::const_iterator tempListPos = tempTokenList.end();
	std::vector<std::pair<std::string, std::string> > tempAttribVec;
	std::pair<std::string, std::string> tempAttrib;

	bool hasEqual = false;

	while (listIt != tempTokenList.end())
	{
		// here you would need to skip any of the first
		if (listIt->tokenType == OPTIONAL_CHAR)
		{
			++listIt;
			continue;
		}
		else if (listIt->tokenType == WORD)
		{
			if (outPair.first.empty())
			{
				outPair.first = listIt->str;
				if (httpContentTypeChar().isMatch(outPair.first) == false)
				{
					return (false);
				}
			}
			else
			{
				if (!tempAttrib.first.empty())
					return (false);
				tempAttrib.first = listIt->str;
				if (httpTokenChar().isMatch(tempAttrib.first) == false)
					return (false);
				tempListPos = listIt;
			}
			++listIt;
			continue;
		}
		else if (listIt->tokenType == DELIMITER)
		{
			if (listIt->str[0] == ';')
			{
				if (outPair.first.empty())
					return (false);

				if (!tempAttrib.first.empty() && !hasEqual)
					return (false);

				if (!tempAttrib.first.empty())
				{
					tempAttribVec.push_back(tempAttrib);
					tempAttrib.first.clear();
					tempAttrib.second.clear();
					hasEqual = false;
				}
				++listIt;
				continue;
			}
			else if (listIt->str[0] == '=')
			{
				if (listIt->str.size() != 1 || outPair.first.empty() || tempAttrib.first.empty()
				|| (listIt + 1) == tempTokenList.end()
				|| tempListPos == tempTokenList.end()
				|| listIt == tempTokenList.begin()
				|| hasEqual == true
				|| !tempAttrib.second.empty()
				|| (listIt - 1) != tempListPos
				|| ((listIt + 1)->tokenType == DELIMITER && (listIt + 1)->str[0] != '\"')
				|| ((listIt + 1)->tokenType != WORD && (listIt + 1)->tokenType != DELIMITER))
				{
					return (false);
				}
				
				hasEqual = true;
				tempListPos = tempTokenList.end();
				if ((++listIt)->tokenType == WORD)
				{
					tempAttrib.second = listIt->str;
					if (httpTokenChar().isMatch(tempAttrib.second) == false)
					{
						return (false);
					}
					
					++listIt;
					continue;
				}
				else
				{
					if (listIt->str.size() >= 2)
						return (false);
					// here deal with quoted

					++listIt;
					if (listIt == tempTokenList.end())
						return (false);
					while (listIt != tempTokenList.end() && !(listIt->tokenType == DELIMITER && listIt->str[0] == '\"'))
					{
						if (listIt->tokenType == ESCAPE_CHAR)
						{
							if (httpQuotedPairAllowedChar()[listIt->str[0]] == false)
								return (false);
							tempAttrib.second += listIt->str;
						}
						else
						{
							if (httpAllowedQuotedChar().isMatch(listIt->str) == false)
								return (false);
							tempAttrib.second += listIt->str;
						}
						++listIt;
					}
					if (listIt == tempTokenList.end() || listIt->str.size() >= 2)
						return (false);
					
					++listIt;
					continue;	
				}
			}
			else
				return (false);
		}
		else
			return (false);
	}

	if (outPair.first.empty() || (!tempAttrib.first.empty() && !hasEqual))
		return (false);

	if (!tempAttrib.first.empty())
		tempAttribVec.push_back(tempAttrib);

	outPair.second = tempAttribVec;
	return (true);
}

bool Http::httpFieldNormalCommaElement(const std::string& toExtract, std::vector<std::string>& outVec)
{
	if (!outVec.empty())
		outVec.clear();

	if (toExtract.empty())
		return (true);


	std::deque<s_http_field_value_token> tempTokenList;
	if (toExtract.find_first_of(',') == std::string::npos)
	{
		/* if no comma, just simply single ton*/
		std::string tempStringOut;
		if (httpFieldNormalSingletonTrim(toExtract, tempStringOut) == false)
			return (false);
		if (!tempStringOut.empty())
			outVec.push_back(tempStringOut);
		return (true);
	}
	else
	{
		/* here, there is a comma */
		if (extractHttpFieldValueString(toExtract, tempTokenList, ",", " \t") == false)
			return (false);

		std::deque<s_http_field_value_token>::const_iterator listIt = tempTokenList.begin();
		std::deque<s_http_field_value_token>::const_iterator headIt = tempTokenList.end();
		std::deque<s_http_field_value_token>::const_iterator tailIt;
		while (listIt != tempTokenList.end())
		{
			/* simply skip any whitespaces first? */
			if (listIt->tokenType == OPTIONAL_CHAR)
			{
				++listIt;
				continue;
			}
			else if (listIt->tokenType == WORD)
			{
				if (headIt == tempTokenList.end())
					headIt = listIt;
				++listIt;
				continue;
			}
			else if (listIt->tokenType == DELIMITER)
			{
				if (headIt != tempTokenList.end())
				{
					std::string tempStr;
					tailIt = listIt - 1;

					while (tailIt->tokenType != WORD)
						--tailIt;
					
					while (headIt != tailIt)
					{
						tempStr += headIt->str;
						++headIt;
					}
					tempStr += headIt->str;

					if (!tempStr.empty())
						outVec.push_back(tempStr);
					
					headIt = tempTokenList.end();
				}
				++listIt;
				continue;
			}
			else
				return (false);
		}

		if (headIt != tempTokenList.end())
		{
			std::string tempStr;
			tailIt = listIt - 1;

			while (tailIt->tokenType != WORD)
				--tailIt;
			
			while (headIt != tailIt)
			{
				tempStr += headIt->str;
				++headIt;
			}
			tempStr += headIt->str;

			if (!tempStr.empty())
				outVec.push_back(tempStr);
			
		}

		/* If have comma it must have at least 1 element if not then should return false */
		if (outVec.size() < 1)
			return (false);

		return (true);
	}

	return (true);
}

bool Http::httpFieldNormalSingletonTrim(const std::string& toExtract, std::string& outString)
{
	/* for normal singleton, i just gonna trim the SP and HTAB
		** this doens't handle the quoted string */
	if (!outString.empty())
		outString.clear();

	if (toExtract.empty())
		return (true);

	size_t frontPos = htabSp().findFirstNotCharset(toExtract);

	if (frontPos == std::string::npos)
		return (true);
	
	size_t endPos = htabSp().findLastNotCharset(toExtract);

	/* End pos should not get the npos though, looking from the logic */
	if (endPos == std::string::npos)
		return(false);

	outString = toExtract.substr(frontPos, endPos - frontPos + 1);
	return (true);
}


bool Http::extractHttpFieldValueString(const std::string& toExtract, std::deque<s_http_field_value_token>& tokenList, const std::string& delimiterCharSet, const std::string& optionalSpaceCharSet)
{
	tokenList.clear();
	if (toExtract.empty())
		return (true);

	CharTable delimTable(delimiterCharSet, true);
	CharTable optionalTable(optionalSpaceCharSet, true);
	CharTable compTable(delimiterCharSet + optionalSpaceCharSet, true);

	size_t i = 0;
	size_t j = 0;
	std::string tempStr;
	while (i < toExtract.size())
	{
		if (toExtract[i] == '\\')
		{
			// quoted-pair escape char??
			if (i + 1 == toExtract.size())
				return (false);
			if (httpQuotedPairAllowedChar()[toExtract[i + 1]] == false)
				return (false);
			tokenList.push_back(s_http_field_value_token());

			s_http_field_value_token& targetToken = tokenList.back();

			targetToken.tokenType = ESCAPE_CHAR;
			targetToken.str += toExtract[i + 1];

			i += 2;
			continue;
		}

		if (optionalTable[toExtract[i]] == true)
		{
			j = optionalTable.findFirstNotCharset(toExtract, i);

			if (j == std::string::npos)
				j = toExtract.size();

			tokenList.push_back(s_http_field_value_token());
			s_http_field_value_token& targetToken = tokenList.back();
			
			targetToken.tokenType = OPTIONAL_CHAR;
			targetToken.str = toExtract.substr(i, j - i);

			i = j;
			continue;
		}
		else if (delimTable[toExtract[i]] == true)
		{
			//j = toExtract.find_first_not_of(toExtract[i], i);

			//if (j == std::string::npos)
			//	j = toExtract.size();

			j = i + 1;

			tokenList.push_back(s_http_field_value_token());
			s_http_field_value_token& targetToken = tokenList.back();

			targetToken.tokenType = DELIMITER;
			targetToken.str = toExtract.substr(i, j - i);

			i = j;
			continue;
		}
		else
		{
			// general token i think ?

			j = compTable.findFirstCharset(toExtract, i);
			if (j == std::string::npos)
				j = toExtract.size();

			tokenList.push_back(s_http_field_value_token());
			s_http_field_value_token& targetToken = tokenList.back();

			targetToken.tokenType = WORD;
			targetToken.str = toExtract.substr(i, j - i);

			i = j;
			continue;
		}
	}
	return (true);
}
