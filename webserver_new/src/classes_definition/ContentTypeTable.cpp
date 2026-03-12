#include "../../include/classes/ContentTypeTable.hpp"

ContentTypeTable::ContentTypeTable()
{
	_extensionToContentTypeMap["html"] = "text/html";
	_contentTypeMapToExtension["text/html"].insert("html");

	_extensionToContentTypeMap["htm"] = "text/html";
	_contentTypeMapToExtension["text/html"].insert("htm");

	_extensionToContentTypeMap["css"] = "text/css";
	_contentTypeMapToExtension["text/css"].insert("css");

	_extensionToContentTypeMap["txt"] = "text/plain";
	_contentTypeMapToExtension["text/plain"].insert("txt");

	_extensionToContentTypeMap["jpg"] = "image/jpeg";
	_contentTypeMapToExtension["image/jpeg"].insert("jpg");

	_extensionToContentTypeMap["jpeg"] = "image/jpeg";
	_contentTypeMapToExtension["image/jpeg"].insert("jpeg");

	_extensionToContentTypeMap["png"] = "image/png";
	_contentTypeMapToExtension["image/png"].insert("png");

	_extensionToContentTypeMap["gif"] = "image/gif";
	_contentTypeMapToExtension["image/gif"].insert("gif");

	_extensionToContentTypeMap["js"] = "application/javasvript";
	_contentTypeMapToExtension["application/javasvript"].insert("js");

	_extensionToContentTypeMap["pdf"] = "application/pdf";
	_contentTypeMapToExtension["application/pdf"].insert("pdf");

}

std::string ContentTypeTable::extensionToContentType(const std::string& filePath) const
{
	std::string extractedExt;
	{
		size_t pos = filePath.find_last_of('.');

		if (pos == filePath.npos)
		{
			return ("applcation/octet-stream");
		}

		if (pos + 1 < filePath.size())
			extractedExt = filePath.substr(pos + 1);
	}

	if (extractedExt.empty())
	{
		return ("applcation/octet-stream");
	}

	std::map<std::string, std::string>::const_iterator foundIt = _extensionToContentTypeMap.find(extractedExt);

	if (foundIt == _extensionToContentTypeMap.end())
		return ("applcation/octet-stream");
	else
		return (foundIt->second);
}


const std::set<std::string>& ContentTypeTable::contentTypeToExtension(const std::string& contentType) const
{
	std::map<std::string, std::set<std::string> >::const_iterator foundContentType	= _contentTypeMapToExtension.find(contentType);

	if (foundContentType == _contentTypeMapToExtension.end())
	{
		return (_fallbackEmptySet);
	}
	else
	{
		return (foundContentType->second);
	}
}