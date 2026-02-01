#include "../../include/classes/ConfigData.hpp"
#include "../../include/utility_function.hpp"

ConfigData::ConfigData(const ConfigData& obj){
	_serversConfigs = obj._serversConfigs;
}
ConfigData& ConfigData::operator=(const ConfigData& obj){
	if (this != &obj){
		_serversConfigs = obj._serversConfigs;
	}
	return (*this);
}
ConfigData::~ConfigData(){

}


ConfigData::ConfigData(const std::string &configPath)
{
	// Checking File extention (.conf)
	if (configPath.empty() || configPath.substr(configPath.size() - 5) != ".conf")
	{
		throw WebservException("Wrong file extension! <" + configPath + ">");
	}
	// Check file type
	{
		struct stat statbuf;
		std::string errorMsg = "Checking file type <" + configPath + ">";
		if (stat(configPath.c_str(), &statbuf) == -1)
		{
			errorMsg += "::stat() failed::";
			errorMsg += std::strerror(errno);
			throw WebservException(errorMsg);
		}
		if (S_ISDIR(statbuf.st_mode) == true)
		{
			errorMsg += "::Is A Directory";
			throw WebservException(errorMsg);
		}
	}

	std::ifstream configFile(configPath.c_str());
	if (!configFile.is_open())
	{
		std::string errorMsg = "Cannot Open Config File!::";
		errorMsg += std::strerror(errno);
		throw WebservException(errorMsg);
	}

	// The actual read
	std::vector<s_config_token> tokenList;
	{
		std::string readBuffer;
		while (std::getline(configFile, readBuffer, '\n')){
			splitToken(readBuffer, tokenList);
		}
	}
	if (tokenList.empty()){
		throw WebservException("ConfigData::token list is empty");
	}

	// Put into structure
	{
		bool	isInServerBlock = false;
		bool	isInLocationBlock = false;
		size_t	tokenListSize = tokenList.size();
		size_t	i = 0;
		t_config_map	tempServerConfig;
		std::string					tempDirectiveName;
		std::vector<std::string>	tempDirectiveValue;
		t_location_map	tempLocationMap;
		t_config_map	tempLocationConfig;
		std::string		tempLocationName;
		CharTable	allowChar("0123456789:.", true);

		while (i < tokenListSize){
			if (tokenList[i].token_type == OP_TOKEN){
				if ((tokenList[i].tokenString)[0] == '{'){
					//if (!tempServerConfig.empty() || !tempLocationMap.empty())
					//{
					//	throw WebservException("ConfigData::tempServerConfig, tempLocationMap is not empty");
					//}
					if (!isInServerBlock) {
						if (tempDirectiveName != "server" || !tempDirectiveValue.empty())
							throw WebservException("ConfigData::Each server block can only begin with one \"server\"");
						tempDirectiveName.clear();
						tempDirectiveValue.clear();
						isInServerBlock = true;
					}
					else if (!isInLocationBlock) {
						if (tempDirectiveName != "location")
							throw WebservException("ConfigData::Only \"location\" block can be  inside \"server\" block");
						if (tempDirectiveValue.size() != 1)
							throw WebservException("ConfigData::\"location\" block needs specifically 1 path (location <path> {<location block>})");
						tempLocationName = tempDirectiveValue[0];
						isInLocationBlock = true;
						tempDirectiveName.clear();
						tempDirectiveValue.clear();
					}
					else
						throw WebservException("ConfigData::WRONG CURLY BRACKET(\"{}\")");
				}
				else if ((tokenList[i].tokenString)[0] == '}'){
					if (!isInServerBlock && !isInLocationBlock)
						throw WebservException("ConfigData::WRONG CURLY BRACKET(\"{}\")");
					if (!tempDirectiveName.empty())
						throw WebservException("ConfigData::WRONG CURLY BRACKET(\"{}\")::Directives need to end with semicolon.");
					if (isInLocationBlock) {
						if (tempLocationMap.find(tempLocationName) != tempLocationMap.end())
							throw WebservException("ConfigData::Cannot have multiple block with the same path");
						tempLocationMap.insert(std::make_pair(tempLocationName, tempLocationConfig));
						tempLocationName.clear();
						tempLocationConfig.clear();
						isInLocationBlock = false;
					}
					else {
						// Finding server's port
						t_config_map::const_iterator it = tempServerConfig.find("listen");
						if (it == tempServerConfig.end())
							throw WebservException("ConfigData::need \"listen\" directive to find the server's port");
						if (it->second.size() > 1)
							throw WebservException("ConfigData::\"listen\" to only 1 specified port");
						
						// check if ip:port is valid
						{
							size_t	i = 0;
							size_t	portPos;
							int		port;
							while (i < it->second.size()){
								std::string currStr = it->second[i];
								if (allowChar.isMatch(currStr) == false){
									throw WebservException("ConfigData::\"listen\"::InvalidValue = \"" + currStr + "\"");
									std::cout << "FALSE" << std::endl;
								}
								portPos	= currStr.find_first_of(":");

								if (portPos == currStr.npos){
									if (currStr.size() > 5)
										throw WebservException("ConfigData::\"listen\"::InvalidValue = \"" + currStr + "\"");
									port = std::atoi(currStr.c_str());
								}
								else {
									if (++portPos >= currStr.size())
										throw WebservException("ConfigData::\"listen\"::InvalidValue = \"" + currStr + "\"");
									if (currStr.size() - portPos > 5)
										throw WebservException("ConfigData::\"listen\"::InvalidValue = \"" + currStr + "\"");
									port = std::atoi(currStr.substr(portPos).c_str());
								}
								if (port > 65535 || port < 0)
									throw WebservException("ConfigData::\"listen\"::InvalidValue = \"" + currStr + "\"");
								_serversConfigs[port].push_back(ServerConfig(tempServerConfig, tempLocationMap, port));
								++i;
							}
							tempServerConfig.clear();
							tempLocationMap.clear();
						}
						isInServerBlock = false;
					}
				}
				// receive the ';' indicatind the end of each directives and value
				// we should push config map here
				else {
					if (!isInServerBlock && !isInLocationBlock)
						throw WebservException("ConfigData::SEMICOLON should be only inside a block");
					if (tempDirectiveName.empty())
						throw WebservException("ConfigData::SEMICOLON WRONG");
					if (tempDirectiveValue.empty())
						throw WebservException("ConfigData::Directives need atleast 1 value");
					if (isInLocationBlock){
						tempLocationConfig.insert(std::make_pair(tempDirectiveName, tempDirectiveValue));
						tempDirectiveName.clear();
						tempDirectiveValue.clear();
					}
					else {
						std::vector<std::string>& target = tempServerConfig[tempDirectiveName];

						target.insert(target.end(), tempDirectiveValue.begin(), tempDirectiveValue.end());
						tempDirectiveName.clear();
						tempDirectiveValue.clear();
					}
				}
			}

			else if (tokenList[i].token_type == DATA_TOKEN){
				if (tempDirectiveName.empty())
					tempDirectiveName = tokenList[i].tokenString;
				else 
					tempDirectiveValue.push_back(tokenList[i].tokenString);
			}

			++i;
		}

		checkServersConfigDuplicate();
	}

	// throw SomeExceptions Error when something wrong
}


void ConfigData::checkServersConfigDuplicate() {
	std::map<int, std::vector<ServerConfig> >::iterator it = _serversConfigs.begin();

	size_t	vectorIndex;
	size_t	vectorIndexIncre;
	while (it != _serversConfigs.end()){
		std::vector<ServerConfig>& targetVector = it->second;
		vectorIndex = 0;
		vectorIndexIncre = 1;
		while (vectorIndex < targetVector.size() - 1){
			if (targetVector[vectorIndex] == targetVector[vectorIndex + 1]){
				targetVector.erase(targetVector.begin() + vectorIndex);
				vectorIndex = 0;
				vectorIndexIncre = 1;
				continue;
			}
			++vectorIndex;
			if (vectorIndex == targetVector.size() - 1)
				vectorIndex = vectorIndexIncre++;
		}
		++it;
	}
}

void	ConfigData::splitToken(const std::string &readLine, std::vector<s_config_token>& tokenList){
	size_t indexCurrent = 0;
	size_t indexNext;
	size_t readLineSize = readLine.size();
	CharTable configTokenTable(" \t\v\n\f\r#{};", true);
	while (indexCurrent < readLineSize)
	{
		if (readLine[indexCurrent] == '#')
			break ;

		else if (whiteSpaceTable().operator[](readLine[indexCurrent])){
			//indexNext = readLine.find_first_not_of(" \t\v\n\f\r", indexCurrent);
			indexNext = whiteSpaceTable().findFirstNotCharset(readLine, indexCurrent);
			if (indexNext == readLine.npos)
				break;
			indexCurrent = indexNext;
			continue;
		}

		else if (readLine[indexCurrent] == ';' ||
		readLine[indexCurrent] == '{' ||
		readLine[indexCurrent] == '}'){
			s_config_token	temp_config_data;
			temp_config_data.token_type = OP_TOKEN;
			temp_config_data.tokenString = readLine[indexCurrent];
			tokenList.push_back(temp_config_data);
			++indexCurrent;
			continue;
		}

		else {
			s_config_token	temp_config_data;
			temp_config_data.token_type = DATA_TOKEN;
			//indexNext = readLine.find_first_of(" \t\v\n\f\r#;{}", indexCurrent);
			if (readLine[indexCurrent] == '\"' || readLine[indexCurrent] == '\'')
			{
				if (indexCurrent + 1 >= readLineSize)
					throw WebservException("ConfigData::splitToken::wrong syntax::use of \', \"::" + readLine);
				indexNext = readLine.find_first_of(readLine[indexCurrent], indexCurrent + 1);
				if (indexNext == readLine.npos)
				{
					throw WebservException("ConfigData::splitToken::wrong syntax::use of \', \"::" + readLine);
				}
				else
				{
					size_t	n = indexCurrent + 1 >= indexNext ? 0 : indexNext - indexCurrent - 1;
					if (n > 0)
					{
						temp_config_data.tokenString = readLine.substr(indexCurrent + 1, n);
						tokenList.push_back(temp_config_data);
					}
					indexCurrent = indexNext + 1;
				}
			}
			else {
				indexNext = configTokenTable.findFirstCharset(readLine, indexCurrent);
				if (indexNext == readLine.npos){
					temp_config_data.tokenString = readLine.substr(indexCurrent);
					tokenList.push_back(temp_config_data);
					break ;
				}
				else {
					temp_config_data.tokenString = readLine.substr(indexCurrent, indexNext - indexCurrent);
					tokenList.push_back(temp_config_data);
					indexCurrent = indexNext;
					continue;
				}
			}
		}
	}
}

const std::map<int, std::vector<ServerConfig> >* ConfigData::getServersConfigMap() const {
	return (&_serversConfigs);
}

void	ConfigData::printConfigData() const {
	size_t i = 0;
	std::map<int, std::vector<ServerConfig> >::const_iterator configIt = _serversConfigs.begin();
	while (configIt != _serversConfigs.end()){
		std::cout << "=========== PORT " << configIt->first << "========" << std::endl;
		i = 0;
		while (i < configIt->second.size()){
			(configIt->second)[i].printServerConfig();
			++i;
		}
		std::cout << "===================" << std::endl;
		++configIt;
	}
}

