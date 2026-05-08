#ifndef HTTPCGI_HPP
# define HTTPCGI_HPP

# include "Http.hpp"
# include <sys/stat.h>

enum e_httpcgi_process_status
{
	HTTPCGI_NO_STATUS,
	HTTPCGI_SENDING_TO_CGI,
	HTTPCGI_READING_RESPONSE_HEADER,
	HTTPCGI_VALIDATING_RESPONSE,
	HTTPCGI_SENDING_RESPONSE_BUFFER,
	HTTPCGI_FINISHED,
	HTTPCGI_CLOSED_CGI
};

struct s_http_cgi_response_body_data
{
	bool isChunkBody;
	bool isManualChunk;
	size_t bodySize;
	bool chunkBodyIsFinished;
	size_t curr_body_read;
};

struct s_http_cgi_temp_file_data
{
	bool isReachEOF;
	FileDescriptor tempReadFileFd;
	unsigned int tempFileNum;
};

class HttpCgi {
	private:
		std::list<HttpResponse>	*_clientResponseList;
		HttpResponse	*_cgiTargetResponse;
		Socket	*_cgiOutSocket;
		OptionalData<Socket *> _cgiInSocket;

		e_httpcgi_process_status httpCgiStatus;
		Shared<CgiProcess> cgiProcessData;
		// need the parent client socket to set the
		// epoll_ctl()

		std::vector<char> _readCgiBuffer;
		std::vector<char> _writeCgiBuffer;
		std::string _responseBuffer;

		/* for send to CGI we need to have the temporaryfileFd that it will read from */
		OptionalData<s_http_cgi_temp_file_data> _tempFileData;

		// FOR CGIOUT
		std::map<std::string, std::string> _responseHeaderCGIOUT;
		OptionalData<s_http_cgi_response_body_data> _bodyData;

		void generate5xxCGIOUTresponseError(unsigned int errorCode, const std::string& throwMsg);

		void processCGIOUTresponseBuffer();
		void	parsingCGIOUTresponseHeader();

		void	validateCGIOUTresponse();
		void	sendToResponseBuffer();

		// ------------------------------

		// FORCGIIN

		//---------------------------------

		bool _isFinishedRead;
		bool _keepConnection;

		void sendToCGI(Socket* currentSocket, const epoll_event& epollEvent);
		void readFromCGI(Socket* currentSocket, const epoll_event& epollEvent);
	public:


		HttpCgi();
		HttpCgi(const HttpCgi& obj);

		void setHttpCgiHasCgiIn(std::list<HttpResponse>* clientResponseList,
		HttpResponse* cgiTargetResponse,
		Socket *cgiOutSocket, Socket* cgiInSocket,
		const s_http_cgi_temp_file_data& tempFileData,
		Shared<CgiProcess>& cgiProcessData);

		void setHttpCgiNoCgiIn(std::list<HttpResponse>* clientResponseList,
		HttpResponse* cgiTargetResponse,
		Socket *cgiOutSocket, Shared<CgiProcess>& cgiProcessData);

		HttpCgi& operator=(const HttpCgi& obj); // declare but no implement
		~HttpCgi();

		bool isKeepConnection(const Socket* currentCgiSocket) const;

		e_httpcgi_process_status& status();

		void processCGI(Socket* currentSocket, const epoll_event& epollEvent);
		CgiProcess& getCgiProcess();

		void forceSigTerm();
};

#endif
