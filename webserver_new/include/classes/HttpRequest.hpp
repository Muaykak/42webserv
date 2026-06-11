#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include "./HttpResponse.hpp"
# include "./ServerConfig.hpp"

class HttpRequest;
class Http;

struct s_http_request_header_data {
	std::string	method;
	std::string	requestTarget;
	std::string	protocol;
	std::map<std::string, std::string> headerField;
	size_t headerBufferCount;

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

enum e_http_multipart_formdata_state
{
	FORMDATA_STATUS_FINDING_BOUNDARY,
	FORMDATA_STATUS_READING_HEADER,
	FORMDATA_STATUS_VALIDATING_HEADER,
	FORMDATA_STATUS_READING_BODY,
	FORMDATA_STATUS_NEXT_BLOCK,
	FORMDATA_STATUS_FINISHED
};

struct s_http_request_body_multiform_data
{
	std::string boundaryString;
	std::list<std::string> allCreatedFileList;

	e_http_multipart_formdata_state state;
	std::map<std::string, std::string> headerField;
	OptionalData<std::string> combinedPathFileName;
	OptionalData<FileDescriptor> fileFd;
};

struct s_http_request_body_data 
{
	//bool			isMultiForm;
	//std::string		boundaryString;
	OptionalData<s_http_request_body_multiform_data> multiformData;

	std::string		bodyContentType;
	bool			isUseTempFile;
	unsigned int	tempRequestBodyFileNum;
	bool			checkExpectBody;
	bool			readBody;
	bool			discardBody; // whether to drain down the body
	OptionalData<FileDescriptor> writeBodyFile;

	void (Http::*readingRequestBodyPtr)(HttpRequest& requestData);
	size_t			body_size;
	bool			chunkedBodyHasTrailerHeader;
	bool			chunkedBodyIsFinished;
	std::set<std::string> trailerHeader;
	size_t			client_max_body_size;
	size_t			curr_body_read;

	bool			skipCRLFtranferencoding;
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
		std::string cutPath;
		std::string targetPath;
		bool		isEndWithSlash;
		bool		isUseIndex;
		std::string combinedPath;
		std::string queryString;

		std::string uploadStorePath;
		std::string authorityPart;
		std::string redirectPath;

		std::string systemPath;
};

class HttpRequest
{
	private:
		e_http_process_status	processStatus;
		std::time_t		state_timeStamp;

	public:
		HttpRequest();
		HttpRequest(const HttpRequest &obj);
		HttpRequest& operator=(const HttpRequest& obj);

		~HttpRequest();

		HttpResponse*	responseTargetPtr;
		s_http_request_header_data	requestData;
		s_http_request_server_data serverData;
		s_http_request_target_data	targetData;
		s_http_request_cgi_data cgiData;
		s_http_request_body_data bodyData;

		const std::pair<void (Http::*)(HttpRequest&), void (Http::*)(HttpRequest&)> *methodExecuteFuncPtr;

		int localRedirectCount;

		void clear();
		void setProcessStatus(e_http_process_status status);
		e_http_process_status getProcessStatus() const;
		std::time_t getStateTimeStamp() const;
};

#endif
