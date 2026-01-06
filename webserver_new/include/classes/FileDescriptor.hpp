#ifndef FILEDESCRIPTOR_HPP
# define FILEDESCRIPTOR_HPP

# include <iostream>
# include <unistd.h>
# include "../defined_value.hpp"
# include "WebservException.hpp"
# include <cerrno>
# include <cstring>

// Usage: like int fd = somegetfdfunction()
// The main difference is you should not assign fd that always need to open
// like 0, 1, 2
// FileDescriptor a = open(file)   and toss this class around is fine.
class FileDescriptor
{
private:
	int _fd;
	size_t *_reference_counter;

	void release();
public:
	FileDescriptor();
	FileDescriptor(int fd);
	FileDescriptor(const FileDescriptor &obj);
	FileDescriptor &operator=(const FileDescriptor &obj);
	FileDescriptor &operator=(int fd);
	~FileDescriptor();
	operator int() const;
	int	getFd() const;
};

#endif