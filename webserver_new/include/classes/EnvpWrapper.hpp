#ifndef ENVPWRAPPER_HPP
# define ENVPWRAPPER_HPP
# include <map>
# include <string>
# include <cstring>
# include <iostream>
# include "../../include/utility_function.hpp"

class EnvpWrapper
{
	private:
		char	**_envp;
		bool	_needReallocate;
		std::map<std::string, std::string>	_map;

		void	reallocate_envp();
	public:
		EnvpWrapper();
		EnvpWrapper(const EnvpWrapper& obj);
		EnvpWrapper(const char * const * const envp);
		EnvpWrapper& operator=(const EnvpWrapper& obj);
		~EnvpWrapper();
		void	assignEnv(const std::string& key, const std::string& value);
		void	removeEnv(const std::string& key);
		std::string findValue(const std::string& key) const;
		char* const* getEnvp();
		void	printEnvp() const;
};

#endif