
#include "../../include/utility_function.hpp"
#include <limits>

static size_t max_hex_digit()
{
	size_t	return_digits = 1;
	size_t	cur_div = 1;

	size_t max_size_t = std::numeric_limits<size_t>::max();

	while (max_size_t / cur_div >= 16)
	{
		cur_div *= 16;
		return_digits += 1;
	}

	return (return_digits);
}

bool hex_to_size_t(const std::string& hexStr, size_t& returnValue)
{
	static size_t max_hex_digits = max_hex_digit();
	static size_t max_size_t = std::numeric_limits<size_t>::max();

	// won't torelate if '0' is in front
	if (hexStr.empty() || hexChar().isMatch(hexStr) == false || (hexStr.size() > 1 && hexStr[0] == '0') || hexStr.size() > max_hex_digits)
		return (false);
		
	// would be FFF... anyway so i assume we don't need to change if size_t still behave in 2 bits
	returnValue = 0;
	char c;
	for (size_t i = 0; i < hexStr.size(); i++)
	{
		if (i != 0)
			returnValue *= 16;

		c = static_cast<unsigned char>(std::tolower(hexStr[i]));

		if (c >= '0' && c <= '9')
			returnValue += c - '0';
		else
			returnValue += c - 87;
	}
	
	return (true);
}
