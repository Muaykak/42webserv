# include "../../include/utility_function.hpp"

// how many digit of size_t
static size_t size_t_digits()
{
	size_t	return_digits = 1;
	size_t	cur_div = 1;

	size_t max_size_t = (size_t) - 1;

	while (max_size_t / cur_div >= 10)
	{
		cur_div *= 10;
		return_digits += 1;
	}

	return (return_digits);
}

static std::string max_size_t_string()
{
	size_t	max_size_t = (size_t) - 1;
	size_t	size_t_dig = size_t_digits();

	std::string numstr;
	numstr.resize(size_t_dig);
	size_t i = size_t_dig - 1;
	while (max_size_t > 0)
	{
		numstr[i] = static_cast<unsigned char>((max_size_t % 10) + 48);
		max_size_t /= 10;
		i--;
	}
	return (numstr);
}
// 64 bit size_t
bool string_to_size_t(const std::string& toConvert, size_t& returnValue)
{
	static size_t max_size_t = (size_t) - 1;
	static size_t max_digits = size_t_digits();
	static std::string max_size_str = max_size_t_string();

	if (toConvert.size() < 1 || toConvert.size() > max_digits || digitChar().isMatch(toConvert) == false || (toConvert.size() > 1 && toConvert[0] == '0'))
		return (false);

	// compare before conversion!
	if (toConvert.size() == max_size_str.size() && max_size_str.compare(toConvert) < 0)
	{
		return (false);
	}
	returnValue = 0;
	for (size_t i = 0; i < toConvert.size(); i++)
	{
		if (i != 0)
			returnValue *= 10;
		returnValue += static_cast<unsigned char>(toConvert[i] - 48);
	}


	return (true);
}
