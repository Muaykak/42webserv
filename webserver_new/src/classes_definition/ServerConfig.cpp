#include "../../include/classes/ServerConfig.hpp"
#include "../../include/classes/EnvpWrapper.hpp"
#include <limits>

#include <sys/stat.h>

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

	/* check the t_config_map of serverConfig first */
	try
	{
		std::vector<std::string> tempVec;
		tempVec.push_back("root");
		//tempVec.push_back("host");

		checkMustExistDirective(tempVec, _serverConfig);
	}
	catch (const std::vector<std::string>& e)
	{
		std::string errmsg = "ServerConfig::Directives Missing From ServerBlock:: ";
		for (size_t i = 0; i < e.size(); i++)
		{
			if (i != 0)
				errmsg += ", ";
			errmsg += "\"" + e[i] + "\"";
		}

		throw WebservException(errmsg);
	}

	checkConfigBlock(_serverConfig);

	/* check all location map*/
	t_location_map::iterator it = _locationsConfig.begin();
	bool haveBaseLocation = false;
	while (it != _locationsConfig.end())
	{
		std::cout << "curr check block: \"" << it->first << "\"\n";
		if (it->first == "/")
			haveBaseLocation = true;

		checkConfigBlock(it->second);
		++it;
	}

	if (haveBaseLocation == false)
		throw WebservException("ServerConfig::MustHave Location \"/\"");
}

void	ServerConfig::checkConfigBlock(t_config_map& configBlock)
{
	t_config_map::iterator it = configBlock.begin();

	void (ServerConfig::*funcPtr)(std::vector<std::string>&, const t_config_map*) = NULL;

	while (it != configBlock.end())
	{
		funcPtr = getDirectiveCheckFunction(it->first);
		if (funcPtr)
			(this->*funcPtr)(it->second, &configBlock);
		++it;
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

const std::vector<std::string>* ServerConfig::getLocationDataNoRollBack(const t_config_map* targetLocationBlock, const std::string &keytoFind) const
{
	t_config_map::const_iterator foundKey;
	if (targetLocationBlock)
	{
		foundKey = targetLocationBlock->find(keytoFind);
		if (foundKey != targetLocationBlock->end())
		{
			return (&foundKey->second);
		}
	}
	return (NULL);
}

const std::vector<std::string>* ServerConfig::getLocationData(const t_config_map* targetLocationBlock, const std::string &keytoFind) const
{
	t_config_map::const_iterator foundKey;
	if (targetLocationBlock)
	{
		foundKey = targetLocationBlock->find(keytoFind);
		if (foundKey != targetLocationBlock->end())
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


const t_config_map *ServerConfig::findLocationBlock(std::string locationPath) const
{
	std::string tempPath = locationPath;
	t_location_map::const_iterator it;
	while (true)
	{
		it = _locationsConfig.find(tempPath);
		if (it != _locationsConfig.end())
		{
			 std::cout << "find location block: " << tempPath << std::endl;
			return (&it->second);
		}
		if (tempPath.size() > 0) {
			if (tempPath[tempPath.size() - 1] == '/')
			{
				tempPath.erase(tempPath.size() - 1);

				it = _locationsConfig.find(tempPath);
				if (it != _locationsConfig.end())
				{
					std::cout << "find location block: " << tempPath << std::endl;
					return (&it->second);
				}
			}
			else
			{
				it = _locationsConfig.find(tempPath + "/");
				if (it != _locationsConfig.end())
				{
					std::cout << "find location block: " << tempPath + "/" << std::endl;
					return (&it->second);
				}
			}
		}

		if (tempPath.empty() || tempPath == "/")
			return (NULL);

		size_t lastSlash = tempPath.find_last_of('/');
		if (lastSlash == std::string::npos)
			tempPath = "";
		else if (lastSlash == 0)
			tempPath = "/";
		else
			tempPath = tempPath.substr(0, lastSlash + 1);
	}
}

// will throw std::vector<std::sting> out for the missing directive in particular t_config_map;
void	ServerConfig::checkMustExistDirective(const std::vector<std::string>& directiveKeyVec, const t_config_map& mapToFind)
{
	std::vector<std::string> notfoundDirectiveVec;

	for (size_t i = 0; i < directiveKeyVec.size(); i++)
	{
		if (mapToFind.find(directiveKeyVec[i]) == mapToFind.end())
			notfoundDirectiveVec.push_back(directiveKeyVec[i]);
	}

	if (notfoundDirectiveVec.size() != 0)
		throw std::vector<std::string>(notfoundDirectiveVec);
}

void ServerConfig::checkDirectiveHost(std::vector<std::string>& valueVec, const t_config_map* targetLocationBlock)
{
		(void)targetLocationBlock;
	in_addr_t	temp_in_addr_t;
	for (size_t i = 0; i < valueVec.size(); i++)
	{
		if (valueVec[i] == "localhost")
			valueVec[i] = "127.0.0.1";
		// convert the ip strings into in_addr_t)
		// if converrsion failed should throw back errors
		if (string_to_in_addr_t(valueVec[i], temp_in_addr_t) == false)
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
	}
}

void ServerConfig::checkDirectiveServerName(std::vector<std::string>& valueVec, const t_config_map* targetLocationBlock)
{
	if (targetLocationBlock)
		(void)targetLocationBlock;
	for (size_t i = 0; i < valueVec.size(); i++)
	{
		if (valueVec[i].empty() || valueVec[i].size() > 255)
			throw WebservException("ServerConfig::ServerName::error");
		bool lastCharWasDot = true;
		size_t labelLength = 0;

		for (size_t j = 0; j < valueVec[i].length(); ++j)
		{
			char c = valueVec[i][j];

			if (c == ';')
				throw WebservException("ServerConfig::ServerName::error");

			if (c == '.')
			{
				if (lastCharWasDot || (j > 0 && valueVec[i][j - 1] == '-'))
					throw WebservException("ServerConfig::ServerName::error");
				lastCharWasDot = true;
				labelLength = 0;
			}
			else if (std::isalnum(c) || c == '-')
			{
				if (lastCharWasDot && c == '-')
					throw WebservException("ServerConfig::ServerName::error");
				if (++labelLength > 63)
					throw WebservException("ServerConfig::ServerName::error");
				lastCharWasDot = false;
			}
			else
				throw WebservException("ServerConfig::ServerName::error");
		}

		if (valueVec[i][valueVec[i].length() - 1] == '-')
			throw WebservException("ServerConfig::ServerName::error");

		valueVec[i] = stringToLower(valueVec[i]);
	}
}

void ServerConfig::checkDirectiveClientMaxBodySize(std::vector<std::string>& valueVec, const t_config_map* targetLocationBlock)
{
	(void)targetLocationBlock;
	// check the size of vector must be 1
	if (valueVec.size() != 1)
		throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value");

	static const size_t maxByte = std::numeric_limits<size_t>::max();
	static const size_t maxKB = maxByte / 1024;
	static const size_t maxMB = maxKB / 1024;
	static const size_t maxGB = maxMB / 1024;
	static const size_t maxTB = maxGB / 1024;

	std::string target = stringToLower(valueVec[0]);

	if (digitChar().isMatch(target) == true)
	{
		size_t	convertSize;
		if (string_to_size_t(target, convertSize) == false)
			throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value");
	}
	else
	{
		/* check the validity
		
		allowed : 10m 10mb 10b
		not allowed "10 mb", "10 m b" 
		*/

		size_t pos = digitChar().findLastCharset(target);
		if (pos == std::string::npos || pos == target.size() - 1)
			throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value");

		std::string frontStr = target.substr(0, pos + 1);
		std::string backStr = target.substr(pos + 1);

		size_t convertSize;
		if (string_to_size_t(frontStr, convertSize) == false)
			throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value");

		else if (backStr == "k" || backStr == "kb")
		{
			if (convertSize > maxKB)
				throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value::maximum::" + toString(maxKB) + "kb");
			convertSize *= static_cast<size_t>(1 << 10);
		}
		else if (backStr == "m" || backStr == "mb")
		{
			if (convertSize > maxMB)
				throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value::maximum::" + toString(maxKB) + "kb");
			convertSize *= static_cast<size_t>(1 << 20);

		}
		else if (backStr == "g" || backStr == "gb")
		{
			if (convertSize > maxGB)
				throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value::maximum::" + toString(maxKB) + "kb");
			convertSize *= static_cast<size_t>(1 << 30);

		}
		else if (backStr == "t" || backStr == "t")
		{
			if (convertSize > maxTB)
				throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value::maximum::" + toString(maxKB) + "kb");
			convertSize *= static_cast<size_t>(1) << 40;
		}
		else if (backStr != "b")
			throw WebservException("ServerConfig::server block::client_max_body_size::Invalid Value");
		valueVec[0]	= toString(convertSize);

		std::cout << "byteSIze: " << valueVec[0] << "\n";
	}
}

void ServerConfig::checkIfPathIsDirectory(const std::string& path, const t_config_map* targetLocationBlock)
{
	if (path.empty())
		throw WebservException("ServerConfig::checkIfPathIsDirectory::path empty");

	std::string systemPath;
	std::string rootPath;
	std::string combinedPath;
	if (path[0] != '/' && targetLocationBlock)
	{
		const std::vector<std::string> *tempFound = getLocationData(targetLocationBlock, "root");
		if (tempFound && tempFound->size() == 1)
			rootPath = (*tempFound)[0];
		if (rootPath.empty() || rootPath[0] != '/')
		{
			systemPath = envData().findValue("PWD");
			systemPath += "/" + rootPath;
		}
		else
		{
			systemPath = rootPath;
		}

		combinedPath = systemPath + "/" + path;
	}
	else
	{
		combinedPath = path;
	}


	struct stat fileStat;
	std::memset(&fileStat, 0, sizeof(fileStat));
	if (stat(combinedPath.c_str(), &fileStat) != 0)
	{
		std::string errMsg = "ServerConfig::CheckIfPathIsDirectory::stat()::target_path " + combinedPath + "::";
		errMsg += strerror(errno);
		throw WebservException(errMsg);
	}
		
	/* check the */
	if (!S_ISDIR(fileStat.st_mode))
		throw WebservException("ServerConfig::CheckIfPathIsDirectory::target must be a regular readable file::" + combinedPath);
}

void ServerConfig::checkIfPathIsRegularReadable(const std::string& path, const t_config_map* targetLocationBlock)
{
	if (path.empty())
		throw WebservException("ServerConfig::checkIfPathIsRegularReadable::path empty");

	std::string systemPath;
	std::string rootPath;
	std::string combinedPath;
	if (path[0] != '/' && targetLocationBlock)
	{
		const std::vector<std::string> *tempFound = getLocationData(targetLocationBlock, "root");
		if (tempFound && tempFound->size() == 1)
			rootPath = (*tempFound)[0];
		
		if (rootPath.empty() || rootPath[0] != '/')
		{
			systemPath = envData().findValue("PWD");
			systemPath += "/" + rootPath;
		}
		else
		{
			systemPath = rootPath;
		}

		combinedPath = systemPath + "/" + path;
	}
	else
	{
		combinedPath = path;
	}


	struct stat fileStat;
	std::memset(&fileStat, 0, sizeof(fileStat));
	if (stat(combinedPath.c_str(), &fileStat) != 0)
	{
		std::string errMsg = "ServerConfig::CheckIfPathIsRegularReadable::stat()::target_path " + combinedPath + "::";
		errMsg += strerror(errno);
		throw WebservException(errMsg);
	}
		
	/* check the */
	if (!S_ISREG(fileStat.st_mode))
		throw WebservException("ServerConfig::CheckIfPathRegularReadable::target must be a regular readable file::" + combinedPath);

	if (access(combinedPath.c_str(), F_OK | R_OK) != 0)
		throw WebservException("ServerConfig::CheckIfPathRegularReadable::target must be a regular readable file::" + combinedPath);

}

void ServerConfig::checkIfPathIsRegularExecutable(const std::string& path, const t_config_map* targetLocationBlock)
{
	if (path.empty())
		throw WebservException("ServerConfig::checkIfPathIsRegularExecutable::path empty");

	std::string systemPath;
	std::string rootPath;
	std::string combinedPath;
	if (path[0] != '/' && targetLocationBlock)
	{
		const std::vector<std::string> *tempFound = getLocationData(targetLocationBlock, "root");
		if (tempFound && tempFound->size() == 1)
			rootPath = (*tempFound)[0];
		if (rootPath.empty() || rootPath[0] != '/')
		{
			systemPath = envData().findValue("PWD");
			systemPath += "/" + rootPath;
		}
		else
		{
			systemPath = rootPath;
		}

		combinedPath = systemPath + "/" + path;
	}
	else
	{
		combinedPath = path;
	}


	struct stat fileStat;
	std::memset(&fileStat, 0, sizeof(fileStat));
	if (stat(combinedPath.c_str(), &fileStat) != 0)
	{
		std::string errMsg = "ServerConfig::CheckIfPathIsRegularExecutable::stat()::target_path " + combinedPath + "::";
		errMsg += strerror(errno);
		throw WebservException(errMsg);
	}
		
	/* check the */
	if (!S_ISREG(fileStat.st_mode))
		throw WebservException("ServerConfig::CheckIfPathRegularExecutable::target must be a regular readable file::" + combinedPath);

	if (access(combinedPath.c_str(), F_OK | X_OK) != 0)
		throw WebservException("ServerConfig::CheckIfPathRegularExucutable::target must be a regular readable file::" + combinedPath);
}

void	ServerConfig::checkDirectiveRoot(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() != 1)
		throw WebservException("ServerConfig::\"root\" directive invalid value");

	checkIfPathIsDirectory(valueVec[0], NULL);
	(void)targetLocationBlock;
}

void	ServerConfig::checkDirectiveErrorPage(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() < 2 || valueVec.size() % 2 != 0)
		throw WebservException("ServerConfig::\"error_page\"::invalid value");

	std::map<size_t, std::string> tempMap;

	size_t i = 0;
	size_t convertNum;
	while (i < valueVec.size() - 1)
	{
		if (string_to_size_t(valueVec[i], convertNum) == false || (convertNum < 100 || convertNum >= 600))
			throw WebservException("ServerConfig::\"error_page\"::invalid value");

		checkIfPathIsRegularReadable(valueVec[i + 1], NULL);

		if (tempMap.find(convertNum) != tempMap.end())
			throw WebservException("ServerConfig::\"error_page\"::invalid value::duplicate value");
		else
			tempMap[convertNum] = valueVec[i + 1];

		i += 2;
	}
	(void)targetLocationBlock;
}

void	ServerConfig::checkDirectiveIndex(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() != 1)
		throw WebservException("ServerConfig::\"index\"::invalid value");

	checkIfPathIsRegularReadable(valueVec[0], targetLocationBlock);
}

void	ServerConfig::checkDirectiveAllowedMethods(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() < 1 || valueVec.size() > 3)
		throw WebservException("ServerConfig::\"allowed_methods\"::invalid value");

	/* remove duplicate also */

	std::set<std::string> methodSet;

	for (size_t i = 0; i < valueVec.size(); i++)
	{
		if (alphaAtoZ().isMatch(valueVec[i]) == false)
			throw WebservException("ServerConfig::\"allowed_methods\"::invalid value");
		
		if (methodSet.find(valueVec[i]) != methodSet.end())
			throw WebservException("ServerConfig::\"allowed_methods\"::invalid value::duplicate value");
		methodSet.insert(valueVec[i]);
	}

	for (size_t i = 0; i < valueVec.size(); i++)
	{
		const std::string target = valueVec[i];

		if (target != "GET" && target != "POST" && target != "DELETE")
			throw WebservException("ServerConfig::\"allowed_methods\"::invalid value::accept only GET POST DELETE");
	}

	(void)targetLocationBlock;
}

void ServerConfig::checkDirectiveUploadStore(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() != 1)
		throw WebservException("ServerConfig::\"upload_store\"::invalid value");

	checkIfPathIsDirectory(valueVec[0], NULL);
	(void)targetLocationBlock;
}

void ServerConfig::checkDirectiveAutoIndex(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() != 1)
		throw WebservException("ServerConfig::\"autoindex\"::invalid value");

	if (stringToLower(valueVec[0]) != "on")
		throw WebservException("ServerConfig::\"autoindex\"::invalid value::accept only \"on\"");
	
	(void)targetLocationBlock;
}

void ServerConfig::checkDirectiveReturn(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() != 2)
		throw WebservException("ServerConfig::\"return\"::invalid value");

	size_t convertNum;
	if (string_to_size_t(valueVec[0], convertNum) == false || (convertNum >= 400 || convertNum < 300))
		throw WebservException("ServerConfig::\"return\"::invalid value");

	(void)targetLocationBlock;
	// checkIfPathIsDirectory(valueVec[1], targetLocationBlock);
}

void ServerConfig::checkDirectiveCGIpass(std::vector<std::string>& valueVec, const t_config_map *targetLocationBlock)
{
	if (valueVec.size() < 2)
		throw WebservException("ServerConfig::\"cgi_pass\"::invalid value");

	std::map<std::string, std::vector<std::string> > tempMap;
	std::vector<std::string> tempVec;
	std::vector<std::string>::iterator it = valueVec.begin();
	std::vector<std::string>::iterator next = valueVec.end();
	std::set<std::string> tempSet;

	while (it != valueVec.end())
	{
		if ((*it)[0] != '.')
			throw WebservException("ServerConfig::\"cgi_pass\"::invalid value::must start with \'.\'");

		if (tempMap.find(*it) != tempMap.end())
			throw WebservException("ServerConfig::\"cgi_pass\"::invalid value::duplicate value");

		next = it + 1;
		while (next != valueVec.end())
		{
			if ((*next)[0] == '.')
				break;
			++next;
		}

		tempVec.insert(tempVec.end(), it + 1, next);
		/* check duplication */
		{
			if (tempVec.size() < 1)
				throw WebservException("ServerConfig::\"cgi_pass\"::invalid value::extension no details");
			if (tempVec.size() > 4)
				throw WebservException("ServerConfig::\"cgi_pass\"::invalid value");

			checkIfPathIsRegularExecutable(tempVec[0], NULL);

			for (size_t i = 1; i < tempVec.size(); i++)
			{
				const std::string& target = tempVec[i];

				if (alphaAtoZ().isMatch(target) == false)
					throw WebservException("ServerConfig::\"cgi_pass\"::invalid value::invalid");

				if (target != "GET" && target != "POST" && target != "DELETE")
					throw WebservException("ServerConfig::\"cgi_pass\"::invalid value::invalid");

				if (tempSet.find(target) != tempSet.end())
					throw WebservException("ServerConfig::\"cgi_pass\"::invalid value::Duplicate method");

				tempSet.insert(target);
			}
			tempSet.clear();
		}

		tempMap.insert(std::make_pair(*it, tempVec));
		tempVec.clear();
		it = next;
	}

	(void)targetLocationBlock;
}

std::map<std::string, void(ServerConfig::*)(std::vector<std::string>&, const t_config_map*)> ServerConfig::buildConfigCheckMap()
{
	std::map<std::string, void(ServerConfig::*)(std::vector<std::string>&, const t_config_map*)> returnMap;

	returnMap["root"] = &ServerConfig::checkDirectiveRoot;
	returnMap["server_name"] = &ServerConfig::checkDirectiveServerName;
	returnMap["client_max_body_size"] = &ServerConfig::checkDirectiveClientMaxBodySize;
	returnMap["error_page"] = &ServerConfig::checkDirectiveErrorPage;
	returnMap["index"] = &ServerConfig::checkDirectiveIndex;
	returnMap["allowed_methods"] = &ServerConfig::checkDirectiveAllowedMethods;
	returnMap["upload_store"] = &ServerConfig::checkDirectiveUploadStore;
	returnMap["autoindex"] = &ServerConfig::checkDirectiveAutoIndex;
	returnMap["return"] = &ServerConfig::checkDirectiveReturn;
	returnMap["cgi_pass"] = &ServerConfig::checkDirectiveCGIpass;
	returnMap["host"] = &ServerConfig::checkDirectiveHost;

	return (returnMap);
}

void (ServerConfig::*ServerConfig::getDirectiveCheckFunction(const std::string& directiveKey))(std::vector<std::string>&, const t_config_map*)
{
	static std::map<std::string, void(ServerConfig::*)(std::vector<std::string>&, const t_config_map*)> functionMap = buildConfigCheckMap();

	std::map<std::string, void(ServerConfig::*)(std::vector<std::string>&, const t_config_map*)>::iterator foundIt = functionMap.find(directiveKey);

	if (foundIt != functionMap.end())
		return (foundIt->second);
	else
		return (NULL);
}

const std::set<in_addr_t>&	ServerConfig::getHostIp() const
{
	return (_host_ip_set);
}
