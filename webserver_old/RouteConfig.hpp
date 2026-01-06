#ifndef ROUTECONFIG_HPP
# define ROUTECONFIG_HPP

# include <iostream>
# include <map>
# include <string>
# include <vector>

class RouteConfig
{
	private:
		std::string path;
		std::string root;
		std::string index;
		std::vector<std::string> allow_methods;
		std::map<std::string, std::string> CGIs;
		bool autoindex;
		size_t clientMaxBodySize;
		int returnStatus;
		std::string returnValue;
		std::string uploadStore;

	public:
		RouteConfig();

		const std::string& getPath() const;
		const std::string& getRoot() const;
		const std::string& getIndex() const;
		const std::vector<std::string>& getMethods() const;
		const std::map<std::string, std::string>& getCGIs() const;		
		bool getAutoindex() const;
		size_t getClientMaxBodySize() const;
		int getReturnStatus() const;
		const std::string& getReturnValue() const;
		const std::string& getUploadStore() const;

		void setPath(const std::string& path);
		void setRoot(const std::string& root);
		void setIndex(const std::string& index);
		void setMethods(const std::vector<std::string>& methods);
		void setCGIs(const std::string& ext, const std::string& path);
		void setAutoindex(bool autoIndex);
		void setClientMaxBodySize(size_t clientMaxBodySize);
		void setReturnStatus(int status);
		void setReturnValue(const std::string &value);
		void setUploadStore(const std::string &store);


		std::string getCGI(std::string);

		void debug() const;
};

#endif
