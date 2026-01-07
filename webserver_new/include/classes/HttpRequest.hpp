#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <string>
# include <cstdlib>
# include "../defined_value.hpp"

enum e_http_request_status {
	NO_STATUS,
	ERR_STATUS,
	READING_HEADER,
	READING_BODY,
	COMPLETE
};

class HttpRequest {
	private:
		std::string _requestBuffer;
		e_http_request_status	_status;
		int						_errorCode;
		t_config_map _headerField;

	public:
		HttpRequest();
		HttpRequest(const HttpRequest& obj);
		HttpRequest& operator=(const HttpRequest& obj);
		~HttpRequest();

		void	processingReadBuffer(const std::string& readBuffer, ssize_t readAmount);
		e_http_request_status	getStatus() const;

};

#endif