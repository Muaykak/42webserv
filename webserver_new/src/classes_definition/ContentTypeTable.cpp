#include "../../include/classes/ContentTypeTable.hpp"

ContentTypeTable::ContentTypeTable()
{
	_extensionToContentTypeMap["html"] = "text/html";
	_extensionToContentTypeMap["htm"] = "text/html";
	_extensionToContentTypeMap["css"] = "text/css";
	_extensionToContentTypeMap["txt"] = "text/plain";
	_extensionToContentTypeMap["jpg"] = "image/jpeg";
	_extensionToContentTypeMap["png"] = "image/png";
	_extensionToContentTypeMap["gif"] = "image/gif";
	_extensionToContentTypeMap["js"] = "application/javasvript";
	_extensionToContentTypeMap["pdf"] = "application/pdf";
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