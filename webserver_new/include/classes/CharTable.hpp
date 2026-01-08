#ifndef CHARTABLE
# define CHARTABLE

# include <string>
# include <cstring>

class CharTable {
	private:
		bool _allowchar[256];
	public:
		CharTable();
		CharTable(const std::string& charset, bool isAllow);
		CharTable(const CharTable& obj);
		CharTable& operator=(const CharTable& obj);
		~CharTable();

		bool operator[](char c) const;
		
		bool isMatch(const std::string& str, size_t startPos) const;
		bool isMatch(const std::string& str) const;
		bool isNotMatch(const std::string& str, size_t startPos) const;
		bool isNotMatch(const std::string& str) const;

		// return npos of the input str if failed
		size_t findFirstCharset(const std::string& charsetToFind) const;
		// return npos of the input str if failed
		size_t findFirstCharset(const std::string& charsetToFind, size_t	startPos) const;

		// return npos of the input str if failed
		size_t findFirstNotCharset(const std::string& charsetToFind) const;
		// return npos of the input str if failed
		size_t findFirstNotCharset(const std::string& charsetToFind, size_t	startPos) const;

		// return npos of the input str if failed
		size_t findLastCharset(const std::string& charsetToFind) const;
		// return npos of the input str if failed
		size_t findLastCharset(const std::string& charsetToFind, size_t	startPos) const;

		// return npos of the input str if failed
		size_t findLastNotCharset(const std::string& charsetToFind) const;
		// return npos of the input str if failed
		size_t findLastNotCharset(const std::string& charsetToFind, size_t	startPos) const;

};

#endif