#include "../include/classes/WebServ.hpp"



//int main(int argc, char **argv){
//	signal(SIGPIPE, SIG_IGN);
//	signal(SIGINT, serverStopHandler);
//	signal(SIGQUIT, serverStopHandler);
//	if (argc == 2){
//		try {
//			WebServ	webserv(argv[1]);
//		} catch (WebservException &e) {
//			std::cout << "WebservError::" << e.what() << std::endl;
//		} catch (std::exception &e){
//			std::cout << "std::exception::" << e.what() << std::endl;
//		} catch (...) {
//			std::cout << "Unknown Error" << std::endl;
//		}
//	}
//}


int main(){
	std::cout << "TESTING CONFIG FILE" << std::endl;

	ConfigData conf("./configs/default.conf");
	conf.printConfigData();
}