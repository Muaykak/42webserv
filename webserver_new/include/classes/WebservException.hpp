#ifndef WEBSERVEXCEPTION_HPP
# define WEBSERVEXCEPTION_HPP

# include <exception>
# include <string>

class WebservException : public std::exception
{
protected:
	std::string _message;

public:
	WebservException(const std::string &message) throw();
	virtual ~WebservException() throw();
	virtual const char *what() const throw();
};

#endif