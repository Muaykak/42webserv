#include "../include/classes/WebServ.hpp"
#include "../include/classes/EnvpWrapper.hpp"
#include "../include/classes/TempFileManager.hpp"

int main(int argc, char **argv, char** envp){
	envData() = EnvpWrapper(envp);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, serverStopHandler);
	signal(SIGQUIT, serverStopHandler);
	signal(SIGTERM, serverStopHandler);

	int ret = 0;

	if (argc == 2){
		try {
			WebServ	webserv(argv[1]);

			webserv.run();
		}
		catch (int &e)
		{
			// only throw int to force exit the program
			ret = e;	
		}

		catch (WebservException &e) {
			std::cout << "WebservError::" << e.what() << std::endl;
		} catch (std::exception &e){
			std::cout << "std::exception::" << e.what() << std::endl;
		}
		//catch (...) {
		//	std::cout << "Unknown Error" << std::endl;
		//}
	}
	return (ret);
}

//int main(int ac, char** av)
//{
//	if (ac != 2)
//		return (0);

//	std::string testStr = av[1];

//	std::pair<std::string, std::vector<std::pair<std::string, std::string> > > outPair;

//	if (Http::httpFieldContentTypeExtract(testStr, outPair) == false)
//	{
//		std::cout << "Failed" << std::endl;
//	}
//	std::cout << "Extract complete" << std::endl;
//}

//int main(){
//	std::cout << "TESTING CONFIG FILE" << std::endl;

//	ConfigData conf("./configs/default.conf");
//	conf.printConfigData();
//}


//int main(int argc, char **argv, char** envp)
//{
//	envData() = EnvpWrapper(envp);

//	char* const* envptest = envData().getEnvp();
//	for (size_t i = 0; envptest[i] != NULL; i++)
//	{
//		std::cout << envptest[i] << std::endl;
//	}
//}
