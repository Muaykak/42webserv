#include "../../include/classes/CharTable.hpp"
#include "../../include/classes/WebservException.hpp"

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

bool	CharTable::isMatch(const std::string& str) const {
	size_t	strSize = str.size();
	for (size_t i = 0; i < strSize; i++){
		if ((*this)[str[i]] == false)
			return (false);
	}
	return (true);
}

bool	CharTable::isMatch(const std::string& str, size_t startPos) const {
	size_t	strSize = str.size();
	if (startPos >= strSize)
		throw WebservException("CharTable::checkString(" + str + ") INVALID START POS");
	for (size_t i = startPos; i < strSize; i++){
		if ((*this)[str[i]] == false)
			return (false);
	}
	return (true);
}

bool	CharTable::isMatch(const std::string& str, size_t startPos, size_t size) const {
	size_t	endPos = startPos + size;
	if (endPos >= str.size())
		endPos = str.size();
	if (startPos >= endPos)
		throw WebservException("CharTable::checkString(" + str + ") INVALID START POS");
	for (size_t i = startPos; i < endPos; i++){
		if ((*this)[str[i]] == false)
			return (false);
	}
	return (true);
}

bool	CharTable::isNotMatch(const std::string& str) const {
	size_t	strSize = str.size();
	for (size_t i = 0; i < strSize; i++){
		if ((*this)[str[i]] == true)
			return (false);
	}
	return (true);
}

bool	CharTable::isNotMatch(const std::string& str, size_t startPos) const {
	size_t	strSize = str.size();
	if (startPos >= strSize)
		throw WebservException("CharTable::checkString(" + str + ") INVALID START POS");
	for (size_t i = startPos; i < strSize; i++){
		if ((*this)[str[i]] == true)
			return (false);
	}
	return (true);
}

bool	CharTable::isNotMatch(const std::string& str, size_t startPos, size_t size) const {
	size_t	endPos = startPos + size;
	if (endPos >= str.size())
		endPos = str.size();
	if (startPos >= endPos)
		throw WebservException("CharTable::checkString(" + str + ") INVALID START POS");
	for (size_t i = startPos; i < endPos; i++){
		if ((*this)[str[i]] == true)
			return (false);
	}
	return (true);
}

// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findFirstCharset(const std::string& strToFind) const{
	size_t	strSize = strToFind.size();
	for (size_t i = 0; i < strSize; i++){
		if	((*this)[strToFind[i]] == true)
			return (i);
	}
	return (strToFind.npos);
}

// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findFirstCharset(const std::string& strToFind, size_t	startPos) const{
	size_t	strSize = strToFind.size();
	if (startPos >= strSize)
		return (strToFind.npos);
	for (size_t i = startPos; i < strSize; i++){
		if	((*this)[strToFind[i]] == true)
			return (i);
	}
	return (strToFind.npos);
}

// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findFirstCharset(const std::string& strToFind, size_t	startPos, size_t size) const{
	size_t endPos = startPos + size;
	if (endPos > strToFind.size())
		endPos = strToFind.size();
	if (startPos >= endPos)
		return (strToFind.npos);
	for (size_t i = startPos; i < endPos; i++){
		if	((*this)[strToFind[i]] == true)
			return (i);
	}
	return (strToFind.npos);
}

// return npos of the input str if failed
// return the position in the string of the not match char in chartable 
size_t CharTable::findFirstNotCharset(const std::string& strToFind) const{
	size_t	strSize = strToFind.size();
	for (size_t i = 0; i < strSize; i++){
		if	((*this)[strToFind[i]] == false)
			return (i);
	}
	return (strToFind.npos);

}

// return npos of the input str if failed
// return the position in the string of the not match char in chartable 
size_t CharTable::findFirstNotCharset(const std::string& strToFind, size_t	startPos) const{
	size_t	strSize = strToFind.size();
	if (startPos >= strSize)
		return (strToFind.npos);
	for (size_t i = startPos; i < strSize; i++){
		if	((*this)[strToFind[i]] == false)
			return (i);
	}
	return (strToFind.npos);

}
// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findFirstNotCharset(const std::string& strToFind, size_t	startPos, size_t size) const{
	size_t endPos = startPos + size;
	if (endPos > strToFind.size())
		endPos = strToFind.size();
	if (startPos >= endPos)
		return (strToFind.npos);
	for (size_t i = startPos; i < endPos; i++){
		if	((*this)[strToFind[i]] == false)
			return (i);
	}
	return (strToFind.npos);
}

// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findLastCharset(const std::string& strToFind) const{
	size_t	strSize = strToFind.size();
	for (size_t i = strToFind.size() - 1; i > 0; i--){
		if ((*this)[strToFind[i]] == true)
			return (i);
	}
	if (!strToFind.empty()){
		if ((*this)[strToFind[0]] == true)
			return (0);
	}
	return (strToFind.npos);
}

// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findLastCharset(const std::string& strToFind, size_t	startPos) const{
	size_t	strSize = strToFind.size();
	if (startPos >= strSize)
		return (strToFind.npos);
	for (size_t i = startPos; i > 0; i--){
		if ((*this)[strToFind[i]] == true)
			return (i);
	}
	if (!strToFind.empty()){
		if ((*this)[strToFind[0]] == true)
			return (0);
	}
	return (strToFind.npos);
}
// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findLastCharset(const std::string& strToFind, size_t	startPos, size_t size) const{
	size_t endPos = size > startPos? 0: startPos - size;
	if (startPos >= strToFind.size())
		return (strToFind.npos);
	for (size_t i = startPos; i > endPos; i--){
		if ((*this)[strToFind[i]] == true)
			return (i);
	}
	if (!strToFind.empty() && size > startPos){
		if ((*this)[strToFind[0]] == true)
			return (0);
	}
	return (strToFind.npos);
}

// return npos of the input str if failed
// return the position in the string of the not match char in chartable 
size_t CharTable::findLastNotCharset(const std::string& strToFind) const{
	size_t	strSize = strToFind.size();
	for (size_t i = strToFind.size() - 1; i > 0; i--){
		if ((*this)[strToFind[i]] == false)
			return (i);
	}
	if (!strToFind.empty()){
		if ((*this)[strToFind[0]] == false)
			return (0);
	}
	return (strToFind.npos);

}

// return npos of the input str if failed
// return the position in the string of the not match char in chartable 
size_t CharTable::findLastNotCharset(const std::string& strToFind, size_t	startPos) const{
	size_t	strSize = strToFind.size();
	if (startPos >= strSize)
		return (strToFind.npos);
	for (size_t i = startPos; i > 0; i--){
		if ((*this)[strToFind[i]] == false)
			return (i);
	}
	if (!strToFind.empty()){
		if ((*this)[strToFind[0]] == false)
			return (0);
	}
	return (strToFind.npos);

}

// return npos of the input str if failed
// return the position in the string of the match char in chartable 
size_t CharTable::findLastNotCharset(const std::string& strToFind, size_t	startPos, size_t size) const{
	size_t endPos = size > startPos? 0: startPos - size;
	if (startPos >= strToFind.size())
		return (strToFind.npos);
	for (size_t i = startPos; i > endPos; i--){
		if ((*this)[strToFind[i]] == false)
			return (i);
	}
	if (!strToFind.empty() && size > startPos){
		if ((*this)[strToFind[0]] == false)
			return (0);
	}
	return (strToFind.npos);
}
