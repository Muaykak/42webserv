#ifndef UTILITY_FUNCTION_HPP
# define UTILITY_FUNCTION_HPP

# include <string>
# include <sstream>
# include <vector>
# include "classes/CharTable.hpp"
# include <csignal>
# include <netinet/in.h>
# include <list>
# include "classes/ContentTypeTable.hpp"

class TempFileManager;
class EnvpWrapper;

template <typename T>
std::string toString(const T &value)
{
	std::stringstream ss;
	ss << value;
	return (ss.str());
}

const ContentTypeTable& contentTypeTable();
TempFileManager& tempFileManager();
EnvpWrapper& envData();

const CharTable&	whiteSpaceTable();
const CharTable&	hostipChar();
const CharTable&	digitChar();
const CharTable&	hexChar();
const CharTable&	htabSp();
const CharTable&	alphaAtoZ(); /* capital Letters */
const CharTable&	httpFieldNameChar();
const CharTable&	allAlphaChar();
const CharTable&	httpQuotedPairAllowedChar();
const CharTable&	httpTokenChar();
const CharTable& 	httpAllowedQuotedChar();
const CharTable&	httpContentTypeChar();
const CharTable&	forbiddenFieldValueChar();
const CharTable&	allowRequestTargetChar();

std::string in_addr_t_to_string(in_addr_t toConvert);
bool	string_to_in_addr_t(const std::string& toConvert, in_addr_t& returnValue);

std::string	my_ft_trim(const std::string& toTrim, const std::string& trimCharSet);

void	splitString(const std::string& toSplit,
		const std::string& delimitter,
		std::list<std::string>& returnList);
void	splitString(const std::string& toSplit,
		const std::string& delimitter,
		std::vector<std::string>& returnVec);
void	splitString(const std::string& toSplit,
		const std::string& delimitter,
		std::vector<std::string>& returnVec, size_t startPos);
void	splitString(const std::string& toSplit,
		const std::string& delimitter,
		std::vector<std::string>& returnVec, size_t startPos, size_t size);

void	splitString(const std::string& toSplit,
		const CharTable& delimitter,
		std::vector<std::string>& returnVec);
void	splitString(const std::string& toSplit,
		const CharTable& delimitter,
		std::vector<std::string>& returnVec, size_t startPos);
void	splitString(const std::string& toSplit,
		const CharTable& delimitter,
		std::vector<std::string>& returnVec, size_t startPos, size_t size);

std::string	stringToLower(const std::string& toConvert);
std::string	stringToUpper(const std::string& toConvert);

bool string_to_size_t(const std::string& toConvert, size_t& returnValue);
std::string size_t_to_hex(size_t toConvert);
bool hex_to_size_t(const std::string& hexStr, size_t& returnValue);

void 	serverStopHandler(int signum);

#endif
