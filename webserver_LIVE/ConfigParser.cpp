#include "ConfigParser.hpp"

std::string ConfigParser::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n\r\f\v;");
    return str.substr(first, last - first + 1);
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    std::string temp;
    std::istringstream stream(str);
    while (std::getline(stream, temp, delimiter))
        result.push_back(temp);
    return result;
}

static int stringToInt(const std::string& str)
{
    std::stringstream ss(str);
    int result;
    ss >> result;
    return result;
}

RouteConfig ConfigParser::parseRouteConfig(std::ifstream& file, const std::string& locationPath)
{
    RouteConfig routeConfig;
    std::string line;
    std::vector<std::string> cgi_paths;

    routeConfig.setPath(locationPath);
    while (std::getline(file, line) && !line.empty())
    {
		line = trim(line);
        if (line.empty() || line[0] == '#')
            continue ;
        if (line == "}")
            break ;
        else if (line.find("root") == 0 && line.length() > 5)
            routeConfig.setRoot(line.substr(5));
        else if (line.find("index") == 0 && line.length() > 6)
            routeConfig.setIndex(line.substr(6));
        else if (line.find("allowed_methods") == 0 && line.length() > 16)
            routeConfig.setMethods(split(line.substr(16), ' '));
        else if (line.find("cgi_pass") == 0 && line.length() > 9)
        {
            size_t spacePos = line.find(' ', 9);
            if (spacePos != std::string::npos)
            {
                std::string ext = line.substr(9, spacePos - 9);
                std::string path = line.substr(spacePos + 1);
                routeConfig.setCGIs(ext, path);
            }
        } 
        else if (line.find("autoindex") == 0 && line.length() > 10)
            routeConfig.setAutoindex(line.substr(10) == "on");
        else if (line.find("client_max_body_size") == 0 && line.length() > 21)
            routeConfig.setClientMaxBodySize(stringToInt(line.substr(21)));
        else if (line.find("return") == 0 && line.length() > 7)
        {
            size_t spacePos = line.find(' ', 7);
            routeConfig.setReturnStatus(stringToInt(line.substr(7, spacePos - 7)));
            if (spacePos != std::string::npos)
                routeConfig.setReturnValue(line.substr(spacePos + 1));
            else
                routeConfig.setReturnValue("");
        }
        else if (line.find("upload_store") == 0 && line.length() > 13)
            routeConfig.setUploadStore(line.substr(13));
    }
    return routeConfig;
}

ServerConfig ConfigParser::parseConfig(std::ifstream& file)
{
    ServerConfig currentServerConfig;
    std::string line;
    std::string currentLocation;
 
    while (std::getline(file, line))
    {
		line = trim(line);
        if (line.empty() || line[0] == '#')
            continue ;
        if (line == "}")
            return currentServerConfig;
        if (line.find("server_name") != 0 && line.find("server") == 0)
            continue ;
        else if (line.find("listen") == 0 && line.length() > 7)
            currentServerConfig.setPort(stringToInt(line.substr(7)));
        else if (line.find("server_name") == 0 && line.length() > 12)
            currentServerConfig.setServerName(line.substr(12));
        else if (line.find("host") == 0 && line.length() > 5)
            currentServerConfig.setHost(line.substr(5));
        else if (line.find("root") == 0 && line.length() > 5)
            currentServerConfig.setRoot(line.substr(5));
        else if (line.find("index") == 0 && line.length() > 6)
            currentServerConfig.setIndex(line.substr(6));
        else if (line.find("error_page") == 0)
        {
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos && line.length() > spacePos + 5)
            {
                int errorCode = stringToInt(line.substr(spacePos + 1, 3));
                std::string errorPagePath = line.substr(spacePos + 5);
                currentServerConfig.addErrorPage(errorCode, errorPagePath);
            }
        }
        else if (line.find("location") == 0)
        {   
            std::string locationLine = line.substr(9);
            size_t bracePos = locationLine.find('{');
            if (bracePos != std::string::npos)
                currentLocation = trim(locationLine.substr(0, bracePos));
            else
                currentLocation = trim(locationLine);

            RouteConfig routeConfig = parseRouteConfig(file, currentLocation);
            currentServerConfig.addRoute(routeConfig);
        }
        else if (line.find("server") == 0)
            continue ;
    }
    return currentServerConfig;
}

bool ConfigParser::isValidRouteConfig(const std::map<std::string, RouteConfig>routes)
{
    if (routes.find("/") == routes.end())
    {
        std::cout << "Invalid route configuration: Each server must have at least one '/' location block." << std::endl;
        return false;
    }
    for (std::map<std::string, RouteConfig>::const_iterator it = routes.begin(); it != routes.end(); ++it)
    {
        // const std::string& routePath = it->first;
        const RouteConfig& route = it->second;
        const std::vector<std::string>& methods = route.getMethods();
        for (std::vector<std::string>::const_iterator mit = methods.begin(); mit != methods.end(); ++mit)
        {
            if (*mit != "GET" && *mit != "POST" && *mit != "DELETE")
            {
                std::cout << "Invalid route configuration: Allowed methods must be GET, POST, or DELETE only." << std::endl;
                return false;
            }
        }
        int returnStatus = route.getReturnStatus();
        if (returnStatus != 0)
        {
            if (returnStatus < 100 || returnStatus > 599)
            {
                std::cout << "Invalid route configuration: 'return' directive must have a valid HTTP status code (100-599)." << std::endl;
                return false;
            }
            if (returnStatus >= 300 && returnStatus < 400 && route.getReturnValue().empty())
            {
                std::cout << "Invalid route configuration: required location to redirect" << std::endl;
                return false;
            }
        }
        else
        {
            if (methods.empty())
            {
                std::cout << "Invalid route configuration: Allowed methods are required if no 'return' directive is found." << std::endl;
                return false;
            }
        
            if (route.getRoot().empty() && route.getUploadStore().empty())
            {
                std::cout << "Invalid route configuration: Either 'root' or 'upload_store' directive is required if no 'return' directive is found." << std::endl;
                return false;
            }
        }
        const std::map<std::string, std::string>& cgis = route.getCGIs();
        for (std::map<std::string, std::string>::const_iterator cgiIt = cgis.begin(); cgiIt != cgis.end(); ++cgiIt)
        {
            const std::string& extension = cgiIt->first;
            if (extension != ".php" && extension != ".py")
            {
                std::cout << "Invalid route configuration: Only '.php' and '.py' are allowed for cgi_pass." << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool ConfigParser::isValidServerConfig(const ServerConfig& config)
{
    if ((config.getPort() < 1024 || config.getPort() > 65535) && config.getPort() != 80)
    {
        std::cout << "Invalid configuration: Port number must be within the range 1024 - 65535 or equal 80" << std::endl;
        return false;
    }
    if (config.getHost().empty())
    {
        std::cout << "Invalid configuration: Hostname cannot be empty. Please provide a valid hostname." << std::endl;
        return false;
    }
    if (!isValidRouteConfig(config.getRoutes()))
    {
        return false;
    }
    return true;
}

std::vector<ServerConfig> ConfigParser::parseAllConfigs(const std::string& configFilePath)
{
    std::vector<ServerConfig> serverConfigs;
    std::ifstream file(configFilePath.c_str());
    std::string line;

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open config file: " + configFilePath);
    }

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue ;
        if (line.find("server") == 0)
        {
            ServerConfig serverConfig = parseConfig(file);
            if (!isValidServerConfig(serverConfig))
            {
                continue ;
            }
            serverConfigs.push_back(serverConfig);
        }
    }
    file.close();
    if (serverConfigs.empty())
        throw std::runtime_error("No server in configuration file");
    std::map<int, std::set<std::string> > portToServerNames;
    for (std::vector<ServerConfig>::const_iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it)
    {
        int port = it->getPort();
        const std::string& serverName = it->getServerName();

        if (serverName.empty())
        {
            throw std::runtime_error("Invalid configuration: Servers listening on the same port (" + Util::toString(port) + ") must provide a non-empty server_name.");
        }

        if (portToServerNames[port].find(serverName) != portToServerNames[port].end())
        {
            throw std::runtime_error("Invalid configuration: Servers listening on the same port (" + Util::toString(port) + ") must not have the same server_name ('" + serverName + "').");
        }

        portToServerNames[port].insert(serverName);
    }
    return serverConfigs;
}
