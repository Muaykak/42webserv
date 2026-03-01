#ifndef HTTPCGI_HPP
# define HTTPCGI_HPP

# include "Http.hpp"

class HttpCgi {
	private:
		bool*	_cgiProcessOpen;
		std::list<HttpResponse>* _httpResponseList;




	public:
		HttpCgi();
		HttpCgi(const HttpCgi& obj);

		HttpCgi(bool* cgiProcOpenPtr,
		std::list<HttpResponse>* httpResponseListPtr);

		HttpCgi& operator=(const HttpCgi& obj);
		~HttpCgi();
};

#endif
