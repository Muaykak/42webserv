#ifndef HTTPCGI_HPP
# define HTTPCGI_HPP

# include "Http.hpp"
# include <sys/stat.h>

enum e_httpcgi_process_status
{
	HTTPCGIOUT_NO_STATUS,
	HTTPCGIOUT_READING_RESPONSE_HEADER,
	HTTPCGIOUT_VALIDATING_RESPONSE,
	HTTPCGIOUT_FINISHED
};

class HttpCgi {
	private:
		std::list<HttpResponse>	*_clientResponseList;
		HttpResponse	*_cgiTargetResponse;
		Socket	*_cgiOutSocket;
		OptionalData<Socket *> _cgiInSocket;

		// need the parent client socket to set the
		// epoll_ctl()
		FileDescriptor	_mainHttpSocketFd;

		std::vector<char> _readCgiBuffer;
		std::string _responseBuffer;

		/* for send to CGI we need to have the temporaryfileFd that it will read from */
		OptionalData<FileDescriptor> _tempReadFileFd;

		// FOR CGIOUT
		e_httpcgi_process_status _cgioutProcessStatus;
		std::map<std::string, std::string> _responseHeaderCGIOUT;

		void generate5xxCGIOUTresponseError(unsigned int errorCode, const std::string& throwMsg);

		void processCGIOUTresponseBuffer();
		void	parsingCGIOUTresponseHeader();

		void	validateCGIOUTresponse();

		// ------------------------------

		// FORCGIIN

		//---------------------------------

		bool _isFinishedRead;
		bool _keepConnection;
	public:
		HttpCgi();
		HttpCgi(const HttpCgi& obj);

		HttpCgi(std::list<HttpResponse>* clientResponseList,
		HttpResponse* cgiTargetResponse,
		const FileDescriptor& mainHttpSocket,
		Socket *cgiOutSocket);
		HttpCgi(std::list<HttpResponse>* clientResponseList,
		HttpResponse* cgiTargetResponse,
		const FileDescriptor& mainHttpSocket,
		Socket *cgiOutSocket, Socket* cgiInSocket, const FileDescriptor& tempReadFileFd);

		HttpCgi& operator=(const HttpCgi& obj); // declare but no implement
		~HttpCgi();

		bool isKeepConnection() const;


		void sendToCGI();
		void readFromCGI();
};

#endif
