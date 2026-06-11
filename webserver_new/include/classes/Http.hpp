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

		void	processingRequestBuffer();
		void		parsingHttpRequestLine(size_t& currIndex, size_t& reqBuffSize);
		void		parsingHttpHeader(size_t& currIndex, size_t& reqBuffSize);

		static std::map<std::string, std::pair<void (Http::*)(HttpRequest&), void (Http::*)(HttpRequest &)> > buildHttpMethodMap();
		static const std::pair<void (Http::*)(HttpRequest&), void (Http::*)(HttpRequest &)> *getHttpMethodFunc(const std::string& methodStr);
		static const std::pair<void (Http::*)(HttpRequest&), void (Http::*)(HttpRequest &)> *getHttpCGIFunc();
		static const std::pair<void (Http::*)(HttpRequest&), void (Http::*)(HttpRequest &)> *getHttpRedirectFunc();

		void		validateRequestData(HttpRequest& requestData);
		void			validateRequestBufferSelectServer(HttpRequest& requestData, const std::string& authStr);
		void				checkHost(HttpRequest& requestData, std::string& hoststr);
		void			requestLineCheck(HttpRequest& requestData);
		void				requestLineCheckProtocolVersion(HttpRequest& requestData);
		void				requestLineCheckRequestTarget(HttpRequest& requestData);
		void					requestLineCheckRequestTargetAbsolute(HttpRequest& requestData);
		void					requestLineCheckRequestTargetPathCheck(HttpRequest& requestData);
		void			targetLocationBlockGet(HttpRequest& requestData);
		void			checkRequestBodyType(HttpRequest& requestData);
		void				checkExpectBody(HttpRequest& requestData);
		bool			checkRedirection(HttpRequest& requestData);
		void			checkConnectionHeader(HttpRequest& requestData);
		void			checkMethod(HttpRequest& requestData);
		void			appendTargetPath(HttpRequest& requestData);
		void			checkCgiPath(HttpRequest& requestData);

			// will remove soon!!
			//void			createSystemPath(HttpRequest& requestData, std::string& systemPath);

		void				createSystemPathUploadStore(HttpRequest& requestData);
		void				createSystemPathRoot(HttpRequest& requestData);
		void					createSystemPathAddPWD(std::string& systemPath);

		//void			buildCombinedPath(HttpRequest& requestData);
		void			buildCombinedPathCgi(HttpRequest& requestData);
		void			buildCombinedPathNormal(HttpRequest& requestData);

		//void			handleGetRequest(HttpRequest& requestData, bool isEndWithSlash, const Socket& clientSocket, std::map<int, Socket>& socketMap);
		//void			handlePostRequest(HttpRequest& requestData);
		void				handleUploadPostRequest(HttpRequest& requestData);
		void					handleRequestMultipart(HttpRequest& requestData, std::pair<std::string, std::vector<std::pair<std::string, std::string> > >& outPair);
		// void					handleUploadOctetStream(HttpRequest& requestData);
		// void					handleUploadFoundType(HttpRequest& requestData, std::pair<std::string, std::vector<std::pair<std::string, std::string> > >& outPair);
		void					handleNormalUpload(HttpRequest &requestData, std::pair<std::string, std::vector<std::pair<std::string, std::string> > >& outPair);
		void			handlePostCgiRequest(HttpRequest& requestData);
		void			validateCgiRequest(HttpRequest& requestData);
		void			validateHeadRequest(HttpRequest& requestData);
		void			validateGetRequest(HttpRequest& requestData);
		void			validatePostRequest(HttpRequest& requestData);
		void			validateDeleteRequest(HttpRequest& requestData);

		void		readingRequestBody(HttpRequest& requestData);
		void			readingContentLengthBody(HttpRequest& requestData);
		void			readingTranferEncodingBody(HttpRequest& requestData);
		int				readTransferEncodingChunkFinished(HttpRequest& requestData, size_t& endLinePos);
		int				readTransferEncodingChunk(HttpRequest& requestData, size_t& endLinePos);
		void		processingRequest(HttpRequest& requestData);
		void			handleCGIrequest(HttpRequest& requestData);
		void			handleRedirectRequest(HttpRequest& requestData);
		void			handleGetRequest(HttpRequest& requestData);
		void			handlePostRequest(HttpRequest& requestData);
		void			handleDeleteRequest(HttpRequest& requestData);
		void			handleHeadRequest(HttpRequest& requestData);

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
		bool	hasResponseList() const;

		static bool extractHttpFieldValueString(const std::string& toExtract, std::deque<s_http_field_value_token>& tokenList, const std::string& delimiterCharSet, const std::string& optionalSpaceCharSet);
		static bool httpFieldNormalSingletonTrim(const std::string& toExtract, std::string& outString);
		static bool httpFieldNormalCommaElement(const std::string& toExtract, std::vector<std::string>& outVec);
		static bool httpFieldContentTypeExtract(const std::string& toExtract, std::pair<std::string, std::vector<std::pair<std::string, std::string> > > & outPair);
};

#endif
