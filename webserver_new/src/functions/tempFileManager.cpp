#include "../../include/classes/TempFileManager.hpp"

TempFileManager& tempFileManager()
{
	static TempFileManager tempmanager;

	return (tempmanager);
}