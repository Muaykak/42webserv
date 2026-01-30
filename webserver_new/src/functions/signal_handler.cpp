#include "../../include/utility_function.hpp"

volatile sig_atomic_t& signal_status()
{
	// one is doesn't need to close yet
	static volatile sig_atomic_t status = 1;

	return status;
}

void serverStopHandler(int signum)
{
	std::string msg;
	msg += "\nSignal => " + std::string(signum == SIGINT ? "SIGINT" : "SIGQUIT");
	msg +=  " Received. Terminating program....\n";
	write(STDOUT_FILENO, msg.c_str(), msg.size());

	// 0 mean to terminate program
	signal_status() = 0;
}
