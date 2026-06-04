#include "../../include/utility_function.hpp"

volatile sig_atomic_t& closeWebservSignal()
{
	static volatile sig_atomic_t webservisClose = 0;

	return (webservisClose);
}

void serverStopHandler(int signum)
{
	std::string msg;
	msg += "\nSignal => ";
	{
		std::string signalstr;

		if (signum == SIGPIPE)
			signalstr = "SIGPIPE";
		else if (signum == SIGINT)
			signalstr = "SIGINT";
		else if (signum == SIGTERM)
			signalstr = "SIGTERM";
		else if (signum == SIGQUIT)
			signalstr = "SIGQUIT";

		msg += signalstr;
	}
	msg +=  " Received. Terminating program....";
	std::cout << msg << std::endl;
	//write(STDOUT_FILENO, msg.c_str(), msg.size());

	// 0 mean to terminate program
	closeWebservSignal() = 1;
}
