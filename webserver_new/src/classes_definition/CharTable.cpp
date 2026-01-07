#include "../../include/classes/CharTable.hpp"

CharTable::CharTable(){
	std::memset(_allowchar, 0, 256 * sizeof(bool));
}

CharTable::CharTable(const std::string& charset, bool isAllow){

	bool defaultState = !isAllow;
	for (int i = 0; i < 256; i++){
		_allowchar[i] = defaultState;
	}
	for (size_t i = 0; i < charset.size(); i++){
		_allowchar[static_cast<unsigned char>(charset[i])] = isAllow;
	}
}

CharTable::CharTable(const CharTable& obj){
		std::memmove(_allowchar, obj._allowchar, 256 * sizeof(bool));
}

CharTable& CharTable::operator=(const CharTable& obj){
	if (this != &obj){
		std::memmove(_allowchar, obj._allowchar, 256 * sizeof(bool));
	}
	return (*this);
}

CharTable::~CharTable(){

}

bool	CharTable::operator[](char c) const {
	return (_allowchar[static_cast<unsigned char>(c)]);
}

bool	CharTable::checkString(const std::string& str) const {
	for (size_t i = 0; i < str.size(); i++){
		if ((*this)[str[i]] == false)
			return (false);
	}
	return (true);
}
