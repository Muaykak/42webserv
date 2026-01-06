/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/13 10:10:12 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/20 20:04:36 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef 	HTTP_REQUEST_HPP
# define	HTTP_REQUEST_HPP
# include	<map>
# include	<string>
# include	<cstring>
# include   "ServerConfig.hpp"
# include   "HttpResponse.hpp"
# include   "RequestException.hpp"
# include   "Logger.hpp"



class ServerConfig;
class HttpResponse;

class HttpRequest
{
	private:

		std::string								method; 
		std::string 							path; 
		std::string 							rawPath; 

		std::map<std::string, std::string> 		headers;

		size_t									contentLength;
		std::map<std::string, std::string> 		queryStrings;
		std::string								rawQueryString;

		std::string								body;


	public:
		HttpRequest();
		HttpRequest(HttpRequest const &other);
		HttpRequest &operator=(HttpRequest const &other);
		~HttpRequest();

		const std::string 						&getMethod() const;
		const std::string 						&getPath() const;
		const std::string 						&getRawPath() const;
		const std::map<std::string, std::string> &getHeader() const;
		size_t 									getContentLength() const;
		const std::map<std::string, std::string> &getqueryStrings() const;
		const std::string 						&getBody() const;
		std::string 							getRawQueryString() const; 

		bool 									setMethod(std::string methodStr);
		bool 									setPath(std::string pathStr);
		bool 									setRawPath(std::string rawPathStr);
		bool 									setHeader(std::string name, std::string value, bool overwriteExisting=false);
		bool 									setContentLength(size_t contentLengthVal);
		bool 									setQueryString(std::string key, std::string value);
		bool 									setBody(std::string bodyStr);


		bool 									parseRequestHeaders(ServerConfig server, std::string requestString);
		static size_t preprocessContentLength(std::string requestString, std::string &method);

		std::string getHeader(std::string const str) const;



		void  debug();
		

		
};

#endif

