#ifndef DIRECTORYLISTING_HPP
#define DIRECTORYLISTING_HPP

#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>


class DirectoryListing
{
    public:
        static void generateDirectoryListing(const std::string& path, const std::string& outputFilePath);
};

#endif
