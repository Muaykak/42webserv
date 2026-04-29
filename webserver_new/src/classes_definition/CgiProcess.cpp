#include "../../include/classes/CgiProcess.hpp"
#include <cerrno>

CgiProcess::CgiProcess()
: isCgiProcessOpen(false),
cgiPid(-1),
isCgiInSocketAlive(false),
isCgiOutSocketAlive(false),
cgiInSocketFd(-1),
cgiOutSocketFd(-1),
isCgiFinished(false)
{

}


void CgiProcess::sigProcess(int signal)
{
	if (status == CGI_PROCESS_RUNNING)
	{
		int ret;
		while (true)
		{
			ret = kill(cgiPid, signal);
			if (ret != 0)
			{
				if (errno == EINTR)
					continue ;
				else
				{
					status = CGI_PROCESS_WAITING;
					break ;
				}
			}
			else
			{
				status = CGI_PROCESS_WAITING;
				break ;
			}
		}
	}

	return ;
}

OptionalData<int> CgiProcess::waitProcess()
{
	OptionalData<int> returnValue;

	if (status == CGI_PROCESS_WAITING)
	{
		int ret;
		int waitstatus = 0;
		while (true)
		{
			ret = waitpid(cgiPid, &waitstatus, WNOHANG);

			if (ret < 0)
			{
				if (errno == EINTR)
					continue ;
				else
					break ;
			}
			else
			{
				returnValue = waitstatus;
				status = CGI_PROCESS_FINISHED;
				break ;
			}
		}
	}

	return (returnValue);
}

