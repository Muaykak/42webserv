#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <string>

class HttpRequest {
	private:
		std::string _requestBuffer;
		bool		_isReady;
	public:
		HttpRequest();
		HttpRequest(const HttpRequest& obj);
		HttpRequest& operator=(const HttpRequest& obj);
		~HttpRequest();

};

#endif