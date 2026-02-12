#ifndef HTTPTHROWSTATUS_HPP
# define HTTPTHROWSTATUS_HPP

# include <string>

class HttpThrowStatus
{
	private:
		std::string _throwMsg;
		int			_statusCode;

	public:
		HttpThrowStatus();
		HttpThrowStatus(const HttpThrowStatus& obj) throw();
		HttpThrowStatus(int statusCode, const std::string& throwMessage) throw();
		HttpThrowStatus::HttpThrowStatus(int statusCode) throw();
		~HttpThrowStatus() throw();
		const std::string& message() const throw();
		int	statusCode() const throw();
};

#endif
