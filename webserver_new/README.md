*This project has been created as part of the 42 cirriculum by srussame*

# Description

# Instructions
	How to run
	-makefile
	-usage
	-config

# How to test
## **Basic test**
### GET 
Test GET method
		```bash
		curl localhost:4343/index.html
		```

### POST
Test POST method
		```bash
		curl -X POST -H "Content-Type: plain/text" --data "this is new post" localhost:4343/upload/test.txt
		```

### DELETE
Test DELETE method
		```bash
		curl -X DELETE localhost:4343/upload/uploads/test.txt
		```

### UNKNOWN
Test UNKNOWN method that not implemented
		```bash 
		curl -X UNKNOWN localhost:4343
		```
## Test with CGI
	open your browser to http://localhost:4343
# Resources
	-RFC 9110, 9112, 3875{+request_uri ,redirect status}
### Refereced Sites

### How AI was used in this project

# LOGS

here is where you guys dump your information.

# SRUSSAME

I will try to document this webserv project as much as possible and i hope i cover all the important topics because it is A LOT.

## Introduction

### What is a server?

> definition from [wikipedia](https://en.wikipedia.org/wiki/Server_(computing))

A **server** is a software system that provides data, resources, or services to other computers called "clients". 

So, that means we need to create a program that serves the clients' requests. You fellow 42 Cadets might familiar from the project like minitalk about send and recieve message from other processes, read and write through pipes, byte by byte data checking and manipulation, this project's backbone is also about these, but made easier with those **epoll()** functions to handle I/O events

### What is HTTP ?

As I said that in the layer beneaths server and client, It is just processes try to communicate with each other. whether it is the process in the same local machine or from another machine. when we look into raw data in all those connections over the internet. they are just sending bytes data to each other. And after a while when more and more people using internet to make connections someone came up and just said that we might need some standards or **protocols** to be able to communicate with each other easily

> this is from [developer.mozila.org](https://developer.mozilla.org/en-US/docs/Web/HTTP)

**HTTP** is **Hypertext Transfer Protocol**.
It is an <u>application layer</u> protocol for transmitting hypermedia documents, such as **HTML**. It was designed for ***communication between web browsers and web servers***. So It's just the standard way to read the data from the client and know what the client wants.

### How to start?

For me with zero background in engineering and programming. this project is **incredibly overwhelming** for me and it was really a struggle for me to even know how to start and i will guide you here. First try to atleast establish a **poll()** or **epoll()** or equivalent main loop first before anything else, bind with port, listen and accept connections. These are the core functions and concepts that you need to grasp first before even know what the project is even gonna look like.

This documentation i will try to explain each sections by starting from the main() loop and explain the inside steps by steps and layer by layer as much as possible

## Dive into the project

let's look at the main() function

```cpp
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

```

Let's go line by line i think it is easier for me to explain this way

---

### Environment Data handling

Look at the first line from the main function

```cpp
envData() = EnvpWrapper(envp);
```

the **envData()** return a reference to a static object inside its function, it is just the convention and fancy way kind of global variable. the webserv doesn't forbid the use of global variable yeah? so i think to be able to easily manage environment data is to use this convention.
```cpp
EnvpWrapper& envData(){
	static EnvpWrapper data;
	return (data);
}
```

#### Why we need to handle the environment variable data?

- because we need to handle the CGI which mean we need to spawn processes, and according to CGI standards, there are some environment variables that we need to reconfigure. In which i will explain in later parts.

now let's look into EnvpWrapper class to see how i handle the environment data

```cpp
EnvpWrapper::EnvpWrapper(const char * const * const envp)
:
_envp(NULL),
_needReallocate(true)
{
	if (envp == NULL)
		throw (std::runtime_error("Whaatttt!"));
	
	size_t	sepPos;
	std::string		tempStr;
	for (size_t i = 0; envp[i] != NULL; i++)
	{
		tempStr = envp[i];
		sepPos = tempStr.find_first_of('=');
		if (sepPos == tempStr.npos)
			_map[tempStr].clear();
		else
			_map[tempStr.substr(0, sepPos)].append(tempStr.substr(sepPos + 1));
	}
}
```

This is **EnvpWrapper**'s constructor. It takes the **envp** pointer and build the std::map data to easily configure in the future.

---

### Signals Handling

```cpp
signal(SIGPIPE, SIG_IGN);
signal(SIGINT, serverStopHandler);
signal(SIGQUIT, serverStopHandler);
signal(SIGTERM, serverStopHandler);
```

This is how my webserv project handles the signals. And you might also wanna ask, or evaluator might ask you the same to check your knowledge as well.

#### Why we need to ignore the SIGPIPE signal ?
This is crucial for this project. As the name of the signal said. SIGPIPE will send to your program when you <u>try to read() or write() to a **broken pipe**</u>. the broken pipe in this project always happen when client side might close the connection while our server still not finish sending data. and that moment the SIGPIPE occur. and if we not ignore this signal, the program will immediately crash. And it happens on client side usually so what we can only it is ignore that signal and manage file descriptors through epoll().


move on to other signals handling, you'll see the **serverStopHandler()** function

```cpp
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
	msg +=  " Received. Terminating program....\n";
	write(STDOUT_FILENO, msg.c_str(), msg.size());

	// 1 means to terminate program
	closeWebservSignal() = 1;
}
```

i use the same trick to function that return reference to staic variable. but this variable is special

#### Why we need <u>volatile sig_atomic_t</u> to store the state from the signal handler?

At first i had an idea to **throw()** something out of signal handler, because throw will bubbling up to main and exit cleanly, but it is not.

Normally in a process when a function is being called, it is usually being called from another function. and when C++ runtime destroy a stack or stack unwinding, it uses its own hidden maps generated by the compiler. But signal handler functions are different. Because these functions are not called by another functions, but the OS kernel that manages signals call those functions. Plus the process will not when the signal handler will be called when it still in which stack. So the compiler doesn't really know how to map these signal handler functions to the stack, and may result in undefined behaviors, or crash.

##### **Volatile**

Normally, the C compiler will try to optimize you code to make the program as fast as possible. but as I said that the compiler doesn't know when the signal handler that change the program's state will be called. when the compiler looks into our loop they just optimize the loop by not checking the state variable at all because it sees that no function inside this loop that change this variable. The **volatile** keyword tells the compiler to not assume the value of the variable and must check everytime, that means no optimization here and that important when working with signal handler.

##### **sig_atomic_t**

sig_atomic_t is a special integer type guaranteed by the C standard to be read and written in exactly one, uninterruptible CPU instruction. It just the way of saying that it makes sure that the value written in sig_atomic_t will not be corrupted.

<br>
You will see the main loop that use this signal variable in later parts.

---

### WebServ Class - and all the things

Take a look at the main() you'll see

```cpp
try {
	WebServ	webserv(argv[1]);

	webserv.run();
}
```

Everything all wrapped up inside Webserv class, I don't think this is a good design but i did it this way

let's take a look at the **WebServ**'s constructor

```cpp
class WebServ
{
private:
	// the .conf file that we passed to the program
	const std::string _webservConfigPath;
	ConfigData _configData;
	FileDescriptor _epollFD;
	std::map<int, Socket> sockets;

	epoll_event _epollEvents[MAX_EPOLL_EVENT];
	std::map<int, s_webserv_custom_event> _customEventMap;

	int webservCheckEvent(std::map<int, s_webserv_event>& returnEvents);
	
public:
	WebServ(const std::string &configPath);
	void run();
};

WebServ::WebServ(const std::string &configPath) : _webservConfigPath(configPath), _configData(_webservConfigPath)
{
	/* epoll and server sockets initialize */
}

```

If you look it the initializer lists in the constructor, you will see that it calls another object's constructor which we will take a look at that later. For the brief understanding this constructor takes the **configuration file's path** (.conf) file that stated in the subject and parse it to a proper data structure to make it easy to use. And also after reading the configuration file. This constructor also setups epoll, and server sockets to be ready for the main loop.

#### Deep into WebServ's Constructor

I will throw errors out with any error cases here, i consider them as fatal errors and should terminate program immediately.

#### WHAT IS **EPOLL** ??

Perhaps the most important question that might get asked during the evaluation. **Epoll** is an upgraded version of **poll**. It performs a similar task - to monitor multiple file descriptos to see if I/O operations is possible on any of them 