#ifndef CGIPROCESS_HPP
# define CGIPROCESS_HPP

# include <cstdlib>
# include "WebservException.hpp"
# include <sys/wait.h>

class CgiProcess {
	private:
		pid_t	_pid;
		size_t	*_reference_count;

		void release();
	public:
		CgiProcess();
		CgiProcess(const CgiProcess& obj);
		CgiProcess(pid_t CgiPid);
		CgiProcess& operator=(const CgiProcess& obj);
		CgiProcess& operator=(pid_t pid);
		~CgiProcess();
		pid_t	getPid() const;
};

#endif
