#include "../../include/utility_function.hpp"

std::string	my_ft_trim(const std::string& toTrim, const std::string& trimCharSet)
{
	if (toTrim.empty() || trimCharSet.empty())
		return (toTrim);
	
	CharTable table(trimCharSet, true);

	size_t pos = table.findFirstNotCharset(toTrim);

	if (pos == std::string::npos)
		return (std::string());
	
	size_t backpos = table.findLastNotCharset(toTrim);
	/* not suppose to have npos here, the lowest value
	be the pos var*/

	return (toTrim.substr(pos, backpos - pos + 1));
}
