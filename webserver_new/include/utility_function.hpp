#ifndef UTILITY_FUNCTION_HPP
# define UTILITY_FUNCTION_HPP

# include <string>
# include <sstream>
# include <vector>
# include "classes/CharTable.hpp"

template <typename T>
std::string toString(const T &value)
{
	std::stringstream ss;
	ss << value;
	return (ss.str());
}

const CharTable&	whiteSpaceTable();
const CharTable&	linearWhiteSpaceTable();

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

#endif