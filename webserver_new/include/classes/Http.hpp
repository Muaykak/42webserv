#ifndef HTTP_HPP
# define HTTP_HPP

# include <list>
# include <map>
# include <set>
# include "../defined_value.hpp"

class Socket;

enum e_http_process_status {
	NO_STATUS,
	READING_REQUEST_LINE,
	READING_HEADER,
	PROCESSING_REQUEST,
};

enum e_http_request_method {
	NO_METHOD,
	GET,
	POST,
	DELETE
};

class Http {
	private:
		std::vector<char>	_recvBuffer;
		std::string _requestBuffer;

		bool	_keepConnection;

		std::string	_sendBuffer;


		/*
		return value:

		0 = false, error occurred, usually define
			the error code to _errorStatusCode (no error will default set to -1)
			may skip to create response or just close connection.

		1 = success, can continue to next procedure

		2 = need to wait for new buffer (wait for next EPOLLIN)

		NOTE: return value = 0 usually means ready to
			create response (whether it is fail or success)
		*/
		int		_process_return;



		e_http_process_status	_processStatus;
		void	processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void	Http::parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize);
		void	parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize);
		void	readingRequestBuffer();
		void	validateRequestBufffer();

		int	_errorStatusCode;
		std::string	_throwMessageToClient; // so we know where it happen error
		void	httpError(int errorCode, const std::string& throwToClient);
		void	httpError(int errorCode);


		e_http_request_method	_method;
		std::string				_requestTarget;
		std::string				_protocol;

		void	printHeaderField() const;
		std::map<std::string, std::set<std::string> > _headerField;

	public:
		Http();
		~Http();

		void	readFromClient(const Socket& clientSocket, std::map<int, Socket>& SocketMap);
		void	writeToClient(const Socket& clientSocket, std::map<int, Socket>& SocketMap);

		bool	isKeepConnection() const;
};

#endif
