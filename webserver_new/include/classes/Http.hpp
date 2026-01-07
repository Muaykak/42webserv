#ifndef HTTP_HPP
# define HTTP_HPP

# include "HttpCgiData.hpp"
# include "HttpRequest.hpp"
# include <list>
# include <map>

class Socket;

class Http {
	private:
		std::list<HttpRequest>	_requestData;
		std::string	_responseBuffer;
		HttpCgiData	_cgiData;
		std::string	_readBuffer;

	public:
		Http();
		Http(const Http& obj);
		Http& operator=(const Http& obj);
		~Http();

		// return bool to indicate whether server should remove this client connection from the sockets
		// false means to close connection
		bool	readFromClient(const Socket& clientSocket, std::map<int, Socket>& socketMap);

		// return bool to indicate whether server should remove this client connection from the sockets
		// false means to close connection
		bool	writeToClient(const Socket& clientSocket, std::map<int, Socket>& socketMap);
};

#endif