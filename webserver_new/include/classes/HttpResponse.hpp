#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include "../defined_value.hpp"
# include <list>
# include <map>
# include <set>
# include "./WebservException.hpp"
# include "./utility_function.hpp"
# include <ctime>
# include "../../include/classes/FileDescriptor.hpp"

class Socket;

enum e_http_response_body_type
{
	HTTP_RESPONSE_NOBODY,
	HTTP_RESPONSE_BODY_FIXED_STR,
	HTTP_RESPONSE_BODY_FILE,
	HTTP_RESPONSE_BODY_CGI
};

class HttpResponse {
	private:

		//bool	isCgiAlive;
		//bool	isComplete;
		std::vector<char> _sendBuffer;
		size_t			_sendBufferOffset;

		std::map<std::string, std::set<std::string> > _responseHeader;
		unsigned int _statusCode;
		std::string	_statusMessage;
		std::string _contentType;
		FileDescriptor _fileFd;
		std::string _fixedBodyStr;
		e_http_response_body_type	_responseBodyType;
		bool _keepAfterResponse;
		bool _canModify;
		bool _isComplete;
		bool _hasSomethingtoSend;
		bool _isReachEOF;

		// need to fix this later
		bool _isCgiFinished;


		// if all Buffer is sent. the response has nothing
		// to send
		std::list<s_response_buff> _bufferList;


	public:
		HttpResponse();


		void addHeader(const std::string& headerName, const std::string& headerValue);

		void generateResponse();

		bool getKeepAfterResponse() const;
		void setKeepAfterResponse(bool op);

		int	getStatusCode() const;
		void setStatusCode(unsigned int statusCode);

		const std::string& getStatusMessage() const;
		void setStatusMessage(const std::string& statusMessage);

		//

		const std::string& getContentType() const;
		void setContentType(const std::string& contentType);

		e_http_response_body_type getResponseBodyType() const;
		void setResponseBodyType(e_http_response_body_type bodyType);

		const std::string& getFixedBodyStr() const;
		void setFixedBodyStr(const std::string& bodyStr);

		const FileDescriptor& getFileFd() const;
		void setFileFd(const FileDescriptor& fd);

		ssize_t	sendHttpResponse(const Socket& clientSocket);

		// ready to remove from the list
		bool isComplete() const;
		bool hasSomethingtoSend() const;
};

#endif
