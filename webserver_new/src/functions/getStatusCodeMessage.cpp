#include "../../include/utility_function.hpp"

static std::map<unsigned int, std::string> buildStatusCodeMap()
{
	std::map<unsigned int, std::string> resMap;

	resMap[100] = "Continue";
	resMap[200] = "OK";
	resMap[201] = "Created";
	resMap[202] = "Accepted";
	resMap[204] = "No Content";
	resMap[301] = "Moved Permanently";
	resMap[302] = "Found";
	resMap[400] = "Bad Request";
	resMap[401] = "Unauthorized";
	resMap[403] = "Forbidden";
	resMap[404] = "Not Found";
	resMap[405] = "Method Not Allowed";
	resMap[406] = "Not Acceptable";
	resMap[408] = "Request Timeout";
	resMap[409] = "Conflict";
	resMap[410] = "Gone";
	resMap[411] = "Length Required";
	resMap[412] = "Precondition Failed";
	resMap[413] = "Content Too Large";
	resMap[414] = "URI Too Long";
	resMap[415] = "Unsupported Media Type";
	resMap[416] = "Range Not Satisfiable";
	resMap[417] = "Expectation Failed";
	resMap[421] = "Misdirected Request";
	resMap[422] = "Unprocessable Content";
	resMap[426] = "Upgrade Required";

	resMap[500] = "Internal Server Error";
	resMap[501] = "Not Implemented";
	resMap[502] = "Bad Gateway";
	resMap[503] = "Service Unavailable";
	resMap[504] = "Gateway Timeout";
	resMap[505] = "HTTP Version Not Supported";

	return (resMap);
}

const std::string& getStatusCodeMessage(unsigned int statusCode)
{
	static std::map<unsigned int, std::string> statusCodeMap(buildStatusCodeMap());
	static std::string emptyStr("");

	std::map<unsigned int, std::string>::const_iterator found = statusCodeMap.find(statusCode);
	if (found != statusCodeMap.end())
		return (found->second);
	else
		return (emptyStr);
}
