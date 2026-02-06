#include <string>
#include <cctype>

// convert the given string to 
void	stringToLower(std::string& toConvert)
{
	if (&toConvert == NULL || toConvert.empty())
		return ;
	
	for (size_t i = 0; i < toConvert.size(); i++)
	{
		if (toConvert[i] >= 'A' && toConvert[i] <= 'Z')
			toConvert[i] = static_cast<char>(std::tolower(toConvert[i]));
	}
}