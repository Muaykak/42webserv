#ifndef CONFIGDATA_HPP
# define CONFIGDATA_HPP

# include "ServerConfig.hpp"
# include "WebservException.hpp"
# include "CharTable.hpp"
# include <fstream>
# include <cerrno>
# include <cstring>
# include <cstdlib>
# include <sys/stat.h>
# include <list>

enum e_config_token_type {
	NO_TOKEN_TYPE,
	DATA_TOKEN,
	OP_TOKEN
};

struct s_config_token {
	e_config_token_type	token_type;
	std::string	tokenString;
};

class ConfigData
{
private:
	// search server config by its port
	std::map<int, std::vector<ServerConfig> > _serversConfigs;

	void	splitToken(const std::string &readLine, std::list<s_config_token>& tokenList);

	// remove if found any duplicates
	void checkServersConfigDuplicate();

public:
	ConfigData(const std::string &configPath);
	ConfigData(const ConfigData& obj);
	ConfigData& operator=(const ConfigData& obj);
	~ConfigData();

	const std::map<int, std::vector<ServerConfig> >* getServersConfigMap() const;

	void	printConfigData() const;

};


#endif
