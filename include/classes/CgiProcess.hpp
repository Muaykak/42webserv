#ifndef CGIPROCESS_HPP
# define CGIPROCESS_HPP

# include <cstdlib>
# include "WebservException.hpp"
# include <sys/wait.h>
# include <ctime>
# include <map>
# include "OptionalData.hpp"

class Socket;

enum e_cgi_process_status
{
	CGI_PROCESS_NO_PROCESS,
	CGI_PROCESS_RUNNING,
	CGI_PROCESS_WAITING,
	CGI_PROCESS_FINISHED
};

class CgiProcess {
	private:
		OptionalData<time_t> lastSignalTimeStamp;
	public:
		e_cgi_process_status status;
		pid_t cgiPid;
		//OptionalData<Socket *> cgiOutSocketPtr;
		//OptionalData<Socket *> cgiInSocketPtr;
		OptionalData<Socket *> clientSocketPtr;
		std::map<int, Socket>* socketMapPtr;

		CgiProcess();
		~CgiProcess();


		/* send sigterm to the process */
		void sigProcess(int signal);

		/* return a status if possible, because with WNOHANG may not return status yet */
		OptionalData<int> waitProcess();

		const OptionalData<time_t>& getTimeLastSigTimeStamp() const;
};

#endif
