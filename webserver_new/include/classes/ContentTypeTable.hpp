#ifndef CONTENTTYPETABLE_HPP
# define CONTENTTYPETABLE_HPP

# include <map>
# include <string>

class ContentTypeTable {
	private:
		std::map<std::string, std::string>	_extensionToContentTypeMap;


	public:
		ContentTypeTable();

		std::string extensionToContentType(const std::string& filePath) const;

};

#endif
