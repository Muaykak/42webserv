#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP


# include <exception>
# include <fstream>
# include <iostream>
# include <map>
# include <set>
# include <sstream>
# include <string>
# include <vector>
# include "RouteConfig.hpp"
# include "ServerConfig.hpp"

class ConfigParser
{
	private:
		static std::string trim(const std::string& str);
		static std::vector<std::string> split(const std::string& str, char delimiter);
		static RouteConfig parseRouteConfig(std::ifstream& file, const std::string& locationPath);
		static bool isValidRouteConfig(const std::map<std::string, RouteConfig>routes);
		static bool isValidServerConfig(const ServerConfig& config);

	public:
		static ServerConfig parseConfig(std::ifstream& file);
    	static std::vector<ServerConfig> parseAllConfigs(const std::string& configFilePath);

};

#endif
