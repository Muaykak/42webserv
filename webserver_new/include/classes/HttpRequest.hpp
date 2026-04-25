#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include "./HttpResponse.hpp"
# include "./ServerConfig.hpp"

struct s_http_request_header_data {
	std::string	method;
	std::string	requestTarget;
	std::string	protocol;
	std::map<std::string, std::string> headerField;

	/* because of local redirection, maybe i should 
	treat http request as list but idk, let's try to preocess
	
	In the normal stream, you'll have to read request by request
	and there is ResponseList we will send back to client by list
	from oldest to newest orderly. But local redirect makes us
	needing to reprocess the request data again to new request target

	so, may by the step of validatoin or execute the actual
	request can be done with deque
	*/
};

enum e_http_process_status {
	NO_STATUS,
	READING_REQUEST_LINE,
	READING_HEADER,
	VALIDATING_REQUEST,
	READ_BODY,
	FINISHED_READ_BODY
};

struct s_http_request_body_data 
{
	bool			isMultiForm;
	std::string		boundaryString;
	std::string		bodyContentType;
	bool			isUseTempFile;
	unsigned int	tempRequestBodyFileNum;
	bool			checkExpectBody;
	bool			readBody;
	bool			discardBody; // whether to drain down the body
	std::vector<FileDescriptor> bodyFd; // convention to fd

	int				body_type;
	size_t			body_size;
	bool			chunkedBodyHasTrailerHeader;
	bool			chunkedBodyIsFinished;
	std::set<std::string> trailerHeader;
	size_t			client_max_body_size;
	size_t			curr_body_read;
};

struct s_http_request_cgi_data
{
	std::string cgiScriptPath;
	std::string cgiVirtualPath;
	std::string cgiPathTranslated;
	std::string cgiPath;
};

struct s_http_request_server_data
{
	const ServerConfig* targetServerPtr;
	const t_config_map* targetLocationBlockPtr;
	std::string	serverName;
	std::string portName;
};

struct s_http_request_target_data
{
		std::string targetPath;
		std::string combinedPath;
		std::string queryString;

		std::string uploadStorePath;
		std::string authorityPart;
		std::string redirectPath;
};

class HttpRequest
{
	private:

	public:
		HttpRequest();
		HttpRequest(const HttpRequest &obj);
		HttpRequest& operator=(const HttpRequest& obj);

		~HttpRequest();

		e_http_process_status	processStatus;

		HttpResponse*	responseTargetPtr;
		s_http_request_header_data	requestData;
		s_http_request_server_data serverData;
		s_http_request_target_data	targetData;
		s_http_request_cgi_data cgiData;
		s_http_request_body_data bodyData;

		int localRedirectCount;

		void clear();

};

#endif