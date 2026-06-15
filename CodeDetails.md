# Dive into the project

### Contents
- **1. main() & Initial Setup**
  - [The main() Function](#1-the-main-function)
  - [Environment Data Handling](#2-environment-data-handling-envpwrapper)
  - [Signals Handling](#3-signals-handling) (SIGPIPE, SIGINT/SIGTERM, volatile sig_atomic_t)
- **2. WebServ Initialization**
  - [WebServ Constructor](#4-webserv-constructor)
  - [What is Epoll?](#5-what-is-epoll)
  - [Setting Up Sockets](#6-setting-up-sockets-socketsetupserversocket) (O_NONBLOCK, SO_REUSEADDR)
- **3. The Event Loop**
  - [The Run Loop](#7-the-run-loop-webservrun)
  - [Checking Events](#8-checking-events-webservcheckevent) (epoll_wait, EINTR)
  - [Handling Events](#9-handling-events-webservhandlesocketevents) (Function pointers)
- **4. Socket Wrapper & RAII**
  - [RAII FileDescriptor](#10-raii-filedescriptor)
  - [Socket Types](#11-socket-types)
  - [Server Socket Handler](#12-server-socket-handler-handleserversocketevent) (accept loop)
  - [Client Socket Handler](#13-client-socket-handler-handleclientsocketevent) (EPOLLIN/EPOLLOUT)
- **5. HTTP Engine**
  - [Reading from Client](#14-reading-from-client-httpreadfromclient)
  - [Request Parsing State Machine](#15-request-parsing-state-machine)
  - [Parsing Chunked Transfer Encoding](#16-parsing-chunked-transfer-encoding)
  - [Writing to Client](#17-writing-to-client-httpwritetoclient) (Dynamic EPOLLOUT Scheduling)
- **6. CGI Subsystem**
  - [Fork & Pipe Setup](#18-fork--pipe-setup) (dup2 redirection, envp)
  - [Non-blocking CGI I/O](#19-non-blocking-cgi-io) (sendToCGI, readFromCGI)
  - [Process Tracking](#20-process-tracking-cgiprocesswaitprocess) (waitpid with WNOHANG)
- **7. Config Parser**
  - [ConfigData & ServerConfig](#21-configdata--serverconfig)
- **8. Utility Classes & Decisions**
  - [OptionalData](#22-optionaldata) (C++98 optional wrapper)
  - [TempFileManager](#23-tempfilemanager) (RAM protection)
  - [Why std::ifstream instead of raw open()?](#24-why-stdifstream-instead-of-raw-open)
  - [Why CharTable instead of string search?](#25-why-chartable-instead-of-string-search)
  - [String/IP Helper Utilities](#26-stringip-helper-utilities) (split, ft_trim, tolower, ip_conversion)
- **9. Request Lifecycle Summary**
  - [Summary of the WebServ Request Lifecycle](#summary-of-the-webserv-request-lifecycle)

---

## 1. The main() Function

Let's look at the main() function. This is where the execution starts:

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
	}
	return (ret);
}
```

Let's go line by line because i think it is easier for me to explain this way.

---

## 2. Environment Data Handling: `EnvpWrapper`

Look at the first line from the main function:

```cpp
envData() = EnvpWrapper(envp);
```

The **envData()** function returns a reference to a static object inside its function. It is just a convention and a fancy way of making a global variable. The webserv subject doesn't forbid the use of global variables, yeah? So i think to be able to easily manage environment data is to use this convention:

```cpp
EnvpWrapper& envData(){
	static EnvpWrapper data;
	return (data);
}
```

### Why we need to handle the environment variable data?
Because we need to handle CGI, which means we need to spawn child processes, and according to CGI standards, there are some environment variables that we need to reconfigure. In which i will explain in later parts.

Now let's look into the `EnvpWrapper` class to see how i handle the environment data:

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

This is **EnvpWrapper**'s constructor. It takes the raw `envp` pointer from the OS and builds a `std::map` data structure to make it easy to configure in the future.

---

## 3. Signals Handling

Looking back at `main()`, we set up our signals:

```cpp
signal(SIGPIPE, SIG_IGN);
signal(SIGINT, serverStopHandler);
signal(SIGQUIT, serverStopHandler);
signal(SIGTERM, serverStopHandler);
```

This is how my webserv project handles the signals. And you might also wanna ask, or evaluator might ask you the same to check your knowledge as well.

### Why we need to ignore the SIGPIPE signal?
This is crucial for this project. As the name of the signal says, SIGPIPE is sent to your program when you <u>try to read() or write() to a **broken pipe**</u>. The broken pipe in this project always happens when the client side closes the connection while our server has not finished sending data. At that moment the SIGPIPE occurs. If we do not ignore this signal, the program will immediately crash. Since it happens on the client side, all we can do is ignore that signal and manage file descriptors through epoll().

Move on to other signals handling, you'll see the **serverStopHandler()** function:

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

i use the same trick for the function that returns a reference to a static variable. But this variable is special.

### Why we need <u>volatile sig_atomic_t</u> to store the state from the signal handler?
At first i had an idea to **throw()** something out of the signal handler, because throw will bubble up to main and exit cleanly, but it does not.

Normally in a process when a function is being called, it is usually being called from another function, and when C++ runtime destroys a stack or does stack unwinding, it uses its own hidden maps generated by the compiler. But signal handler functions are different. Because these functions are not called by other functions, but the OS kernel that manages signals calls those functions. Plus the process will not know when the signal handler will be called or what stack state it's in. So the compiler doesn't really know how to map these signal handler functions to the stack, and throwing here may result in undefined behaviors or crashes.

#### **Volatile**
Normally, the C++ compiler will try to optimize your code to make the program as fast as possible. But as I said, the compiler doesn't know when the signal handler that changes the program's state will be called. When the compiler looks into our loop, it might optimize the loop by not checking the state variable at all, because it sees that no function *inside this loop* changes this variable. The **volatile** keyword tells the compiler to not assume the value of the variable and check it every time. That means no optimization here, which is important when working with signal handlers.

#### **sig_atomic_t**
`sig_atomic_t` is a special integer type guaranteed by the C standard to be read and written in exactly one, uninterruptible CPU instruction. It is just the way of making sure that the value written in `sig_atomic_t` will not be corrupted if a signal interrupts the program mid-write.

---

## 4. WebServ Constructor

Now we instantiate WebServ: `WebServ webserv(argv[1]);`

Everything is wrapped up inside the Webserv class. I don't think this is a good design, but i did it this way:

```cpp
WebServ::WebServ(const std::string &configPath) : _webservConfigPath(configPath), _configData(_webservConfigPath)
{
	// epoll creation
	try {
		_epollFD = epoll_create(1);
	} catch (std::exception &e){
		std::string epoll_msg = e.what();
		epoll_msg += "::epoll_create() failed";
		throw WebservException(epoll_msg);
	}

	//create server socket
	{
		const std::map<int, std::vector<ServerConfig> > *serversConfigMapPtr = _configData.getServersConfigMap();
		std::map<int, std::vector<ServerConfig> >::const_iterator serversConfigIterator = serversConfigMapPtr->begin();
		s_webserv_event_controller tempEventController;
		tempEventController.customEventMap = &_customEventMap;
		tempEventController.epollFD = _epollFD;
		while (serversConfigIterator != serversConfigMapPtr->end()){
			try {
				int socketFdInt = socket(AF_INET, SOCK_STREAM, 0);

				sockets.insert(std::make_pair(socketFdInt, Socket(socketFdInt)));
				sockets[socketFdInt].setupServerSocket(&serversConfigIterator->second, tempEventController, &sockets);
			} 
			catch (std::exception &e) {
				throw WebservException("::Create serverSocket failed::" + std::string(e.what()));
			}
			++serversConfigIterator;
		}
	}
}
```

*   `_configData(_webservConfigPath)`: Before the constructor body even runs, the initializer list constructs `_configData`. This parses the config file, setting up the mapping of ports and routes.
*   `_epollFD = epoll_create(1)`: Creates our epoll instance. The file descriptor returned is wrapped inside a `FileDescriptor` object.
*   `socket(AF_INET, SOCK_STREAM, 0)`: Creates a socket. `AF_INET` means IPv4, and `SOCK_STREAM` means TCP.
*   `sockets.insert(...)`: We store the socket in a map of active sockets.
*   `sockets[socketFdInt].setupServerSocket(...)`: This sets up the listening port. Let's look inside this function next!

---

## 5. What is Epoll?

Perhaps the most important question that might get asked during the evaluation. **Epoll** is an upgraded version of **poll**. It performs a similar task - to monitor multiple file descriptors to see if I/O operations (reading or writing data) is possible on any of them.

#### **The Simple Analogy:**
*   **The Blocking Way (Picking up phones one by one)**: Imagine you are a clerk in an office with 100 telephones. To check if anyone is calling, you pick up Phone #1 and wait. If no one is there, you wait and wait. Meanwhile, Phone #50 is ringing like crazy, but you can't hear it because you are stuck waiting on Phone #1. This is **blocking** I/O.
*   **The Poll Way (Running around checking)**: You run around the room picking up each of the 100 phones really fast: "Anyone there? No. Anyone there? No. Anyone there? Yes!" This works, but it wastes all your energy running in circles when no one is calling.
*   **The Epoll Way (The Smart Bell)**: You tell the phone company (the OS kernel): "Here is a list of my 100 phones. Wake me up only when one of them rings, and tell me exactly which ones are ringing." You sit back, take a nap, and when Phone #3 and Phone #77 ring, the kernel wakes you up and hands you a list: `[3, 77]`. You answer only those two. That's **epoll**! It allows our server to handle thousands of clients at the same time using a single process and thread, without eating up all the CPU.

---

## 6. Setting Up Sockets: `Socket::setupServerSocket()`

This function is inside `Socket.cpp`. It prepares our socket to listen for incoming connections.

```cpp
bool Socket::setupServerSocket(const std::vector<ServerConfig> *serversConfig,
		const s_webserv_event_controller &eventController, std::map<int, Socket>* socketMap)
{
	_eventController = eventController;
	_socketType = SERVER_SOCKET;
	_socketMap = socketMap;
	_serversConfig = serversConfig;

	if (fcntl(_socketFD.getFd(), F_SETFL, O_NONBLOCK) != 0)
		throw WebservException("Socket::fcntl() O_NONBLOCK Error");

	int opt = 1;
	if (setsockopt(_socketFD.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		Logger::log(LC_NOTE, "Fail to set socket for reuse socket");

	sockaddr_in sv_addr;
	std::memset(&sv_addr, 0, sizeof(sockaddr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = INADDR_ANY;
	_server_listen_port = (*_serversConfig)[0].getPort();
	sv_addr.sin_port = htons(_server_listen_port);

	if (bind(_socketFD.getFd(), (struct sockaddr *)&sv_addr, sizeof(sv_addr)) != 0)
		throw WebservException("Socket::bind() failed");

	if (listen(_socketFD.getFd(), MAX_LISTENSOCKET_CONNECTION) != 0)
		throw WebservException("Socket::listen() failed");

	// Add the server socket to epoll monitoring event
	epoll_event event;
	std::memset(&event, 0, sizeof(event));
	event.events = EPOLLIN;
	event.data.fd = _socketFD.getFd();
	if (epoll_ctl(_eventController.epollFD.getFd(), EPOLL_CTL_ADD, _socketFD.getFd(), &event) != 0)
		throw(WebservException("epoll_ctl() Error"));

	_lastEventTime = std::time(NULL);
	handleEventPtr = &Socket::handleServerSocketEvent;

	return (true);
}
```

#### **Why do we need non-blocking `O_NONBLOCK`?**
If a socket is blocking (the default), calling `read()` or `write()` will freeze the program until data is sent/received. If a client disconnects halfway or has a slow network, our entire server stops! Setting `O_NONBLOCK` tells the OS: "If the operation can't finish right now, return immediately with `EAGAIN` or `EWOULDBLOCK` instead of freezing."

#### **Why do we need `SO_REUSEADDR`?**
When our server shuts down, the port remains in a kernel state called `TIME_WAIT` for a couple of minutes to catch late packets. If we try to restart the server immediately, `bind()` will fail with "Address already in use". Setting `SO_REUSEADDR` tells the kernel: "Let me reuse this port immediately, i don't want to wait."

#### **Why do we set `handleEventPtr = &Socket::handleServerSocketEvent`?**
This is a member function pointer! Instead of using complex C++ inheritance (which can be slow and hard to read), i use a function pointer to dynamically change what method is called when an event happens on a socket.
*   For server sockets, it points to `handleServerSocketEvent`.
*   For client sockets, it points to `handleClientSocketEvent`.
*   For CGI sockets, it points to `handleCGISocketEvent`.

---

## 7. The Run Loop: `WebServ::run()`

Once setup is complete, `main()` calls `webserv.run()`. This starts our main event loop:

```cpp
void WebServ::run(){
	//epoll loop
	{
		int returnEventsAmount;
		std::map<int, s_webserv_event>::const_iterator eventIt;
		std::map<int, s_webserv_event> returnEvents;
		Logger::log(LC_DEBUG, "Webserv is Waiting for connection!");
		while (closeWebservSignal() == 0){

			returnEventsAmount = webservCheckEvent(returnEvents);

			// ERROR but should retry if EINTR
			if (returnEventsAmount < 0){
				if (errno == EINTR){
					Logger::log(LC_DEBUG, "epoll_wait() got interrupted.");
					if (closeWebservSignal() == 1)
					{
						Logger::log(LC_DEBUG, "Closing the Webserv");
						break ;
					}
					Logger::log(LC_DEBUG, "Retrying... Webserv is Waiting for connection!");
					continue;
				}
				throw WebservException("epoll_wait() failed::" + std::string(std::strerror(errno)));
			}

			handleSocketEvents(eventIt, returnEvents);
			// check socket timeout
			checkSocketTimeOut();
		}
	}
}
```

*   `while (closeWebservSignal() == 0)`: The heartbeat of our server. It loops forever until a signal (like `SIGINT` / Ctrl+C) tells it to stop.
*   `webservCheckEvent()`: Inside this, we call `epoll_wait()`. This function freezes our program and puts the CPU to sleep until something happens (like a client sending data) or it times out. Once active, it populates `returnEvents` with a list of active sockets.
*   `handleSocketEvents()`: Iterates through the list of active sockets and calls their corresponding event handlers.
*   `checkSocketTimeOut()`: A cleanup pass. If a client connects and then goes silent for a long time, we disconnect them to free up resources. If a CGI script runs forever, we terminate it.

---

## 8. Checking Events: `WebServ::webservCheckEvent()`

This function sits and waits for incoming network events.

```cpp
int WebServ::webservCheckEvent(std::map<int , s_webserv_event>& returnEvents)
{
	returnEvents.clear();
	
	/* here we would need to use the epoll_wait*/
	int epollWaitReturnEvent = epoll_wait(_epollFD.getFd(), _epollEvents, MAX_EPOLL_EVENT, WEBSERV_EPOLL_WAIT_MILLISEC);

	/* if the return value is less than 0 should return back because some error might occur*/
	if (epollWaitReturnEvent < 0)
		return (epollWaitReturnEvent);

	/* iterate to all events*/
	for (int i = 0; i < epollWaitReturnEvent; i++)
	{
		returnEvents[_epollEvents[i].data.fd].epollEventData = &_epollEvents[i];
		returnEvents[_epollEvents[i].data.fd].eventFd = _epollEvents[i].data.fd;
	}

	// then for custom events
	std::map<int, s_webserv_custom_event>::const_iterator it = _customEventMap.begin();
	while (it != _customEventMap.end())
	{
		returnEvents[it->first].customEventData = it->second;
		returnEvents[it->first].eventFd = it->first;
		++it;
	}

	/* then need to clear the _customEventMap */
	_customEventMap.clear();

	/* return the size of the map*/
	return (returnEvents.size());
}
```

*   `epoll_wait(...)`: Tells the OS: "Put our process to sleep. Wake us up if any file descriptors in `_epollFD` have activity, or if `WEBSERV_EPOLL_WAIT_MILLISEC` passes."
*   `for (int i = 0; i < epollWaitReturnEvent; i++)`: We loop through all the active events returned by the kernel and store them in `returnEvents`.
*   **What are Custom Events?** Sometimes we need to trigger actions internally (like redirecting a request locally, or shutting down a client due to a manual timeout). We queue these in `_customEventMap` and merge them into our main event processing loop so we can handle them in the exact same place!

---

## 9. Handling Events: `WebServ::handleSocketEvents()`

Once we have our list of active sockets, we process them one by one.

```cpp
void WebServ::handleSocketEvents(std::map<int, s_webserv_event>::const_iterator& eventIt, std::map<int, s_webserv_event>& returnEvents)
{
	eventIt = returnEvents.begin();
	while (eventIt != returnEvents.end())
	{
	    std::map<int, Socket>::iterator sockIt = sockets.find(eventIt->first);
	    if (sockIt != sockets.end())
	    {
	        if (sockIt->second.handleEvent(returnEvents[eventIt->first]) == false)
	        {
	            sockets.erase(sockIt);
	        }
	    }
	    ++eventIt;
	}
}
```

*   `sockets.find(eventIt->first)`: Looks up the file descriptor in our `sockets` map.
*   `sockIt->second.handleEvent(...)`: Calls the socket's event handler.
*   If `handleEvent()` returns `false`, it means the connection was closed. We erase it from our `sockets` map.

---

## 10. RAII `FileDescriptor`

In C++, if you open a file descriptor (like a socket or a file) and your code throws an exception or returns early, you might forget to call `close()`. If this happens repeatedly, the OS will run out of file descriptors, and your server will crash.
To prevent this, we wrote a wrapper class:

```cpp
class FileDescriptor {
private:
    int _fd;
public:
    FileDescriptor() : _fd(-1) {}
    FileDescriptor(int fd) : _fd(fd) {}
    ~FileDescriptor() {
        if (_fd >= 0) {
            close(_fd);
        }
    }
    // ... copy constructors and helpers
};
```
When a `FileDescriptor` object goes out of scope (e.g., when a function returns or an error throws), its **destructor** automatically runs and calls `close(_fd)`. This concept is called **Resource Acquisition Is Initialization (RAII)**. It guarantees we never leak file descriptors!

---

## 11. Socket Types

A socket in our system can represent different things. We use the `_socketType` enum to distinguish them:
*   `SERVER_SOCKET`: A listening socket bound to a port, waiting for connections.
*   `CLIENT_SOCKET`: A connected web browser.
*   `CGI_FD_STDIN`: A pipe pointing to a CGI script's standard input.
*   `CGI_FD_STDOUT`: A pipe reading from a CGI script's standard output.

---

## 12. Server Socket Handler: `handleServerSocketEvent`

When a client tries to connect to our port, epoll triggers `EPOLLIN` on our server socket.

```cpp
bool Socket::handleServerSocketEvent(const s_webserv_event &event)
{
	if (event.epollEventData.hasData() == true)
	{
		// ... (error checks)
		else if (event.epollEventData->events & EPOLLIN)
		{
			sockaddr_in client_address;
			socklen_t len = sizeof(client_address);
			int client_socket;
			while (true)
			{
				client_socket = accept(event.epollEventData->data.fd, (sockaddr *)&client_address, &len);
				if (client_socket > 0)
				{
					_socketMap->insert(std::make_pair(client_socket, Socket(client_socket)));
					(*_socketMap)[client_socket]._client_addr_in = client_address.sin_addr.s_addr;
					(*_socketMap)[client_socket].setServerIpHost(_server_ip_host);
					(*_socketMap)[client_socket].setupClientSocket(_serversConfig, _eventController, _socketMap);
				}
				else
					break ;
			}
		}
	}
	return (true);
}
```

*   `while (true)`: We run a loop because multiple clients might connect at the exact same millisecond. Since we set the server socket to non-blocking, we must keep calling `accept()` until it returns `< 0` (meaning there are no more clients waiting in the connection queue).
*   `accept(...)`: Extracts the first connection request on the queue, creates a new socket file descriptor `client_socket`, and returns it.
*   `setupClientSocket(...)`: Sets this new socket to non-blocking, registers it with epoll (watching for `EPOLLIN` to read their HTTP requests), and instantiates a new `Http` object to handle this client.

---

## 13. Client Socket Handler: `handleClientSocketEvent`

This function handles reading requests from and writing responses to a web browser.

```cpp
bool Socket::handleClientSocketEvent(const s_webserv_event &event)
{
	if (event.epollEventData.hasData() == true)
	{
		// ... (error checks)
		try {
			if (event.epollEventData->events & EPOLLOUT){
				http->writeToClient();
			}
			if ((event.epollEventData->events & EPOLLIN) && timeOutMarked == false){
				http->readFromClient();
			}
		}
		catch (std::exception &e) {
			return (false); // Disconnect client on error
		}
	}
	return http->isKeepConnection();
}
```
*   `EPOLLIN`: The client sent us HTTP request bytes (e.g. `GET /index.html`). We call `http->readFromClient()`.
*   `EPOLLOUT`: The client's socket is ready to receive response bytes from us. We call `http->writeToClient()`.

---

## 14. Reading from Client: `Http::readFromClient()`

Because packets travel in fragments over the internet, a client might send a request header in several tiny pieces. We cannot assume we will read the entire request at once.

```cpp
void Http::readFromClient()
{
	ssize_t	readAmount;

	readAmount = recv(_clientSocketPtr->getSocketFD().getFd(), _recvBuffer.data(), HTTP_RECV_BUFFER, 0);

	if (readAmount == 0)
	{
		Logger::log(LC_CONN_LOG, "Disconnecting client Socket#%d", _clientSocketPtr->getSocketFD().getFd());
		_keepConnection = false;
		return ;
	}
	else if (readAmount < 0)
	{
		return ;
	}
	else 
	{
		_requestBuffer.append(_recvBuffer.data(), readAmount);
		try
		{
			processingRequestBuffer();
		}
		catch (HttpThrowStatus &status)
		{
			Logger::log(LC_INFO, "Http::socket#%d::reponse with status code %d::%s", _clientSocketPtr->getSocketFD().getFd(), status.statusCode(), status.message().c_str());
			httpRequest.clear();
		}
		catch (std::exception &e)
		{
			Logger::log(LC_ERROR, "Http::socket#%d::Exception: %s", _clientSocketPtr->getSocketFD().getFd(), e.what());
			_keepConnection = false;
		}
	}
	
	if (_isEpollout == false && _httpResponseList.empty() == false && _httpResponseList.front().hasSomethingtoSend() == true)
	{
		epoll_event	event;
		std::memset(&event, 0, sizeof(event));
		event.events = EPOLLIN | EPOLLOUT;
		event.data.fd = _clientSocketPtr->getSocketFD().getFd();

		if (epoll_ctl(_clientSocketPtr->getEpollFD().getFd(), EPOLL_CTL_MOD, _clientSocketPtr->getSocketFD().getFd(), &event) == -1)
		{
			std::string msg = "Http::readFromClient()::epoll_ctl()::error::";
			msg += std::strerror(errno);
			throw WebservException(msg);
		}
		_isEpollout = true;
	}
}
```

### Why do we modify epoll to watch for `EPOLLOUT` here?
By default, we only watch for `EPOLLIN` on client sockets (listening for incoming data). We don't watch for `EPOLLOUT` initially.
**Why?** Because a socket is almost always ready to accept write bytes. If we monitored `EPOLLOUT` constantly, epoll would wake us up in an infinite busy-loop saying "You can write! You can write!" even when we have nothing to send.
So, we only modify the events to include `EPOLLOUT` when we have finished parsing a request and have successfully buffered a response to send back!

---

## 15. Request Parsing State Machine

This is our parser loop that runs a state machine to read the HTTP request.

```cpp
void Http::processingRequestBuffer()
{
	try
	{
		while (true)
		{
			size_t	currIndex = 0;
			size_t	reqBuffSize = _requestBuffer.size();

			parsingHttpRequestLine(currIndex, reqBuffSize);
			parsingHttpHeader(currIndex, reqBuffSize);

			validateRequestData(httpRequest);
			readingRequestBody(httpRequest);

			processingRequest(httpRequest);

			if (httpRequest.getProcessStatus() == NO_STATUS && _keepConnection == true)
				continue ;
			else
				break ;
		}
	}
	catch (std::exception &e)
	{
		Logger::log(LC_ERROR, "something weird, Consider as server error ::%s", e.what());
		generate4xx5xxErrorReponse(httpRequest, 500, false, e.what());
	}
}
```

### What are the states of request parsing?
To handle fragmented data, we store our current parsing state in `e_http_process_status`:
1.  `READING_REQUEST_LINE`: Reading the first line (e.g. `GET /index.html HTTP/1.1`).
2.  `READING_HEADER`: Reading the subsequent header lines (e.g. `Content-Length: 42`).
3.  `VALIDATING_REQUEST`: Matching routes, checking permissions, and preparing buffers.
4.  `READ_BODY`: Reading the body (like a POST upload).
5.  `FINISHED_READ_BODY`: Fully completed.

---

## 16. Parsing Chunked Transfer Encoding

If a request has `Transfer-Encoding: chunked`, the body is split into chunks formatted as:
`[size in hex]\r\n[data]\r\n` and ends with `0\r\n\r\n`.

### How we parse this in C++:
We look for the first CRLF (`\r\n`) in our buffer to isolate the size string:
```cpp
endLinePos = _requestBuffer.find("\r\n", _requestBufferOffset);
std::string hexString = _requestBuffer.substr(_requestBufferOffset, endLinePos - _requestBufferOffset);
```
We parse the hex size using our utility `hex_to_size_t`:
```cpp
size_t hexNum = 0;
hex_to_size_t(hexString, hexNum);
```
If the size is `0`, we are finished! Otherwise, we know exactly how many bytes to read for this chunk. We write them to our request body file, skip the trailing `\r\n`, and repeat.

---

## 17. Writing to Client: `Http::writeToClient()`

Once we have a response ready, we write it to the client non-blockingly.

```cpp
void	Http::writeToClient()
{
	HttpResponse *response_block = NULL;

	_isEpollout = true;

	if (_httpResponseList.empty() || _httpResponseList.front().isComplete())
	{
		Logger::log(LC_DEBUG, "WHAAAAAAT");
		return ;
	}
	response_block = &_httpResponseList.front();

	ssize_t	sendReturn = response_block->sendHttpResponse(*_clientSocketPtr);

	if (sendReturn < 0)
	{
		_keepConnection = false;
		return ;
	}

	// remove this reponse from list if complete
	if (response_block->isComplete())
	{
		_keepConnection = response_block->keepAfterResponse;
		_httpResponseList.pop_front();
	}

	// then if the response list is empty now
	// we close the EPOLLOUT
	if (_httpResponseList.empty() || _httpResponseList.front().hasSomethingtoSend() == false)
	{
		epoll_event	event;
		std::memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = _clientSocketPtr->getSocketFD().getFd();

		if (epoll_ctl(_clientSocketPtr->getEpollFD().getFd(), EPOLL_CTL_MOD, _clientSocketPtr->getSocketFD().getFd(), &event) == -1)
		{
			std::string msg = "Http::writeToClient::epoll_ctl()::error::";
			msg += std::strerror(errno);
			throw WebservException(msg);
		}
		_isEpollout = false;
	}
}
```

### Why do we remove `EPOLLOUT` dynamically?
If the response queue becomes empty, we modify the socket events to remove `EPOLLOUT` (leaving only `EPOLLIN`). If we didn't do this, epoll would constantly wake us up saying the socket is writable, causing our server to spin in a 100% CPU busy loop! We only turn on `EPOLLOUT` when we have response bytes ready to send.

---

## 18. Fork & Pipe Setup

When a client requests a CGI script, we cannot run it in our main process because it will block the server. Instead, we fork a child process and set up two pipes.

```cpp
	int pipe_in[2];
	int pipe_out[2];

	if (requestData.bodyData.readingRequestBodyPtr)
		pipe(pipe_in);
	pipe(pipe_out);

	pid_t pid = fork();

	if (pid < 0)
	{
		// ... handle fork error
	}
	else if (pid == 0) // Child Process
	{
		try
		{
			TempFileManager().setIsChild(true);
		
			dup2(pipe_out[1], STDOUT_FILENO);
			if (requestData.bodyData.readingRequestBodyPtr)
				dup2(pipe_in[0], STDIN_FILENO);
		
			for (int i = 3; i < MAX_FD; i++)
				close(i);
		
			{
				/* prepare before execve() */
				// envData().assignEnv("REQUEST_METHOD", method)...
				// ...
				
				char* argv[3];
				argv[2] = NULL;
				argv[0] = const_cast<char *>(requestData.cgiData.cgiPath.c_str());
				argv[1] = const_cast<char *>(requestData.targetData.combinedPath.c_str());

				signal(SIGINT, SIG_DFL);
			    signal(SIGQUIT, SIG_DFL);
			    signal(SIGTERM, SIG_DFL);
			    signal(SIGPIPE, SIG_DFL);

				execve(argv[0], argv, envData().getEnvp());
				exit(1);
			}
		}
		// ...
	}
```
*   `fork()`: Clones our server process. The child process has `pid == 0`.
*   `dup2()`: Redirects standard input and standard output to our pipe ends.
*   `execve()`: Replaces the child process memory with the script (e.g. Python interpreter).
*   In the parent, we close the child's ends of the pipes, set our ends (`pipe_in[1]` and `pipe_out[0]`) to `O_NONBLOCK`, and register them in epoll:
    *   `pipe_in[1]` (CGI input) is registered with `EPOLLOUT` to write client request body.
    *   `pipe_out[0]` (CGI output) is registered with `EPOLLIN` to read script response.

---

## 19. Non-blocking CGI I/O

When epoll detects activity on these pipes, `handleCGISocketEvent` is called, which calls `processCGI()`:
*   `sendToCGI()`: If the pipe is writable (`EPOLLOUT`), we read chunks of the client's uploaded body from the temporary file and write them to `pipe_in[1]`. When done, we close the pipe so the CGI script knows we are finished sending input.
*   `readFromCGI()`: If the pipe has data to read (`EPOLLIN`), we read the CGI script's output, parse the CGI headers (like `Status:` and `Content-Type:`), and buffer the body into the client's response queue.

---

## 20. Process Tracking: `CgiProcess::waitProcess()`

If a child process hangs or crashes, we don't want our server to hang. We use `waitpid` with a special flag.

```cpp
OptionalData<int> CgiProcess::waitProcess()
{
	OptionalData<int> returnValue;
	if (status == CGI_PROCESS_WAITING)
	{
		int ret;
		int waitstatus = 0;
		while (true)
		{
			ret = waitpid(cgiPid, &waitstatus, WNOHANG);

			if (ret < 0){
				if (errno == EINTR) continue;
				break ;
			}
			else if (ret > 0){
				returnValue = waitstatus;
				status = CGI_PROCESS_FINISHED;
				break ;
			}
			else
				break ; // ret == 0 means process is still running!
		}
	}
	return (returnValue);
}
```

### Why do we use `WNOHANG`?
Normally, `waitpid(pid, &status, 0)` blocks our program until the child process finishes. If a script runs an infinite loop, our server freezes forever!
Adding the `WNOHANG` flag tells the OS: "Check if the child has finished. If it has, return its status. If it is still running, return `0` immediately. Do not make me wait."
This lets our server check on the child process' status during our regular timeout checks, without ever blocking the main event loop.

---

## 21. ConfigData & ServerConfig

Before our server can start, it needs to know what ports to listen on and where to look for files.

### 1. `ConfigData`
This class reads and parses the `.conf` configuration file:
*   It handles syntax checks (missing semicolons, unmatched curly braces `{}`).
*   It groups configuration data by server blocks, mapping each host and port configuration to a list of server rules.

### 2. `ServerConfig`
Stores the rules for a specific server instance, such as:
*   Host IP and Listening Port.
*   `server_name` (e.g., `example.com`).
*   `client_max_body_size` (limits file uploads).
*   `error_page` configurations.
*   `location` blocks containing routes, allowed HTTP methods, root directories, file uploads directories, autoindex flag, and redirects.

---

## 22. OptionalData

Because the 42 subject requires our code to compile under the C++98 standard, we do not have access to C++17's `std::optional`. To avoid using sentinel values (like returning `-1` or `NULL` which can cause crashes), i created the `OptionalData` template class. It simply wraps a value and a boolean `_hasData` flag, letting us safely check if a variable has a valid state:

```cpp
template <typename T>
class OptionalData {
private:
    T _data;
    bool _hasData;
public:
    OptionalData() : _hasData(false) {}
    OptionalData(T data) : _data(data), _hasData(true) {}
    bool hasData() const { return _hasData; }
    T& operator*() { return _data; }
    // ...
};
```
*   `_hasData`: A flag tracking whether the object holds a valid value.
*   If we return `OptionalData<int>`, the calling code checks `hasData() == true` before reading, avoiding undefined values!

---

## 23. TempFileManager

When receiving file uploads, storing everything in RAM will cause the server to crash if the file is large. `TempFileManager` streams the client request body chunks directly to temporary files on the disk. Once the request is processed or closed, it automatically deletes the temporary files.

---

## 24. Why std::ifstream instead of raw open()?

You'll see `std::ifstream` used throughout my project to read files (like the config file or static HTML files). I chose `std::ifstream` because of **RAII** (Resource Acquisition Is Initialization). If an error happens or an exception is thrown while reading a file, the `std::ifstream` destructor automatically runs and closes the file descriptor. If we used raw `open()`, we would have to manually call `close()` in every single error block, which is super error-prone and leads to **file descriptor leaks**. Plus, `std::ifstream` integrates beautifully with `std::getline()` which makes line-by-line parsing incredibly simple!

---

## 25. Why CharTable instead of string search?

HTTP headers and URIs are made of specific allowed characters (e.g. alphanumeric, certain symbols). To validate them quickly, i created the `CharTable` class. Instead of doing slow searches like `std::string::find` or regex which take O(N) time, `CharTable` allocates a 256-boolean array where each index corresponds to an ASCII character. Checking if a character is allowed is a simple O(1) array lookup: `table[c]`. This is extremely fast and prevents buffer injections!

---

## 26. String/IP Helper Utilities

*   `split`: Breaks strings by a delimiter (e.g. parsing config paths).
*   `ft_trim`: Strips leading and trailing whitespaces.
*   `tolower`: Converts strings to lowercase.
    *   **Why?** HTTP standards require HTTP header names to be case-insensitive (e.g. `Content-Length` is the same as `content-length`). We convert all headers to lowercase before inserting them into our maps for reliable lookup.
*   `ip_conversion`: Converts strings like `"127.0.0.1"` into the integer network byte order required by socket structures.

---

## Summary of the WebServ Request Lifecycle

Here is the exact path a request takes through our server:

1.  **Listen**: `WebServ`'s epoll loop is running. A client connects to port eg;`9081`.
2.  **Accept**: Epoll triggers `EPOLLIN` on the `SERVER_SOCKET`. We accept the connection, set the new client socket to non-blocking, and register it to epoll.
3.  **Read Request**: The client sends HTTP request data. Epoll triggers `EPOLLIN` on the `CLIENT_SOCKET`. The `Http` state machine reads and parses the request.
    *   *If the request has a body*: We write it to a temporary file on the disk via `TempFileManager`.
4.  **Route & Process**: We match the URL to our configuration.
    *   *Static File*: We read the file and buffer it.
    *   *CGI Script*: We fork a child process, create pipes, redirect stdin/stdout, and run the script. We feed the body from the temp file to CGI stdin, and read output from CGI stdout non-blockingly using epoll events.
5.  **Write Response**: The response is prepared. Epoll triggers `EPOLLOUT` on the `CLIENT_SOCKET`. We write the response data back to the client in chunks.
6.  **Cleanup**: If keep-alive is active, we reset the state machine and wait for the next request. Otherwise, we close the socket and destroy the associated objects, and `FileDescriptor` closes the file descriptors automatically.
---