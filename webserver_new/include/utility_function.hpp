#ifndef UTILITY_FUNCTION_HPP
# define UTILITY_FUNCTION_HPP

# include<string>

template <typename T>
std::string toString(const T &value)
{
	std::stringstream ss;
	ss << value;
	return (ss.str());
}

#endif