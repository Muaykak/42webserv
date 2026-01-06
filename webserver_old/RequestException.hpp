/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestException.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/09 13:07:53 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/09 13:46:01 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef		REQUESTEXCEPTION_HPP
# define	REQUESTEXCEPTION_HPP
# include 	<exception>
# include 	<iostream>
# include 	<sstream>

class RequestException: public std::exception
{
	private:
		int			statusCode;
		std::string	message;



	public:
		RequestException();
		RequestException(int statusCode, std::string const &message);
		RequestException(RequestException const &other);
		RequestException &operator=(RequestException const &other);

		~RequestException() throw ();
		virtual const char* what() const throw();
		
		int	getCode() const;
		std::string	getMessage() const;

};

#endif
