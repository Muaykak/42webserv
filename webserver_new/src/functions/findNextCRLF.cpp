#include "../../include/utility_function.hpp"

bool findNextCRLF(const std::string& str, size_t& outPos)
{
	return findNextCRLF(str, outPos, 0);
}

bool findNextCRLF(const std::string& str, size_t& outPos, size_t startIndex)
{
	size_t lfpos;
	size_t crpos;

	if (startIndex>= str.size())
	{
		outPos = std::string::npos;
		return true;
	}

	crpos = str.find_first_of('\r', startIndex);
	lfpos = str.find_first_of('\n', startIndex);

	if (crpos == std::string::npos && lfpos == std::string::npos)
	{
		outPos = std::string::npos;
		return (true);
	}
	
	if (crpos != std::string::npos && lfpos != std::string::npos)
	{
		if (crpos + 1 == lfpos)
		{
			outPos = crpos;
			return true;
		}
		else
			return false;
	}

	if (crpos == std::string::npos)
		return (false);
	else
	{
		if (crpos == str.size() - 1)
		{
			outPos = std::string::npos;
			return true;
		}
		else
			return (false);
	}
}
