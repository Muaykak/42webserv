#include <string>
#include <cctype>

// convert the given string to 
std::string	stringToLower(const std::string& toConvert)
{
	std::string returnStr;

	if (&toConvert == NULL || toConvert.empty())
		return returnStr;
	
	returnStr = toConvert;
	for (size_t i = 0; i < returnStr.size(); i++)
	{
		if (returnStr[i] >= 'A' && returnStr[i] <= 'Z')
			returnStr[i] = static_cast<char>(std::tolower(returnStr[i]));
	}
	return (returnStr);
}
std::string	stringToUpper(const std::string& toConvert)
{
	std::string returnStr;

	if (&toConvert == NULL || toConvert.empty())
		return returnStr;
	
	returnStr = toConvert;
	for (size_t i = 0; i < returnStr.size(); i++)
	{
		if (returnStr[i] >= 'A' && returnStr[i] <= 'Z')
			returnStr[i] = static_cast<char>(std::toupper(returnStr[i]));
	}
	return (returnStr);
}
