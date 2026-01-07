#include "../../include/classes/Http.hpp"
#include "../../include/classes/Socket.hpp"


Http::Http() {
	_readBuffer.resize(HTTP_RECV_BUFFER);
}
Http::Http(const Http& obj) : _requestData(obj._requestData), _responseBuffer(obj._responseBuffer){}
Http& Http::operator=(const Http& obj){
	_requestData = obj._requestData;
	_responseBuffer = obj._responseBuffer;
	return (*this);
}
Http::~Http(){}

// return bool to indicate whether server should remove this client connection from the sockets
// false means to close connection
bool Http::readFromClient(const Socket& clientSocket, std::map<int, Socket>& socketMap){

	ssize_t	readAmount;
	while (true){
		readAmount = recv(clientSocket.getSocketFD(), &_readBuffer[0], HTTP_RECV_BUFFER, 0);

		if (readAmount == 0){
			return (false);
		}
		if (readAmount < 0){
			if (errno == EINTR)
				continue;
				
		}
	}
	return (true);
}

// return bool to indicate whether server should remove this client connection from the sockets
// false means to close connection
bool Http::writeToClient(const Socket& clientSocket, std::map<int, Socket>& socketMap){
	return (true);
}