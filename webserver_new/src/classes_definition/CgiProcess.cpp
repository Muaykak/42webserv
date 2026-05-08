#include "../../include/classes/CgiProcess.hpp"
#include <cerrno>
#include <iostream>

CgiProcess::CgiProcess()
:
lastSignalTimeStamp(0),
status(CGI_PROCESS_NO_PROCESS),
cgiPid(-1)
{

}

CgiProcess::~CgiProcess()
{
	if (status != CGI_PROCESS_NO_PROCESS && status != CGI_PROCESS_FINISHED)
	{
		int ret;

		while (true)
		{
			ret = kill(cgiPid, SIGKILL);
			if (ret != 0)
			{
				if (errno == EINTR)
					continue;
			}
			waitpid(cgiPid, NULL, 0);
				break ;
			/* should announce something when the process is killed */
		}
	}
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
					lastSignalTimeStamp = std::time(NULL);
					status = CGI_PROCESS_WAITING;
					break ;
				}
			}
			else
			{
				lastSignalTimeStamp = std::time(NULL);
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
			else if (ret > 0)
			{
				returnValue = waitstatus;
				status = CGI_PROCESS_FINISHED;
				break ;
			}
			else
				break ;
		}
	}

	return (returnValue);
}

const OptionalData<time_t>& CgiProcess::getTimeLastSigTimeStamp() const
{
	return (lastSignalTimeStamp);
}

