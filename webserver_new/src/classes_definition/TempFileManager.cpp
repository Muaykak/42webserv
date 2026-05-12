#include "../../include/classes/TempFileManager.hpp"
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <cstring>
#include <fstream>


TempFileManager::TempFileManager()
: _nextGenNumber(1)
, isChildProcess(false)
{
}

// will remove all the file
TempFileManager::~TempFileManager()
{
	if (isChildProcess == true)
		return ;

	if (!_tempFileSet.empty())
	{
		std::string tempFilePath;
		std::set<unsigned int>::const_iterator it = _tempFileSet.begin();

		while (it != _tempFileSet.end())
		{
			tempFilePath = TEMP_FILE_DIR + toString(*it);
			std::remove(tempFilePath.c_str());
			++it;
		}
	}
}

void TempFileManager::setIsChild(bool op)
{
	isChildProcess = op;
}

bool TempFileManager::getIsChild() const
{
	return (isChildProcess);
}

unsigned int TempFileManager::generateNewTempFile()
{
	unsigned int thisgennum = _nextGenNumber;

	// create temp file
	{

		std::string tempPath = TEMP_FILE_DIR + toString(thisgennum);

		//int fd;

		std::fstream targetFile;

		while (true)
		{
			//fd = open(tempPath.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
			targetFile.open(tempPath.c_str(), std::ios::in | std::ios::out | std::ios::trunc);

			if (targetFile.is_open())
				break ;
			
			if (errno == EINTR)
			{
				targetFile.clear();
				continue;
			}
			else
				throw WebservException("TempFileManager::fatal error:: cannot generate temporary file in specific directory::" + std::string(std::strerror(errno)));
			

			//if (fd < 0)
			//{
			//	if (errno == EINTR)
			//		continue;
			//	else
			//		throw WebservException("TempFileManager::fatal error:: cannot generate temporary file in specific directory::" + std::string(std::strerror(errno)));
			//}
			//break ;
		}

		//close(fd);
		targetFile.close();
	}

	_tempFileSet.insert(thisgennum);

	// calculate next gen number
	{
		std::set<unsigned int>::iterator it;

		while (true)
		{
			it = _tempFileSet.upper_bound(_nextGenNumber);

			if (it == _tempFileSet.end())
			{
				++_nextGenNumber;
				break ;
			}
			else
			{
				// check if there are at least 1 gap 
				if (_nextGenNumber + 1 < *it)
				{
					++_nextGenNumber;	
					break ;
				}
				else
					++_nextGenNumber;
			}
		}
	}

	return (thisgennum);
}

void TempFileManager::removeTempFile(unsigned int tempfileNum)
{
	// delete temp file

	// check if this filenum exist in the set first
	std::set<unsigned int>::iterator it = _tempFileSet.find(tempfileNum);
	if (it != _tempFileSet.end())
	{
		std::string tempFileName = TEMP_FILE_DIR + toString(tempfileNum);
		int ret = std::remove(tempFileName.c_str());
		if (ret == 0)
		{
			_tempFileSet.erase(it);
			if (tempfileNum < _nextGenNumber)
			_nextGenNumber = tempfileNum;
		}
		else
		{
			// this is if remove failed

			// this is fine
			if (errno == ENOENT)
			{
				_tempFileSet.erase(it);
				if (tempfileNum < _nextGenNumber)
				_nextGenNumber = tempfileNum;
			}
			else if (errno != EBUSY)
			{
				// will not tolerate if not EBUSY
				throw WebservException("TempFileManager::remove failed");
			}
		}
	}
}
