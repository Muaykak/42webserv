#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include "../defined_value.hpp"
# include <list>

class HttpResponse {
	private:

		bool	isCgiAlive;
		bool	isComplete;

		// if all Buffer is sent. the response has nothing
		// to send
		std::list<s_response_buff> _bufferList;


	public:
		HttpResponse();
		HttpResponse(int errorCode);
};

#endif
