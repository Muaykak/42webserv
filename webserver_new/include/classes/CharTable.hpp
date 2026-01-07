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
		
		bool checkString(const std::string& str) const;
};

#endif