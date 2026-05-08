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
# include "CgiProcess.hpp"
# include "Shared.hpp"
# include "../webserv_structs.hpp"

class Socket;
class HttpRequest;
class ServerConfig;

enum e_http_response_body_type
{
	HTTP_RESPONSE_NOBODY,
	HTTP_RESPONSE_BODY_FIXED_STR,
	HTTP_RESPONSE_BODY_FILE,
	HTTP_RESPONSE_CGI_BODY
};

// struct s_http_response_cgidata
// {
// 		bool isUseCgi;
// 		bool isCgiProcessOpen;
// 		pid_t cgiPid;
// 		bool isCgiInSocketAlive;
// 		bool isCgiOutSocketAlive;
// 		int  cgiInSocketFd;
// 		int  cgiOutSocketFd;
// 		bool isCgiFinished;
// };

class HttpResponse {
	private:

		bool _isComplete;
		bool _hasSomethingtoSend;

		//bool	isCgiAlive;
		//bool	isComplete;
		std::vector<char> _sendBuffer;
		size_t			_sendBufferOffset;

		// if all Buffer is sent. the response has nothing
		// to send
		std::list<s_response_buff> _bufferList;

	public:
		HttpResponse();
		~HttpResponse();

		std::map<std::string, std::vector<std::string> > responseHeader;

		//unsigned int statusCode;
		//std::string	statusMessage;

		OptionalData<std::pair<unsigned int, std::string> > statusLine;

		std::string contentType;
		OptionalData<FileDescriptor> fileFd;
		//FileDescriptor cgiOutFd;
		size_t	fileSize;

		std::string fixedBodyStr;
		e_http_response_body_type	responseBodyType;
		bool keepAfterResponse;
		bool canModify;
		bool isReachEOF;

		OptionalData<HttpRequest> httpRequestData;

		// all about cgi

		// s_http_response_cgidata cgiData;
		OptionalData<Shared<CgiProcess> > cgiProcessData;

		std::map<int, Socket> *socketMapPtr;
		const ServerConfig*	targetServer;
		const t_config_map* targetLocationBlock;

		void pushNewResponseBuff(s_response_buff& newBuff);

		void addHeader(const std::string& headerName, const std::string& headerValue);
		std::map<std::string, std::vector<std::string> >& getHeader();

		void generateResponse();

		ssize_t	sendHttpResponse(const Socket& clientSocket);

		// use for CGI child process error
		void forcePrintAllResponse();

		// ready to remove from the list
		bool isComplete() const;

		bool hasSomethingtoSend() const;

};

#endif
