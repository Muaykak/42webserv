/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/13 10:32:15 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/10 17:28:58 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"
#include <sstream>
#include <cstdarg>
#include <cstdio>
#include <ctime>

Logger::Logger()
{

}
Logger::Logger(Logger const &other)
{
	if(this != &other)
		*this = other;

}
Logger &Logger::operator=(Logger const &other)
{
	if(this != &other)
		*this = other;
	return (*this);
}

Logger::~Logger()
{

}
void Logger::log(std::string color, std::string message, ...)
{

	if(color == LC_MINOR_NOTE)
		return ; 

	std::stringstream 	ss;
	va_list		args;
	va_start(args, message);
	char	buff[500];
	vsprintf(buff, message.c_str() , args);

	std::cout << color << getTimestamp() << std::string(buff) << LC_RESET << std::endl; 

}
std::string Logger::getTimestamp()
{
	time_t	t; 
	char	buff[24];

	t = time(0);
	strftime(buff, sizeof(buff) , "[%Y-%m-%d %H:%M:%S] " , localtime(&t));
	return std::string(buff);
}
