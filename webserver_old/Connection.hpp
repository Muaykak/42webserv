/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/04 17:17:25 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/20 19:42:37 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef 	CONNECTION_HPP
# define	CONNECTION_HPP

# include	<ctime>
# include 	<cstdio>
# include	<fcntl.h>
# include	<stdexcept>
# include	<string>
# include 	<errno.h>
# include 	<sys/epoll.h>
# include 	<sstream>
# include 	<unistd.h>

# include 	"settings.hpp"
# include	"ServerConfig.hpp"
# include	"HttpRequest.hpp"
# include	"HttpResponse.hpp"
# include	"ConnectionController.hpp"
# include	"Logger.hpp"

class Connection 
{
	private:

		int					fd;	
		time_t				expiresOn; 
		ServerConfig		serverConfig;
		bool				isReady;
		int					bodyLength;
		std::string			boundary;
		std::string			requestBuffer;
		std::vector<char>	rawPostBody;
		std::string 		responseBuffer;		
		bool				headerIsCompleted;
		bool				requestIsCompleted;
		int					epollSocket;
		size_t 				contentLength;
		void 				setNonBlock();

	public:
		Connection();
		Connection(int fd, ServerConfig config);
		Connection(Connection const &other);
		Connection &operator=(Connection const other);
		~Connection();

		time_t				getExpiresOn() const ;
		int 				getSocket() const;
		std::string			getRequestBuffer() const;
		std::string 		getResponseBuffer() const;
		bool 				getHeaderIsComplete() const;
		bool 				getRequestIsComplete() const;
		ServerConfig		&getServerConfig();
		std::vector<char>	&getRawPostBody();
		bool				getIsReady() const;
		size_t				getContentLength();
		std::string			getBoundary();

		void 				setRequestIsComplete(bool newValue);
		bool 				setExpiresOn(time_t);
		bool 				setSocket(int fd);
		void				setHeaderIsComplete(bool newValue);
		void				setIsReady(bool newValue) ;
		void 				setContentLength(int i);

		void				punchIn(void);		
		bool				isHeaderComplete(); 

		bool				appendRawPostBody(char *, size_t bytesRead);
		bool				appendRequestBuffer(char *buffer, size_t length, std::vector<ServerConfig> servers);
		bool				processRequest(HttpRequest &httpRequest);
		bool				ready(HttpResponse &httpResponse, bool sendAsWell=false);
		bool				needsToWrite();
		size_t				truncateResponseBuffer(size_t bytes);
		bool				isExpired(time_t comp) const;

		void				debugPostBody();
		void 				debug();
		void 				clear();
		bool				shouldRetry();
		std::string			debugText;

		class ParseRequestException: public std::exception
		{
			public:
				virtual const char *what() const throw();
		};
}; 

#endif
