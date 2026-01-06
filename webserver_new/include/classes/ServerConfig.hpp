#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <map>
# include <string>
# include <vector>
# include <iostream>
# include "WebservException.hpp"

typedef std::map<std::string, std::vector<std::string> > t_config_map;
typedef std::map<std::string, const t_config_map> t_location_map;

class ServerConfig
{
private:
	t_config_map _serverConfig;
	t_location_map _locationsConfig;
	int				_listenPort;
	const std::vector<std::string> *_serverNameVec;

	static void resolveLocationPath(std::string locationPath);
	// ####################  AI GEN -> Make sure to understand logic ########
	const t_config_map *longestPrefixMatch(std::string locationPath) const;
	// ##############################################################
public:
	ServerConfig();
	ServerConfig(const ServerConfig &obj);
	ServerConfig &operator=(const ServerConfig &obj);
	~ServerConfig();
	ServerConfig(const t_config_map& serverConfig, const t_location_map& locationsConfig, int listenPort);
	bool operator==(const ServerConfig& obj) const;
	const std::vector<std::string>* getServerData(const std::string &keyToFind) const;
	const std::vector<std::string>* getLocationData(const std::string &locationPath, const std::string &keytoFind) const;
	int	getPort() const;
	const std::vector<std::string> * getServerNameVec() const;

	void	printServerConfig() const;
};

#endif