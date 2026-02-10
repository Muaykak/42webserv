#include "../../include/classes/EnvpWrapper.hpp"
#include <stdexcept>

EnvpWrapper::EnvpWrapper()
:_envp(NULL),
_needReallocate(false)
{

}
EnvpWrapper::EnvpWrapper(const EnvpWrapper& obj)
:_map(obj._map),
_needReallocate(true)
{
	reallocate_envp();
}
EnvpWrapper::EnvpWrapper(const char * const * const envp)
: _needReallocate(true),
_envp(NULL)
{
	if (envp == NULL)
		throw (std::runtime_error("Whaatttt!"));
	
	size_t	sepPos;
	std::string		tempStr;
	for (size_t i = 0; envp[i] != NULL; i++)
	{
		tempStr = envp[i];
		sepPos = tempStr.find_first_of('=');
		if (sepPos == tempStr.npos)
			_map[tempStr].clear();
		else
			_map[tempStr.substr(0, sepPos)].append(tempStr.substr(sepPos + 1));
	}
}
EnvpWrapper& EnvpWrapper::operator=(const EnvpWrapper& obj)
{
	if (this != &obj)
	{
		_needReallocate = true;
		_map = obj._map;
	}
	return (*this);
}

EnvpWrapper::~EnvpWrapper()
{
	if (_envp != NULL)
	{
		for (size_t i = 0; _envp[i] != NULL; i++)
			delete[] _envp[i];
		delete[] _envp;
		_envp = NULL;
	}
}

void	EnvpWrapper::reallocate_envp()
{
	if (_envp != NULL)
	{
		for (size_t i = 0; _envp[i] != NULL; i++)
			delete[]  _envp[i];
		delete[] _envp;
		_envp = NULL;
	}

	if (_map.size() == 0)
		return ;
	size_t keyamount = _map.size();
	_envp = new char*[keyamount + 1]();

	size_t i = 0;
	std::map<std::string, std::string>::const_iterator it = _map.begin();
	try
	{
		size_t	line_size = 0;
		std::string tempStr;
		while (it != _map.end())
		{
			line_size = it->first.size() + 1 + it->second.size();
			_envp[i] = new char[line_size + 1]();
			std::memmove(_envp[i], it->first.c_str(), it->first.size());
			_envp[i][it->first.size()] = '=';
			std::memmove(&(_envp[i][it->first.size() + 1]), it->second.c_str(), it->second.size());
			++i;
			++it;
		}
	}
	catch (std::exception &e)
	{
		size_t	j = 0;
		while (j < i)
		{
			if (_envp[j])
				delete[] _envp[j];
			j++;
		}
		delete[] _envp;
		_envp = NULL;
		throw;
	}
	_needReallocate = false;

}

void EnvpWrapper::assignEnv(const std::string& key, const std::string& value)
{
	if (key.empty())
		return ;
	_needReallocate = true;

	_map[key] = value;
	return ;
}
void EnvpWrapper::removeEnv(const std::string& key)
{
	if (key.empty())
		return ;
	
	
	_map.erase(key);
	_needReallocate = true;
}

std::string EnvpWrapper::findValue(const std::string& key) const
{
	std::string returnStr;
	if (key.empty())
		return returnStr;
	
	std::map<std::string, std::string>::const_iterator it = _map.find(key);
	if (it == _map.end())
		return returnStr;
	else
		return it->second;
}

char* const* EnvpWrapper::getEnvp()
{
	if (_needReallocate == true)
		reallocate_envp();
	return _envp;
}

void EnvpWrapper::printEnvp() const
{
	std::map<std::string, std::string>::const_iterator it = _map.begin();

	while (it != _map.end())
	{
		std::cout << it->first << '=' << it->second << std::endl;
		++it;
	}
}
