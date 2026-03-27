
#include <iostream>
#include <string>
#include <vector>
#include "./include/utility_function.hpp"
#include "./include/classes/Http.hpp"
#include <deque>

bool extractHttpFieldValueString(const std::string& toExtract, std::deque<s_http_field_value_token>& tokenList, const std::string& delimiterCharSet, const std::string& optionalCharSet);

bool httpFieldNormalSingletonTrim(const std::string& toExtract, std::string& outString);
bool httpFieldContentTypeExtract(const std::string& toExtract, std::pair<std::string, std::vector<std::pair<std::string, std::string> > > & outPair);
bool httpFieldNormalCommaElement(const std::string& toExtract, std::vector<std::string>& outVec);

int main()
{
	{
		std::string testContentTypeString = "multipart/form-data; boundary=\"h e l l o w o r l d\";";

		std::pair<std::string, std::vector<std::pair<std::string, std::string> > > outVec;

		if (httpFieldContentTypeExtract(testContentTypeString, outVec) == false)
		{
			std::cout << "Conversion failed " << std::endl;
			return (0);
		}

		/* check the data */
		{
				std::cout << "====== Element []======" << std::endl;
				std::cout << "element name: " << outVec.first << std::endl;
				for (size_t j = 0; j < outVec.second.size(); j++)
				{
					std::cout << "== attribute[" << j + 1 << "] ==" << std::endl;
					std::cout << "KEY: " << outVec.second[j].first << " VALUE: \"" << outVec.second[j].second << "\"" << std::endl;
				}
		}
	}

	{
		std::string testSingleton = "               test         hello      ";

		std::string outString;

		std::cout << std::endl << "test httpFieldNormalSingletonTrim()" << std::endl;

		if (httpFieldNormalSingletonTrim(testSingleton, outString) == false)
			std::cout << "conversion failed" << std::endl;
		else
			std::cout << "str =>\"" << outString << "\"" << std::endl;
	}

	{
		std::cout << std::endl << "test httpFieldNormalCommaElement()" << std::endl;

		std::string toExtract = ","; /* suppose to failed */
		std::vector<std::string> outVec;

		if (httpFieldNormalCommaElement(toExtract, outVec) == false)
			std::cout << "conversion failed" << std::endl;
		else
		{
			for (size_t i = 0;i < outVec.size(); i++)
			{
				std::cout << "Element[" << i + 1 << "]: \"" << outVec[i] << "\"" << std::endl;
			}
		}
	}

}

bool httpFieldNormalCommaElement(const std::string& toExtract, std::vector<std::string>& outVec)
{
	if (!outVec.empty())
		outVec.clear();

	if (toExtract.empty())
		return (true);


	std::deque<s_http_field_value_token> tempTokenList;
	if (toExtract.find_first_of(',') == std::string::npos)
	{
		/* if no comma, just simply single ton*/
		std::string tempStringOut;
		if (httpFieldNormalSingletonTrim(toExtract, tempStringOut) == false)
			return (false);
		if (!tempStringOut.empty())
			outVec.push_back(tempStringOut);
		return (true);
	}
	else
	{
		/* here, there is a comma */
		if (extractHttpFieldValueString(toExtract, tempTokenList, ",", " \t") == false)
			return (false);

		std::deque<s_http_field_value_token>::const_iterator listIt = tempTokenList.begin();
		std::deque<s_http_field_value_token>::const_iterator headIt = tempTokenList.end();
		std::deque<s_http_field_value_token>::const_iterator tailIt;
		while (listIt != tempTokenList.end())
		{
			/* simply skip any whitespaces first? */
			if (listIt->tokenType == OPTIONAL_CHAR)
			{
				++listIt;
				continue;
			}
			else if (listIt->tokenType == WORD)
			{
				if (headIt == tempTokenList.end())
					headIt = listIt;
				++listIt;
				continue;
			}
			else if (listIt->tokenType == DELIMITER)
			{
				if (headIt != tempTokenList.end())
				{
					std::string tempStr;
					tailIt = listIt - 1;

					while (tailIt->tokenType != WORD)
						--tailIt;
					
					while (headIt != tailIt)
					{
						tempStr += headIt->str;
						++headIt;
					}
					tempStr += headIt->str;

					if (!tempStr.empty())
						outVec.push_back(tempStr);
					
					headIt = tempTokenList.end();
				}
				++listIt;
				continue;
			}
			else
				return (false);
		}

		if (headIt != tempTokenList.end())
		{
			std::string tempStr;
			tailIt = listIt - 1;

			while (tailIt->tokenType != WORD)
				--tailIt;
			
			while (headIt != tailIt)
			{
				tempStr += headIt->str;
				++headIt;
			}
			tempStr += headIt->str;

			if (!tempStr.empty())
				outVec.push_back(tempStr);
			
		}

		/* If have comma it must have at least 1 element if not then should return false */
		if (outVec.size() < 1)
			return (false);

		return (true);
	}

	return (true);
}

bool httpFieldNormalSingletonTrim(const std::string& toExtract, std::string& outString)
{
	/* for normal singleton, i just gonna trim the SP and HTAB
		** this doens't handle the quoted string */
	if (!outString.empty())
		outString.clear();

	if (toExtract.empty())
		return (true);

	size_t frontPos = htabSp().findFirstNotCharset(toExtract);

	if (frontPos == std::string::npos)
		return (true);
	
	size_t endPos = htabSp().findLastNotCharset(toExtract);

	/* End pos should not get the npos though, looking from the logic */
	if (endPos == std::string::npos)
		return(false);

	outString = toExtract.substr(frontPos, endPos - frontPos + 1);
	return (true);
}

/*
	Explaination of the outVec

	So the Content-Type value field have structure some how like this
	1. this field can have many element and separate with ',' so it is a vector
	2. for each element has its name and can its optional attributes, so it is a std::pair (its name and its attributes)
	3. Each of element can have more than one attributes so it is a vector
	4. each attributes is its name and value separated by '=' so it is a std::pair
*/
bool httpFieldContentTypeExtract(const std::string& toExtract, std::pair<std::string, std::vector<std::pair<std::string, std::string> > > & outPair)
{
	std::deque<s_http_field_value_token> tempTokenList;

	if (extractHttpFieldValueString(toExtract, tempTokenList, ";=\"", " \t") == false)
	{
		return (false);
	}

	/* Now we go and read to token List*/

	std::deque<s_http_field_value_token>::const_iterator listIt = tempTokenList.begin();

	std::deque<s_http_field_value_token>::const_iterator tempListPos = tempTokenList.end();
	std::vector<std::pair<std::string, std::string> > tempAttribVec;
	std::pair<std::string, std::string> tempAttrib;

	while (listIt != tempTokenList.end())
	{
		// here you would need to skip any of the first
		if (listIt->tokenType == OPTIONAL_CHAR)
		{
			++listIt;
			continue;
		}
		else if (listIt->tokenType == WORD)
		{
			if (outPair.first.empty())
			{
				outPair.first = listIt->str;
				if (httpContentTypeChar().isMatch(outPair.first) == false)
				{
					return (false);
				}
			}
			else
			{
				if (!tempAttrib.first.empty())
					return (false);
				tempAttrib.first = listIt->str;
				if (httpTokenChar().isMatch(tempAttrib.first) == false)
					return (false);
				tempListPos = listIt;
			}
			++listIt;
			continue;
		}
		else if (listIt->tokenType == DELIMITER)
		{
			if (listIt->str[0] == ';')
			{
				if (outPair.first.empty())
					return (false);

				if (!tempAttrib.first.empty() && tempAttrib.second.empty())
					return (false);
				
				if (!tempAttrib.first.empty())
				{
					tempAttribVec.push_back(tempAttrib);
					tempAttrib.first.clear();
					tempAttrib.second.clear();
				}
				++listIt;
				continue;
			}
			else if (listIt->str[0] == '=')
			{
				if (listIt->str.size() != 1 || outPair.first.empty() || tempAttrib.first.empty()
				|| (listIt + 1) == tempTokenList.end()
				|| tempListPos == tempTokenList.end()
				|| listIt == tempTokenList.begin()
				|| !tempAttrib.second.empty()
				|| (listIt - 1) != tempListPos
				|| ((listIt + 1)->tokenType == DELIMITER && (listIt + 1)->str[0] != '\"')
				|| ((listIt + 1)->tokenType != WORD && (listIt + 1)->tokenType != DELIMITER))
				{
					return (false);
				}
				
				tempListPos = tempTokenList.end();
				if ((++listIt)->tokenType == WORD)
				{
					tempAttrib.second = listIt->str;
					if (httpTokenChar().isMatch(tempAttrib.second) == false)
					{
						return (false);
					}
					
					listIt += 2;
					continue;
				}
				else
				{
					if (listIt->str.size() >= 2)
						return (false);
					// here deal with quoted

					++listIt;
					if (listIt->str[0] == '\"' || listIt == tempTokenList.end())
					{
						return (false);
					}
					while (listIt != tempTokenList.end() && listIt->str[0] != '\"')
					{
						tempAttrib.second += listIt->str;
						++listIt;
					}
					if (listIt == tempTokenList.end() || listIt->str.size() >= 2 || tempAttrib.second.empty())
						return (false);
					
					if (httpAllowedQuotedChar().isMatch(tempAttrib.second) == false)
						return (false);
					++listIt;
					continue;	
				}
			}
			else
				return (false);
		}
		else
			return (false);
	}

	if (outPair.first.empty() || (!tempAttrib.first.empty() && tempAttrib.second.empty()))
		return (false);

	if (!tempAttrib.first.empty())
		tempAttribVec.push_back(tempAttrib);

	outPair.second = tempAttribVec;
	return (true);
}


bool extractHttpFieldValueString(const std::string& toExtract, std::deque<s_http_field_value_token>& tokenList, const std::string& delimiterCharSet, const std::string& optionalSpaceCharSet)
{
	tokenList.clear();
	if (toExtract.empty())
		return (true);

	CharTable delimTable(delimiterCharSet, true);
	CharTable optionalTable(optionalSpaceCharSet, true);
	CharTable compTable(delimiterCharSet + optionalSpaceCharSet, true);

	size_t i = 0;
	size_t j = 0;
	size_t pos;
	char c;
	std::string tempStr;
	while (i < toExtract.size())
	{
		if (optionalTable[toExtract[i]] == true)
		{
			j = optionalTable.findFirstNotCharset(toExtract, i);

			if (j == std::string::npos)
				j = toExtract.size();

			tokenList.push_back(s_http_field_value_token());
			s_http_field_value_token& targetToken = tokenList.back();
			
			targetToken.tokenType = OPTIONAL_CHAR;
			targetToken.str = toExtract.substr(i, j - i);

			i = j;
			continue;
		}
		else if (delimTable[toExtract[i]] == true)
		{
			j = toExtract.find_first_not_of(toExtract[i], i);

			if (j == std::string::npos)
				j = toExtract.size();

			tokenList.push_back(s_http_field_value_token());
			s_http_field_value_token& targetToken = tokenList.back();

			targetToken.tokenType = DELIMITER;
			targetToken.str = toExtract.substr(i, j - i);

			i = j;
			continue;
		}
		else
		{
			// general token i think ?

			j = compTable.findFirstCharset(toExtract, i);
			if (j == std::string::npos)
				j = toExtract.size();

			tokenList.push_back(s_http_field_value_token());
			s_http_field_value_token& targetToken = tokenList.back();

			targetToken.tokenType = WORD;
			targetToken.str = toExtract.substr(i, j - i);

			i = j;
			continue;
		}
	}
	return (true);
}
