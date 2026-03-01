#ifndef HTTP_HPP
# define HTTP_HPP

# include <list>
# include <map>
# include <set>
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

class Http {
	private:
		std::vector<char>	_recvBuffer;
		std::string _requestBuffer;

		bool	_keepConnection;
		bool	_isEpollout;

		std::string	_sendBuffer;
		std::list<HttpResponse>	_httpResponseList;

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
		void	parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize);
		void	parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize);
		void	validateRequestBufffer(const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void	validateRequestBufferSelectServer(const Socket& clientSocket, const std::string& authStr);
		void	readingRequestBody(size_t& currIndex, size_t& reqBuffSize);

		void	generate4xx5xxErrorReponse(unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg);
		void	generate3xxRedirectResponse(unsigned int statusCode);


		void	clearRequestData();
		e_http_process_status	_processStatus;
		std::string		_method;
		std::string		_requestTarget;	
		std::string		_targetPath;
		std::string		_combinedPath;
		std::string		_queryString;
		std::string		_protocol; // after validating the string will be "HTTP/1.1" => "11"

		std::string		_cgiScriptPath;
		std::string		_cgiVirtualPath;
		std::string		_cgiPathTranslated;
		std::string		_cgiPath;
		bool			_isCgiProcessOpen;

		std::string 	_uploadStorePath;
		std::string		_authorityPart;
		std::string		_redirectPath;

		bool			_isUseTempFile;
		unsigned int	_tempRequestBodyFileNum;
		bool			_checkExpectBody;
		bool			_readBody;
		bool			_discardBody; // whether to drain down the body
		/*
			_body_type
			0 = no body
			1 = content-lenth body
			2 = transfer-encoding body
		*/
		int				_body_type;
		size_t			_body_size;
		size_t			_curr_body_read;

		void	printHeaderField() const;
		std::map<std::string, std::vector<std::string> > _headerField;
		const ServerConfig*	_targetServer;
		const t_config_map* _targetLocationBlock;

	public:
		Http();
		~Http();

		void	readFromClient(const Socket& clientSocket, std::map<int, Socket>& SocketMap);
		void	writeToClient(const Socket& clientSocket, std::map<int, Socket>& SocketMap);

		bool	isKeepConnection() const;
};

#endif
