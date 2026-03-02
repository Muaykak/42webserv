#ifndef HTTPCGI_HPP
# define HTTPCGI_HPP

# include "Http.hpp"

class HttpCgi {
	private:
		Http*	_clientSocketHttp;

		// need the parent client socket to set the
		// epoll_ctl()
		FileDescriptor	_mainHttpSocketFd;

		std::vector<char> _readCgiBuffer;


	public:
		HttpCgi();
		HttpCgi(const HttpCgi& obj);

		HttpCgi(Http* clientSocketHttp, const FileDescriptor& mainHttpSocketFd);

		HttpCgi& operator=(const HttpCgi& obj); // declare but no implement
		~HttpCgi();

		Http* getClientSocketHttp() const;

		void sendToCGI();
		void readFromCGI();
};

#endif
