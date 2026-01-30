#include "../../include/utility_function.hpp"

void	splitString(const std::string& toSplit, const std::string& delimitter, std::vector<std::string>& returnVec){
	returnVec.clear();
	size_t	currIndex = 0;
	size_t	nextIndex;
	while (true){
		nextIndex =	toSplit.find_first_not_of(delimitter, currIndex);

		if (nextIndex == toSplit.npos){
			return ;
		}

		currIndex = nextIndex;

		nextIndex = toSplit.find_first_of(delimitter, currIndex);
		if (nextIndex == toSplit.npos){
			returnVec.push_back(toSplit.substr(currIndex));
			return;
		}
		else {
			returnVec.push_back(toSplit.substr(currIndex, nextIndex - currIndex));
			currIndex = nextIndex;
		}
	}
}
void	splitString(const std::string& toSplit,
		const std::string& delimitter,
		std::vector<std::string>& returnVec, size_t startPos){
	returnVec.clear();
	if (startPos >= toSplit.size())
			return ;
	size_t	currIndex = startPos;
	size_t	nextIndex;
	while (true){
		nextIndex =	toSplit.find_first_not_of(delimitter, currIndex);

		if (nextIndex == toSplit.npos){
			return ;
		}

		currIndex = nextIndex;

		nextIndex = toSplit.find_first_of(delimitter, currIndex);
		if (nextIndex == toSplit.npos){
			returnVec.push_back(toSplit.substr(currIndex));
			return;
		}
		else {
			returnVec.push_back(toSplit.substr(currIndex, nextIndex - currIndex));
			currIndex = nextIndex;
		}
	}
}
void	splitString(const std::string& toSplit,
		const std::string& delimitter,
		std::vector<std::string>& returnVec, size_t startPos, size_t size){
	returnVec.clear();
	if (startPos >= toSplit.size())
			return ;
	size_t	currIndex = startPos;
	size_t	nextIndex;
	size_t	endPos = startPos + size;
	while (true){
		nextIndex =	toSplit.find_first_not_of(delimitter, currIndex);

		if (nextIndex == toSplit.npos || nextIndex >= endPos){
			return ;
		}

		currIndex = nextIndex;

		nextIndex = toSplit.find_first_of(delimitter, currIndex);
		if (nextIndex == toSplit.npos || nextIndex >= endPos){
			returnVec.push_back(toSplit.substr(currIndex, endPos - currIndex));
			return;
		}
		else {
			returnVec.push_back(toSplit.substr(currIndex, nextIndex - currIndex));
			currIndex = nextIndex;
		}
	}
}

void	splitString(const std::string& toSplit, const CharTable& delimitter, std::vector<std::string>& returnVec){
	returnVec.clear();
	size_t	currIndex = 0;
	size_t	nextIndex;
	while (true){
		nextIndex = delimitter.findFirstNotCharset(toSplit, currIndex);

		if (nextIndex == toSplit.npos){
			return ;
		}

		currIndex = nextIndex;

		nextIndex = delimitter.findFirstCharset(toSplit, currIndex);
		if (nextIndex == toSplit.npos){
			returnVec.push_back(toSplit.substr(currIndex));
			return;
		}
		else {
			returnVec.push_back(toSplit.substr(currIndex, nextIndex - currIndex));
			currIndex = nextIndex;
		}
	}
}
void	splitString(const std::string& toSplit,
		const CharTable& delimitter,
		std::vector<std::string>& returnVec, size_t startPos){
	returnVec.clear();
	if (startPos >= toSplit.size())
			return ;
	size_t	currIndex = startPos;
	size_t	nextIndex;
	while (true){
		nextIndex = delimitter.findFirstNotCharset(toSplit, currIndex);

		if (nextIndex == toSplit.npos){
			return ;
		}

		currIndex = nextIndex;

		nextIndex = delimitter.findFirstCharset(toSplit, currIndex);
		if (nextIndex == toSplit.npos){
			returnVec.push_back(toSplit.substr(currIndex));
			return;
		}
		else {
			returnVec.push_back(toSplit.substr(currIndex, nextIndex - currIndex));
			currIndex = nextIndex;
		}
	}
}
void	splitString(const std::string& toSplit,
		const CharTable& delimitter,
		std::vector<std::string>& returnVec, size_t startPos, size_t size){
	returnVec.clear();
	if (startPos >= toSplit.size())
			return ;
	size_t	currIndex = startPos;
	size_t	nextIndex;
	size_t	endPos = startPos + size;
	while (true){
		nextIndex = delimitter.findFirstNotCharset(toSplit, currIndex);

		if (nextIndex == toSplit.npos || nextIndex >= endPos){
			return ;
		}

		currIndex = nextIndex;

		nextIndex = delimitter.findFirstCharset(toSplit, currIndex);
		if (nextIndex == toSplit.npos || nextIndex >= endPos){
			returnVec.push_back(toSplit.substr(currIndex, endPos - currIndex));
			return;
		}
		else {
			returnVec.push_back(toSplit.substr(currIndex, nextIndex - currIndex));
			currIndex = nextIndex;
		}
	}
}
