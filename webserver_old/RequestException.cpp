/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestException.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/09 13:14:53 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/09 13:45:13 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestException.hpp"



RequestException::RequestException(RequestException const &other)
{
	statusCode = other.statusCode;
	message = other.message;
}
RequestException &RequestException::operator=(RequestException const &other)
{
	statusCode = other.statusCode;
	message = other.message;
	return *(this);
}



RequestException::RequestException():statusCode(0),message( std::string("Something Wrong"))
{
	
}
RequestException::RequestException(int statusCode, std::string const &message):statusCode(statusCode), message(message)
{

}
RequestException::~RequestException() throw ()
{


}
const char* RequestException::what() const throw()
{
	
	return message.c_str();

}

int			RequestException::getCode() const
{
	return (statusCode);
}

std::string	RequestException::getMessage() const
{
	return (message);
}
