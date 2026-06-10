#ifndef HTTP_HPP
# define HTTP_HPP

# include <list>
# include <map>
# include <set>
# include <deque>
# include "../defined_value.hpp"
# include "./ServerConfig.hpp"
# include "./HttpThrowStatus.hpp"
# include "./HttpResponse.hpp"
# include "./HttpRequest.hpp"

class Socket;

enum e_http_field_value_token_type
{
	TOKEN_UNKNOWN_TYPE,
	WORD,
	ESCAPE_CHAR,
	DELIMITER,
	OPTIONAL_CHAR
};


struct s_http_field_value_token {
	e_http_field_value_token_type tokenType;
	std::string str;
};


class Http {
	private:
		Socket* _clientSocketPtr;
		std::map<int, Socket>* _socketMapPtr;
		std::vector<char>	_recvBuffer;
		std::string _requestBuffer;
		size_t		_requestBufferOffset;

		bool	_keepConnection;
		bool	_isEpollout;

		std::list<HttpResponse>	_httpResponseList;

		HttpRequest httpRequest;

		/* small functions */
		// void (Http::*readingRequestBodyPtr)(HttpRequest& requestData);

		/*
		return value:

		0 = false, error occurred, usually define
			the error code to _errorStatusCode (no error will default set to -1)
			may skip to create response or just close connection.
		
		4 = false, error occured, but it is possible to response error and continue
		to next request
		

		1 = success, can continue to next procedure

		2 = need to wait for new buffer (wait for next EPOLLIN)

		NOTE: return value = 0 usually means ready to
			create response (whether it is fail or success)
		*/

		void	processingRequestBuffer(const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void		parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize);
		void		parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize);

		void		validateRequestData(HttpRequest& requestData, const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void			validateRequestBufferSelectServer(HttpRequest& requestData, const Socket& clientSocket, const std::string& authStr);
		void				checkHost(HttpRequest& requestData, std::string& hoststr);
		void			requestLineCheck(HttpRequest& requestData, const Socket& clientSocket);
		void				requestLineCheckProtocolVersion(HttpRequest& requestData);
		void				requestLineCheckRequestTarget(HttpRequest& requestData, const Socket& clientSocket);
		void					requestLineCheckRequestTargetAbsolute(HttpRequest& requestData, const Socket& clientSocket);
		void					requestLineCheckRequestTargetPathCheck(HttpRequest& requestData);
		void			targetLocationBlockGet(HttpRequest& requestData);
		void			checkRequestBodyType(HttpRequest& requestData);
		void				checkExpectBody(HttpRequest& requestData);
		bool			checkRedirection(HttpRequest& requestData);
		void			checkConnectionHeader(HttpRequest& requestData);
		void			checkMethod(HttpRequest& requestData);
		void			appendTargetPath(HttpRequest& requestData, bool isEndWithSlash);
		void			checkCgiPath(HttpRequest& requestData);
		void			buildCombinedPath(HttpRequest& requestData);
		void			createSystemPath(HttpRequest& requestData, std::string& systemPath);
		void			handleDeleteRequest(HttpRequest& requestData);
		void			handleGetRequest(HttpRequest& requestData, bool isEndWithSlash, const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void			handlePostRequest(HttpRequest& requestData);
		void				handleUploadPostRequest(HttpRequest& requestData);
		void					handleRequestMultipart(HttpRequest& requestData, std::pair<std::string, std::vector<std::pair<std::string, std::string> > >& outPair);
		// void					handleUploadOctetStream(HttpRequest& requestData);
		// void					handleUploadFoundType(HttpRequest& requestData, std::pair<std::string, std::vector<std::pair<std::string, std::string> > >& outPair);
		void					handleNormalUpload(HttpRequest &requestData, std::pair<std::string, std::vector<std::pair<std::string, std::string> > >& outPair);
		void			handlePostCgiRequest(HttpRequest& requestData);

		void		readingRequestBody(HttpRequest& requestData);
		void			readingContentLengthBody(HttpRequest& requestData);
		void			readingTranferEncodingBody(HttpRequest& requestData);
		int				readTransferEncodingChunkFinished(HttpRequest& requestData, size_t& endLinePos);
		int				readTransferEncodingChunk(HttpRequest& requestData, size_t& endLinePos);
		void		processingRequestBody(HttpRequest& requestData, const Socket& clientSocket, std::map<int, Socket>& socketMap);
		void			processMultiFormData(HttpRequest& requestData);
		void				multiformDataProcessBuffer(HttpRequest& requestData, std::string& multipartBufferString);
		void					multiformDataFindingBoundary(HttpRequest& requestData, std::string& multipartBufferString, size_t& currBufpos);
		void					multiformDataReadingHeader(HttpRequest& requestData, std::string& multipartBufferString, size_t& currBufpos);
		void					multiformDataValidating(HttpRequest& requestData);
		void					multiformDataReadBody(HttpRequest& requestData, std::string& multipartBufferString);

		void	cgiChildProcessNoRequestBody(HttpRequest& requestData, int pipeForCgiStdOut[2]);


		void	printHeaderField(HttpRequest& requestData) const;

		void	generate4xx5xxErrorReponse(HttpRequest& requestData, unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg);
		void	generate4xx5xxErrorReponseChildProcess(HttpRequest& requestData, unsigned int errorStatusCode, bool keepAfterResponse, const std::string& throwMsg);
		void	generate3xxRedirectResponse(HttpRequest& requestData, unsigned int statusCode);

	public:
		Http();
		Http(Socket *clientSocketPtr, std::map<int, Socket>* socketMapPtr);
		Http(const Http& obj);
		~Http();

		void	readFromClient();
		void	writeToClient();

		void	directRequestProcess(HttpRequest requestData);
		void	send408();

		bool	isKeepConnection() const;

		static bool extractHttpFieldValueString(const std::string& toExtract, std::deque<s_http_field_value_token>& tokenList, const std::string& delimiterCharSet, const std::string& optionalSpaceCharSet);
		static bool httpFieldNormalSingletonTrim(const std::string& toExtract, std::string& outString);
		static bool httpFieldNormalCommaElement(const std::string& toExtract, std::vector<std::string>& outVec);
		static bool httpFieldContentTypeExtract(const std::string& toExtract, std::pair<std::string, std::vector<std::pair<std::string, std::string> > > & outPair);
};

#endif
