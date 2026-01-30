#ifndef UTILITY_FUNCTION_HPP
# define UTILITY_FUNCTION_HPP

# include <string>
# include <sstream>
# include <vector>
# include "classes/CharTable.hpp"
# include <csignal>

template <typename T>
std::string toString(const T &value)
{
	std::stringstream ss;
	ss << value;
	return (ss.str());
}

const CharTable&	whiteSpaceTable();
const CharTable&	htabSp();
const CharTable&	allowedFieldNameChar();
const CharTable&	forbiddenFieldValueChar();

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

void 	serverStopHandler(int signum);
volatile sig_atomic_t& signal_status();

#endif
