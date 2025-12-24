#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <iostream>
# include <string>
# include <vector>
# include <map>
# include "RouteConfig.hpp"
# include "HttpRequest.hpp"
# include "RequestException.hpp"
# include "HttpResponse.hpp"

# include "Util.hpp"

class HttpRequest; 

class ServerConfig
{
	private:
		int port;
		std::string serverName;
		std::string host;
		std::string root;
		std::string index;
		size_t clientMaxBodySize;
		std::map<int, std::string> errorPages;
		std::map<std::string, RouteConfig> routes;

	public:
		ServerConfig();
		ServerConfig(ServerConfig const &other);
		ServerConfig &operator=(ServerConfig const other);
		~ServerConfig();

		int getPort() const;
		const std::string& getServerName() const;
		const std::string& getHost() const;
		const std::string& getRoot() const;
		const std::string& getIndex() const;
		size_t getClientMaxBodySize() const;
		const std::map<int, std::string>& getErrorPages() const;
		const std::map<std::string, RouteConfig>& getRoutes() const;

		void setPort(int port);
		void setServerName(const std::string& serverName);
		void setHost(const std::string& host);
		void setRoot(const std::string& root);
		void setIndex(const std::string& index);
		void setClientMaxBodySize(size_t clientMaxBodySize);
		void setErrorPages(const std::map<int, std::string>& errorPages);
		void setRoutes(const std::map<std::string, RouteConfig>& routes);
		
		
		void addRoute(const RouteConfig& routeConfig);
		void addErrorPage(int errorCode, const std::string& path);

		std::string getNick();
		RouteConfig 	*findRoute(std::string path);
		bool			resolveRoute(HttpRequest &httpRequest, RouteConfig &route, std::string &localPath , bool &allowDirectoryListing);
		std::map<std::string,std::string>  getAllRouteCGIs(); 

		void debug() const;

};

#endif
