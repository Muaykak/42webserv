/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/13 10:15:43 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/19 16:42:37 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef		LOGGER_HPP
# define	LOGGER_HPP
# include 	<iostream>
# include	"settings.hpp"



class Logger
{
    private:
		Logger();
		Logger(Logger const &other);
		Logger &operator=(Logger const &other);
		~Logger();

	public:
		//void log(int output, std::string color, std::string message);
		static void log(std::string color, std::string  message, ...);
		static std::string getTimestamp();
};

#endif 
