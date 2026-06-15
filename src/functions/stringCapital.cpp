
#include "../../include/utility_function.hpp"

/*  change "hello world" to "Hello World" if you know what i mean*/
void stringCapital(std::string& toConvert)
{
	if (toConvert.empty())
		return ;
	
	toConvert = stringToLower(toConvert);
	
	size_t currPos = allAlphaChar().findFirstCharset(toConvert);
	size_t endWordPos;
	while (currPos != std::string::npos)
	{
		toConvert[currPos] = static_cast<unsigned char>(std::toupper(toConvert[currPos]));

		endWordPos = allAlphaChar().findFirstNotCharset(toConvert, currPos);

		if (endWordPos == std::string::npos)
			break ;
		
		currPos = allAlphaChar().findFirstCharset(toConvert, endWordPos);
	}

	return ;
}
