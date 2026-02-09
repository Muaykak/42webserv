#include "../../include/classes/CgiProcess.hpp"

CgiProcess::CgiProcess():
_pid(-1),
_reference_count(NULL)
{

}

CgiProcess::CgiProcess(pid_t pid):
_pid(pid),
_reference_count(NULL)
{
	if (pid < 1)
		throw WebservException("CgiProcess::pid is invalid");

	try
	{
		_reference_count = new size_t(1);
	}
	catch (std::exception &e)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0);
		throw;
	}
}

CgiProcess::CgiProcess(const CgiProcess& obj):
_pid(obj._pid),
_reference_count(obj._reference_count)
{
	if (_reference_count)
		(*_reference_count)++;
}

CgiProcess& CgiProcess::operator=(const CgiProcess& obj)
{
	if (this != &obj)
	{
		release();
		_reference_count = obj._reference_count;
		if (_reference_count)
			(*_reference_count)++;
		_pid = obj._pid;
	}
	return (*this);
}
CgiProcess& CgiProcess::operator=(pid_t pid)
{
	if (pid < 1)
		throw WebservException("CgiProcess::pid is invalid");
	if (_pid != pid)
	{
		size_t	*new_ref_count = NULL;
		try
		{
			new_ref_count = new size_t(1);
		}
		catch (std::exception &e)
		{
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			throw;
		}
		release();
		_pid = pid;
		_reference_count = new_ref_count;	
	}
	return (*this);
}

CgiProcess::~CgiProcess()
{
	release();
}

void	CgiProcess::release()
{
	if (_reference_count)
	{
		(*_reference_count)--;
		if (*_reference_count == 0)
		{
			kill(_pid, SIGKILL);
			waitpid(_pid, NULL, 0);
			delete _reference_count;
			_reference_count = NULL;
			_pid = -1;
		}
	}
}
