#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <map>
# include <string>
# include <vector>
# include <set>
# include <iostream>

// for uint32_t
# include <netinet/in.h>
# include "WebservException.hpp"
# include "../defined_value.hpp"
# include "../utility_function.hpp"


class ServerConfig
{
private:
	t_config_map _serverConfig;
	t_location_map _locationsConfig;
	int				_listenPort;
	std::vector<std::string> _serverNameVec;

	// set of ipv4 address host server
	std::set<in_addr_t>	_host_ip_set;
	// there are helper functions in utility_function.hpp

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
	const std::vector<std::string> &getServerNameVec() const;
	const std::set<in_addr_t>&	getHostIp() const;

	void	printServerConfig() const;
};

#endif
