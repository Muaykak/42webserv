#include "../../include/classes/HttpCgiData.hpp"


HttpCgiData::HttpCgiData(){}
HttpCgiData::HttpCgiData(const HttpCgiData& obj) : _writeToCgiBuffer(obj._writeToCgiBuffer),
_readFromCgiBuffer(obj._readFromCgiBuffer){}
HttpCgiData&	HttpCgiData::operator=(const HttpCgiData& obj){
	if (this != &obj){
		_writeToCgiBuffer = obj._writeToCgiBuffer;
		_readFromCgiBuffer = obj._readFromCgiBuffer;
	}
	return (*this);
}
HttpCgiData::~HttpCgiData(){}