#include "../../include/utility_function.hpp"
#include <cstdlib>

// 0xFFFFFFFF treated as error
bool	string_to_in_addr_t(const std::string& toConvert, in_addr_t& returnValue)
{
	// check that string consist only 0-9, '.'

	if (hostipChar().isMatch(toConvert) == false)
		return (false);
	
	std::string	tempStr;
	size_t	sepPos = 0;
	size_t	curPos = 0;
	int		tempNum;
	in_addr_t	tempAddr = 0;
	unsigned int	currI = 0;
	while (currI < 4 && curPos < toConvert.size())
	{
		sepPos = toConvert.find_first_of('.', curPos);
		if (sepPos == curPos || (sepPos == toConvert.npos && currI != 3) || (sepPos != toConvert.npos && currI == 3))
				return (false);

		if (sepPos != toConvert.npos)
			tempStr = toConvert.substr(curPos, sepPos - curPos);
		else
			tempStr = toConvert.substr(curPos);

		if (tempStr.size() > 3 || (tempStr.size() > 1 && tempStr[0] == '0'))
			return (false);
			
		tempNum = std::atoi(tempStr.c_str());

		if (tempNum > 255)
			return (false);
		
		tempAddr |= (static_cast<unsigned int>(tempNum)) << (currI * 8);
		
		currI++;
		curPos = sepPos == toConvert.npos ? toConvert.size() : sepPos + 1;
	}

	if (currI != 4)
		return (false);
	returnValue = tempAddr;
	return (true);
}

// unsigned int to ascii
// use std::string
static std::string	my_utoa(unsigned int number)
{
	std::string	returnStr;

	size_t	digit = 1;
	size_t	digitcount = 1;
	while (number / digit >= 10)
	{
		digitcount++;
		digit *= 10;
	}
	returnStr.reserve(digitcount);
	while (digit > 0)
	{
		returnStr += static_cast<char>((number / (digit)) + 48);
		number = number % digit;
		digit /= 10;
	}	
	return (returnStr);
}

std::string in_addr_t_to_string(in_addr_t toConvert)
{
	std::string returnStr;

	returnStr.reserve(16);
	unsigned int i = 0;
	while (i < 4)
	{
		if (i > 0)
			returnStr += '.';
		returnStr += my_utoa((toConvert >> (i << 3)) & 0xFF);
		i++;
	}

	return (returnStr);
}
