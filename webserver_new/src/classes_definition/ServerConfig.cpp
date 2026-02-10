#include "../../include/classes/ServerConfig.hpp"


ServerConfig::ServerConfig() : _listenPort(-1){}
ServerConfig::ServerConfig(const ServerConfig &obj)
: _serverConfig(obj._serverConfig),
_locationsConfig(obj._locationsConfig),
_listenPort(obj._listenPort),
_serverNameVec(obj._serverNameVec),
_host_ip_set(obj._host_ip_set)
{
	const std::vector<std::string>* vecPtr = getServerData("server_name");
	if (vecPtr)
		_serverNameVec = *vecPtr;
	for (size_t i = 0; i < _serverNameVec.size(); i++)
		_serverNameVec[i] = stringToLower(_serverNameVec[i]);
		

}
ServerConfig& ServerConfig::operator=(const ServerConfig &obj)
{
	if (this != &obj)
	{
		_serverConfig = obj._serverConfig;
		_locationsConfig = obj._locationsConfig;
		_serverNameVec = obj._serverNameVec;
		_listenPort = obj._listenPort;
	}
	return (*this);
}
ServerConfig::~ServerConfig() {}
ServerConfig::ServerConfig(const t_config_map& serverConfig, const t_location_map& locationsConfig, int listenPort)
: _serverConfig(serverConfig), _locationsConfig(locationsConfig)
{
	if (listenPort < 0 || listenPort > 65535) {
		throw WebservException("ServerConfig::InvalidListenPort");
	}
	_listenPort = listenPort;
	const std::vector<std::string>* vecPtr = getServerData("server_name");
	if (vecPtr)
		_serverNameVec = *vecPtr;
	for (size_t i = 0; i < _serverNameVec.size(); i++)
		_serverNameVec[i] = stringToLower(_serverNameVec[i]);
	
	// check if has 'host'in _serverConfig
	t_config_map::const_iterator it = _serverConfig.find("host");
	if (it != _serverConfig.end())
	{
		std::vector<std::string>::const_iterator vecIt = it->second.begin();

		in_addr_t	temp_in_addr_t;
		while (vecIt != it->second.end())
		{
			// convert the ip strings into in_addr_t)
			// if converrsion failed should throw back errors
			if (string_to_in_addr_t(*vecIt, temp_in_addr_t) == false)
				throw WebservException("ServerConfig::string_to_in_addr_t::error");
			if (temp_in_addr_t == 0xFFFFFFFF)
				throw WebservException("ServerConfig::string_to_in_addr_t::cannot assign server to broadcast address");
			if (temp_in_addr_t == 0x00000000)
			{
				// mean that accept all connection
				_host_ip_set.clear();
				break ;
			}
			_host_ip_set.insert(temp_in_addr_t);
			++vecIt;
		}
	}
}

bool ServerConfig::operator==(const ServerConfig& obj) const {
	return (_serverConfig == obj._serverConfig && _locationsConfig == obj._locationsConfig);
}

const std::vector<std::string>* ServerConfig::getServerData(const std::string &keyToFind) const
{
	t_config_map::const_iterator FoundKey = _serverConfig.find(keyToFind);
	if (FoundKey == _serverConfig.end())
		return (NULL);
	return (&FoundKey->second);
}
const std::vector<std::string>* ServerConfig::getLocationData(const std::string &locationPath, const std::string &keytoFind) const
{
	const t_config_map *locationConfig = longestPrefixMatch(locationPath);
	t_config_map::const_iterator foundKey;
	if (locationConfig)
	{
		foundKey = locationConfig->find(keytoFind);
		if (foundKey != locationConfig->end())
		{
			return (&foundKey->second);
		}
	}
	foundKey = _serverConfig.find(keytoFind);
	if (foundKey != _serverConfig.end())
	{
		return (&foundKey->second);
	}
	return (NULL);
}

int	ServerConfig::getPort() const {
	return (_listenPort);
}
const std::vector<std::string> &ServerConfig::getServerNameVec() const {
	return (_serverNameVec);
}

void	ServerConfig::printServerConfig() const {
	t_config_map::const_iterator serverIt = _serverConfig.begin();
	std::cout << "-- Server Config --" << std::endl;
	size_t	i = 0;
	while (serverIt != _serverConfig.end()){	
		std::cout << serverIt->first << ": ";
		i = 0;
		while (i < serverIt->second.size()){
			if (i != 0)
				std::cout << ", ";
			std::cout << (serverIt->second)[i];
			++i;
		}
		std::cout << std::endl;
		++serverIt;
	}
	t_location_map::const_iterator locationIt = _locationsConfig.begin();
	while (locationIt != _locationsConfig.end()){
		std::cout << "\tLocation " << locationIt->first << std::endl;
		t_config_map::const_iterator locationValueIt = locationIt->second.begin();
		while (locationValueIt != locationIt->second.end()){
			std::cout << "\t\t" << locationValueIt->first << ": ";
			i = 0;
			while (i < locationValueIt->second.size()){
				if (i != 0)
					std::cout << ", ";
				std::cout << (locationValueIt->second)[i];
				++i;
			}
			std::cout << std::endl;
			++locationValueIt;
		}
		++locationIt;
	}
	std::cout << "--------------" << std::endl;
}

void ServerConfig::resolveLocationPath(std::string locationPath)
{
}

// ####################  AI GEN -> Make sure to understand logic ########
const t_config_map *ServerConfig::longestPrefixMatch(std::string locationPath) const
   {
       std::string tempPath = locationPath;

       while (true)
       {
           t_location_map::const_iterator it = _locationsConfig.find(tempPath);
           if (it != _locationsConfig.end())
               return (&it->second);
           
           // Optional: Handle cases where config has "/path/" but request is "/path"
           /*
           if (tempPath.length() > 0 && tempPath[tempPath.length() - 1] != '/') {
               it = _locationsConfig.find(tempPath + "/");
               if (it != _locationsConfig.end()) return (&it->second);
           }
           */

           if (tempPath.empty() || tempPath == "/")
               return (NULL);

           size_t lastSlash = tempPath.find_last_of('/');
           if (lastSlash == std::string::npos)
               tempPath = "";
           else if (lastSlash == 0)
               tempPath = "/";
           else
               tempPath = tempPath.substr(0, lastSlash);
       }
   }
// ##############################################################

const std::set<in_addr_t>&	ServerConfig::getHostIp() const
{
	return (_host_ip_set);
}
