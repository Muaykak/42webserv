#include "../../include/classes/FileDescriptor.hpp"


FileDescriptor::FileDescriptor() : _fd(-1), _reference_counter(NULL)
{
}
FileDescriptor::FileDescriptor(int fd) : _fd(fd), _reference_counter(NULL)
{
	if (_fd < 0 || _fd >= MAX_FD)
	{
		std::string errMsg = "FileDescriptor::fd out of range::";
		throw WebservException(errMsg);
	}
	try
	{
		_reference_counter = new size_t(1);
	}
	catch (std::exception &e)
	{
		if (close(_fd) != 0)
			std::cerr << "FileDescriptor::close() failed" << std::strerror(errno) << std::endl;
		throw;
	}
}
FileDescriptor::FileDescriptor(const FileDescriptor &obj) : _fd(obj._fd), _reference_counter(obj._reference_counter)
{
	if (_reference_counter)
		(*_reference_counter)++;
}
FileDescriptor& FileDescriptor::operator=(const FileDescriptor &obj)
{
	if (this != &obj)
	{
		release();
		_reference_counter = obj._reference_counter;
		if (_reference_counter)
			(*_reference_counter)++;
		_fd = obj._fd;
	}
	return (*this);
}
FileDescriptor& FileDescriptor::operator=(int fd)
{
	if (fd < 0 || fd >= MAX_FD)
	{
		std::string errMsg = "FileDescriptor::fd out of range::";
		throw WebservException(errMsg);
	}
	if (_fd != fd)
	{
		size_t *new_reference_counter = NULL;
		try
		{
			new_reference_counter = new size_t(1);
		}
		catch (std::exception &e)
		{
			if (close(fd) != 0)
				std::cerr << "FileDescriptor::close() failed" << std::strerror(errno) << std::endl;
			throw;
		}
		release();
		_fd = fd;
		_reference_counter = new_reference_counter;
	}
	return (*this);
}
FileDescriptor::~FileDescriptor()
{
	release();
}

int	FileDescriptor::getFd() const {
	return (_fd);
}


void FileDescriptor::release()
{
	if (_reference_counter)
	{
		(*_reference_counter)--;
		if (*_reference_counter == 0)
		{
			if (close(_fd) != 0)
				std::cerr << "FileDescriptor::close() failed" << std::strerror(errno) << std::endl;
			delete _reference_counter;
			_reference_counter = NULL;
		}
	}
}
