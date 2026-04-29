#ifndef CGIPROCESS_HPP
# define CGIPROCESS_HPP

# include <cstdlib>
# include "WebservException.hpp"
# include <sys/wait.h>
# include <ctime>
# include "OptionalData.hpp"

enum e_cgi_process_status
{
	CGI_PROCESS_NO_PROCESS,
	CGI_PROCESS_RUNNING,
	CGI_PROCESS_WAITING,
	CGI_PROCESS_FINISHED
};

class CgiProcess {
	private:
	public:
		e_cgi_process_status status;
		bool isCgiProcessOpen;
		pid_t cgiPid;
		bool isCgiInSocketAlive;
		bool isCgiOutSocketAlive;
		int  cgiInSocketFd;
		int  cgiOutSocketFd;
		bool isCgiFinished;

		CgiProcess();


		/* send sigterm to the process */
		void sigProcess(int signal);

		/* return a status if possible, because with WNOHANG may not return status yet */
		OptionalData<int> waitProcess();

};

#endif
