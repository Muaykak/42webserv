#ifndef HTTP_HPP
# define HTTP_HPP

# include <list>
# include <map>
# include <set>
# include <deque>
# include "../defined_value.hpp"
# include "./ServerConfig.hpp"
# include "./HttpThrowStatus.hpp"
# include "./HttpResponse.hpp"

class Socket;

enum e_http_process_status {
	NO_STATUS,
	READING_REQUEST_LINE,
	READING_HEADER,
	VALIDATING_REQUEST,
	READ_BODY,
	FINISHED_READ_BODY
};

enum e_http_field_value_token_type
{
	TOKEN_UNKNOWN_TYPE,
	WORD,
	ESCAPE_CHAR,
	DELIMITER,
	OPTIONAL_CHAR
};

struct s_http_request_data {
	std::string	method;
	std::string	requestTarget;
	std::string	protocol;
	std::map<std::string, std::string> headerField;

	/* because of local redirection, maybe i should 
	treat http request as list but idk, let's try to preocess
	
	In the normal stream, you'll have to read request by request
	and there is ResponseList we will send back to client by list
	from oldest to newest orderly. But local redirect makes us
	needing to reprocess the request data again to new request target

	so, may by the step of validatoin or execute the actual
	request can be done with deque
	*/
};

struct s_http_field_value_token {
	e_http_field_value_token_type tokenType;
	std::string str;
};


class Http {
	private:
		Socket* _clientSocketPtr;
		std::map<int, Socket>* _socketMapPtr;
		std::vector<char>	_recvBuffer;
		std::string _requestBuffer;

		bool	_keepConnection;
		bool	_isEpollout;

		std::string	_sendBuffer;
		std::list<HttpResponse>	_httpResponseList;

		/* small functions */


		/*
		return value:

		0 = false, error occurred, usually define
			the error code to _errorStatusCode (no error will default set to -1)
			may skip to create response or just close connection.
		
		4 = false, error occured, but it is possible to response error and continue
		to next request
		

		1 = success, can continue to next procedure

		2 = need to wait for new buffer (wait for next EPOLLIN)

		NOTE: return value = 0 usually means ready to
			create response (whether it is fail or success)
		*/

		void	processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void		parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize);
		void		parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize);



		void		validateRequestData(s_http_request_data& requestData, const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void			validateRequestBufferSelectServer(const Socket& clientSocket, const std::string& authStr);
		void			requestLineCheck(s_http_request_data& requestData, const Socket& clientSocket);
		void				requestLineCheckProtocolVersion(s_http_request_data& requestData);
		void				requestLineCheckRequestTarget(s_http_request_data& requestData, const Socket& clientSocket);
		void					requestLineCheckRequestTargetAbsolute(s_http_request_data& requestData, const Socket& clientSocket);
		void					requestLineCheckRequestTargetPathCheck();
		void			targetLocationBlockGet();
		void			checkRequestBodyType(s_http_request_data& requestData);
		bool			checkRedirection();
		void			checkMethod(s_http_request_data& requestData);
		void			checkCgiPath();
		void			createSystemPath();
		void			createSystemPath(std::string& systemPath);
		void			handleDeleteRequest();
		void			handleGetRequest(bool isEndWithSlash, const Socket& clientSocket, std::map<int, Socket>& socketMap);

		void	readingRequestBody();
		void	processingRequestBody(const Socket& clientSocket, std::map<int, Socket>& socketMap);

		void	cgiChildProcessNoRequestBody(s_http_request_data& requestData, int pipeForCgiStdOut[2]);

		void	clearRequestData();
		e_http_process_status	_processStatus;


		s_http_request_data TrequestData;
		//std::map<std::string, std::string> _headerField;
		//std::string		_method;
		//std::string		_requestTarget;	
		//std::string		_protocol; // after validating the string will be "HTTP/1.1" => "11"
		std::string		_serverName;
		std::string		_portString;
		std::string		_targetPath;
		std::string		_combinedPath;
		std::string		_queryString;

		std::string		_cgiScriptPath;
		std::string		_cgiVirtualPath;
		std::string		_cgiPathTranslated;
		std::string		_cgiPath;
		// bool			_isCgiProcessOpen;
		// pid_t			_cgiProcessPid;
		// bool			_isCgiInSocketAlive;
		// bool			_isCgiOutSocketAlive;
		// std::vector<HttpCgi> _httpCgi;
		// int	_cgiInSocket;
		// int	_cgiOutSocket;

		std::string 	_uploadStorePath;
		std::string		_authorityPart;
		std::string		_redirectPath;

		bool			_isMultiForm;
		std::string		_boundaryString;
		std::string		_bodyContentType;
		bool			_isUseTempFile;
		unsigned int	_tempRequestBodyFileNum;
		bool			_checkExpectBody;
		bool			_readBody;
		bool			_discardBody; // whether to drain down the body
		std::vector<FileDescriptor> _bodyFd; // convention to fd
		/*
			_body_type
			0 = no body
			1 = content-lenth body
			2 = transfer-encoding body
		*/
		int				_body_type;
		size_t			_body_size;
		bool			_chunkedBodyHasTrailerHeader;
		bool			_chunkedBodyIsFinished;
		std::set<std::string> _trailerHeader;
		size_t			_client_max_body_size;
		size_t			_curr_body_read;

		void	printHeaderField(s_http_request_data& requestData) const;
		const ServerConfig*	_targetServer;
		const t_config_map* _targetLocationBlock;

		void	generate4xx5xxErrorReponse(unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg);
		void	generate3xxRedirectResponse(unsigned int statusCode);

	public:
		Http();
		Http(Socket *clientSocketPtr, std::map<int, Socket>* socketMapPtr);
		Http(const Http& obj);
		~Http();

		void	readFromClient();
		void	writeToClient();

		bool	isKeepConnection() const;



		static bool extractHttpFieldValueString(const std::string& toExtract, std::deque<s_http_field_value_token>& tokenList, const std::string& delimiterCharSet, const std::string& optionalSpaceCharSet);
		static bool httpFieldNormalSingletonTrim(const std::string& toExtract, std::string& outString);
		static bool httpFieldNormalCommaElement(const std::string& toExtract, std::vector<std::string>& outVec);
		static bool httpFieldContentTypeExtract(const std::string& toExtract, std::pair<std::string, std::vector<std::pair<std::string, std::string> > > & outPair);
};

#endif
