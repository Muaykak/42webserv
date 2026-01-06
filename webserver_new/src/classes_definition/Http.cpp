#include "../../include/classes/Http.hpp"
#include "../../include/classes/Socket.hpp"


Http::Http() {}
Http::Http(const Http& obj) : _requestData(obj._requestData), _responseBuffer(obj._responseBuffer){}
Http& Http::operator=(const Http& obj){
	_requestData = obj._requestData;
	_responseBuffer = obj._responseBuffer;
	return (*this);
}
Http::~Http(){}

// return bool to indicate whether server should remove this client connection from the sockets
// false means to close connection
bool Http::request(const Socket& clientSocket, std::map<int, Socket>& socketMap){
	return (true);
}

// return bool to indicate whether server should remove this client connection from the sockets
// false means to close connection
bool Http::response(const Socket& clientSocket, std::map<int, Socket>& socketMap){
	return (true);
}