#include "../include/classes/WebServ.hpp"

int main(int argc, char **argv){
	if (argc == 2){
		try {
			WebServ	webserv(argv[1]);
		} catch (WebservException &e) {
			std::cout << "WebservError::" << e.what() << std::endl;
		} catch (std::exception &e){
			std::cout << "std::exception::" << e.what() << std::endl;
		} catch (...) {
			std::cout << "Unknown Error" << std::endl;
		}
	}
}