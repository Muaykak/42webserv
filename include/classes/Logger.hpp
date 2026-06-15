#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <string>
# include "../defined_value.hpp"
# include <sstream>
# include <cstdarg>
# include <iostream>
# include <cstdio>

class Logger
{
	private:
		Logger();
		Logger(Logger const &other);
		Logger &operator=(Logger const &other);
		~Logger();
	
	public:
		// void log(int output, std::string color, std::string message);
		static void log(std::string color, std::string message, ...);
		static std::string getTimestamp();
};

#endif
