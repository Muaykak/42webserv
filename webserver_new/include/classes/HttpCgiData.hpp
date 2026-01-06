#ifndef HTTPCGIDATA_HPP
# define HTTPCGIDATA_HPP

# include "FileDescriptor.hpp"

class HttpCgiData {
	private:
		std::string		_writeToCgiBuffer;
		std::string		_readFromCgiBuffer;
		FileDescriptor	_writeFd;
		FileDescriptor	_readFD;
	public:
		HttpCgiData();
		HttpCgiData(const HttpCgiData& obj);
		HttpCgiData&	operator=(const HttpCgiData& obj);
		~HttpCgiData();

};

#endif