#include "../../include/classes/HttpRequest.hpp"


HttpRequest::HttpRequest(): _isReady(false){
}
HttpRequest::HttpRequest(const HttpRequest& obj){

}
HttpRequest& HttpRequest::operator=(const HttpRequest& obj){
	if (this != &obj){

	}
	return (*this);
}
HttpRequest::~HttpRequest(){
}