#include "../../include/utility_function.hpp"

TempFileManager& tempfileManager()
{
	static TempFileManager tempmanager;

	return (tempmanager);
}