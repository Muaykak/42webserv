# include "../../include/utility_function.hpp"

std::string size_t_to_hex(size_t toConvert)
{
	static const char hexNum[] = "0123456789ABCDEF";

	if (toConvert == 0)
	{
		return ("0");
	}

	size_t	strHexLen = 1;
	size_t	temp = toConvert;
	while (temp / 16 > 0)
	{
		++strHexLen;
		temp /= 16;
	}

	std::string returnStr;
	returnStr.append(strHexLen, '0');

	while (strHexLen > 0)
	{
		returnStr[strHexLen - 1] = hexNum[toConvert % 16];
		--strHexLen;
		toConvert /= 16;
	}

	return (returnStr);
}
