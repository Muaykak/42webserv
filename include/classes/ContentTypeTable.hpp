#ifndef CONTENTTYPETABLE_HPP
# define CONTENTTYPETABLE_HPP

# include <map>
# include <string>
# include <set>

class ContentTypeTable {
	private:
		std::map<std::string, std::string>	_extensionToContentTypeMap;
		std::map<std::string, std::set<std::string> >	_contentTypeMapToExtension;
		std::set<std::string> _fallbackEmptySet;


	public:
		ContentTypeTable();

		std::string extensionToContentType(const std::string& filePath) const;

		const std::set<std::string>& contentTypeToExtension(const std::string& contentType) const;

		const std::map<std::string, std::string>& getExtensionToContentTypeMap() const;
		const std::map<std::string, std::set<std::string> >& getContentTypetoExtensionMap() const;
};

#endif
