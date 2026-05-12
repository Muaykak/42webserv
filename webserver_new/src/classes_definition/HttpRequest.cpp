#include "../../include/classes/HttpRequest.hpp"

HttpRequest::HttpRequest()
: processStatus(NO_STATUS),
responseTargetPtr(NULL)
{
	serverData.targetLocationBlockPtr = NULL;
	serverData.targetLocationBlockPtr = NULL;

	bodyData.body_size = 0;
	bodyData.body_type = 0;
	bodyData.checkExpectBody = false;
	bodyData.chunkedBodyHasTrailerHeader = false;
	bodyData.chunkedBodyIsFinished = false;
	bodyData.client_max_body_size = 0;
	bodyData.curr_body_read = 0;
	bodyData.discardBody = false;
	bodyData.readBody = false;
	bodyData.isUseTempFile = false;
	bodyData.tempRequestBodyFileNum = 0;
	localRedirectCount = 0;
}

HttpRequest::HttpRequest(const HttpRequest &obj)
: processStatus(obj.processStatus),
responseTargetPtr(obj.responseTargetPtr),
requestData(obj.requestData),
serverData(obj.serverData),
targetData(obj.targetData),
cgiData(obj.cgiData),
bodyData(obj.bodyData),
localRedirectCount(obj.localRedirectCount)
{

}

HttpRequest& HttpRequest::operator=(const HttpRequest& obj)
{
	if (this != &obj)
	{
		this->clear();

		processStatus = obj.processStatus;
		localRedirectCount = obj.localRedirectCount;
		responseTargetPtr = obj.responseTargetPtr;
		requestData = obj.requestData;
		serverData = obj.serverData;
		targetData = obj.targetData;
		cgiData = obj.cgiData;
		bodyData = obj.bodyData;
	}
	return (*this);
}

HttpRequest::~HttpRequest()
{

}

void HttpRequest::clear()
{
	/* clean all and ready for the next request*/
	processStatus = NO_STATUS;
	responseTargetPtr = NULL;
	
	requestData.headerField.clear();
	requestData.method.clear();
	requestData.protocol.clear();
	requestData.requestTarget.clear();

	serverData.portName.clear();
	serverData.serverName.clear();
	serverData.targetLocationBlockPtr = NULL;
	serverData.targetServerPtr = NULL;

	targetData.authorityPart.clear();
	targetData.cutPath.clear();
	targetData.combinedPath.clear();
	targetData.queryString.clear();
	targetData.redirectPath.clear();
	targetData.targetPath.clear();
	targetData.uploadStorePath.clear();

	cgiData.cgiPath.clear();
	cgiData.cgiPathTranslated.clear();
	cgiData.cgiScriptPath.clear();
	cgiData.cgiVirtualPath.clear();

	bodyData.multiformData.clear();
	bodyData.bodyContentType.clear();
	bodyData.writeBodyFile.clear();
	bodyData.trailerHeader.clear();
	bodyData.body_size = 0;
	bodyData.body_type = 0;
	bodyData.checkExpectBody = false;
	bodyData.chunkedBodyHasTrailerHeader = false;
	bodyData.chunkedBodyIsFinished = false;
	bodyData.client_max_body_size = 0;
	bodyData.curr_body_read = 0;
	bodyData.discardBody = false;
	bodyData.isUseTempFile = false;
	bodyData.readBody = false;
	bodyData.tempRequestBodyFileNum = 0;

	localRedirectCount = 0;
}
