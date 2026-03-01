#include "../../include/utility_function.hpp"

void serverStopHandler(int signum)
{
	std::string msg;
	msg += "\nSignal => " + std::string(signum == SIGINT ? "SIGINT" : "SIGQUIT");
	msg +=  " Received. Terminating program....\n";
	write(STDOUT_FILENO, msg.c_str(), msg.size());

	// 0 mean to terminate program
	throw int(1);
}
