#ifndef HTTPCGI_HPP
# define HTTPCGI_HPP

# include "Http.hpp"

class HttpCgi {
	private:
		std::list<HttpResponse>	*_clientResponseList;
		HttpResponse	*_cgiTargetResponse;
		Socket	*_thisCgiSocket;

		// need the parent client socket to set the
		// epoll_ctl()
		FileDescriptor	_mainHttpSocketFd;

		std::vector<char> _readCgiBuffer;
		std::string _responseBuffer;
		
		bool _isFinishedRead;
		bool _isFinishedWrite;

	public:
		HttpCgi();
		HttpCgi(const HttpCgi& obj);

		HttpCgi(std::list<HttpResponse>* clientResponseList,
		HttpResponse* cgiTargetResponse,
		const FileDescriptor& mainHttpSocket,
		Socket *thisCgiSocket);

		HttpCgi& operator=(const HttpCgi& obj); // declare but no implement
		~HttpCgi();

		void sendToCGI();
		void readFromCGI();
};

#endif
