*This project has been created as part of the 42 cirriculum by srussame*

# Description
This project is about building a HTTP server in C++ and using only standard libraries from C and C++ only. This project aiming is to understand one of the most crucial procotol that underlies in our daily internet usage. This project will want you to have clear understanding of HTTP into basically byte by byte how it works, I/O multiplexing, sockets, connections, file managing are also the required knownledge to do this project.

this one in particular based on RFC9110, which is the new version of HTTP/1.1 but only implement fer sections of the standards ad the subject states.

This project is a part of 42 Coding School's Common Core curriculum.

#### quick navigations
- [configuration file](#config-file)
- [how to test](#how-to-test)
- [Resources and AI usage](#resources)
- [What is HTTP ?](#what-is-http)
- [What is Epoll?](#what-is-epoll-)
- [how my webserver works?](#how-my-webserver-works-)
- [Importance Notes](#important-notes) ## will be use to defense during evaluation
- [code overview](./CodeDetails.md)

# Instructions

This part will teach you how to start on this project.

## Requirements
- this project only works in **Linux** operating system (tested only Ubuntu os)
- prerequisite packages that you need to have to run this project you can simply run

```bash
sudo apt update;sudo apt install build-essential make php-cgi python3
```
*(there might be some missing packages here sorry in advance)*

## Makefile

`make` - to compile the project result in and executable file named **webserv**

`make clean` - use to remove compiled object files and temporary files

`make fclean` - remove objects and tempfiles, and also remove the executable and temporary directory

`make re` - remove all objects and executable, and recompile the project again

## Start the program
must have **webserv** executable in the root directory
```bash
./webserv <config_file.conf>
```

## Config File

the program requires `.conf` so we need a little bit of explanation on how to use the `.conf` file to configure the web server

Let's take a look at the example to see the structure of the config file

```conf
server {
	listen 4343;
	root tester42/YoupiBanane;
	host 127.0.0.1;
	client_max_body_size 0;
	cgi_pass .bla tester42/cgi_tester POST;
	location / {
		autoindex on;
		allowed_methods GET;
	}

	location /post_body
	{
		client_max_body_size 100;
		allowed_methods POST;
		upload_store tester42/YoupiBanane;
	}

	location /directory/ {
		allowed_methods GET POST;
		index youpi.bad_extension;
		root tester42/YoupiBanane;
	}
}
```

this webserver can be host as multiple http servers and each server block will write as
`server { <server configuration> }`

Each server block will have smaller `location` block inside. Each server must have at least 1 location block which is `location /`

- `#` - use for commenting
- `;` - each directive and values must end with semicolon

## Directives Type

| Directive Type     | Nginx Syntax                                | Apache Syntax                  | What It Does |
|--------------------|---------------------------------------------|--------------------------------|--------------|
| **Domain Name**    | `server_name localhost test;`               | `ServerName localhost`<br>`ServerAlias test` | Defines which domain names the server responds to |
| **Port**           | `listen 9081;`                              | `Listen 9081`<br>`<VirtualHost *:9081>` | Specifies the port the server listens on |
| **Root Directory** | `root wwwroot/www2;`                        | `DocumentRoot /path/to/wwwroot/www2` | Sets the main folder containing website files |
| **Default File**   | `index index.html;`                         | `DirectoryIndex index.html`     | Defines the first file served (e.g., homepage) |
| **Error Pages**    | `error_page 404 wwwroot/www2/404.html;`<br>`error_page 500 wwwroot/www/error400.html;` | `ErrorDocument 404 /wwwroot/www2/404.html`<br>`ErrorDocument 500 /wwwroot/www/error400.html` | Specifies custom files to display when errors occur |
| **Upload Store**   | `upload_store wwwroot/www2/uploads;`        | *(No direct equivalent — requires modules like mod_dav or custom scripts)* | Directory where uploaded files are stored |
| **Allowed Methods**| `allowed_methods GET POST DELETE;`          | `<Limit GET POST DELETE> ... </Limit>` | Restricts which HTTP methods are permitted |
| **Autoindex**      | `autoindex on;`                             | `Options +Indexes`              | Enables directory listing for browsing files |
| **CGI Pass**       | `cgi_pass .php /usr/bin/php-cgi;`<br>`cgi_pass .py /usr/bin/python3;` | `ScriptAlias /cgi-bin/ /path/to/cgi-bin`<br>`AddHandler cgi-script .php .py` | Defines interpreters for PHP and Python scripts |
| **Redirects**      | `return 302 /cgi-bin/;`<br>`return 302 /session/;`<br>`return 302 /;` | `Redirect 302 /cgi-bin/ /cgi-bin/`<br>`Redirect 302 /session/ /session/`<br>`Redirect 302 / /` | Redirects requests to another path |

---

## Usage
```console
> ./webserv configs/default.conf
[2026-06-15 18:20:56] Server <localhost, test> is listening on port 9081 (fd 4)
[2026-06-15 18:20:56] Webserv is Waiting for connection!

```
this indicates that the server is now running and we can access it through specified port.

## Closing webserv
To close webserv can simple `CTRL + C` to exit the program cleanly
```console
^C
Signal => SIGINT Received. Terminating program....
[2026-06-15 18:24:13] epoll_wait() got interrupted.
[2026-06-15 18:24:13] Closing the Webserv
> 
```

## How to test
### **Basic test**
#### GET 
Test GET method
		```bash
		curl localhost:4343/index.html
		```
#### POST
Test POST method
		```bash
		curl -X POST -H "Content-Type: plain/text" --data "this is new post" localhost:4343/upload/test.txt
		```

#### DELETE
Test DELETE method
		```bash
		curl -X DELETE localhost:4343/upload/uploads/test.txt
		```

#### UNKNOWN
Test UNKNOWN method that not implemented
		```bash
		curl -X UNKNOWN localhost:4343
		```
### Test with CGI
	open your browser to http://localhost:4343

---

there are two custom testers built to test this project easily

> *explicitly state that these tests are not mandatory requirements for this project so they were generated by AI (Gemini Flash 3.5)*

### `tester.sh` - this one checks the status code and headers received from the webserver
### `configtester.sh` - this one checks this configuration file parsing




# Resources

### Refereced Sites
- [RFC9110](https://www.rfc-editor.org/rfc/rfc9110.html) - basic understanding and semantics of HTTP/1.1
- [RFC9112](https://www.rfc-editor.org/rfc/rfc9112.html) - all HTTP/1.1 standards
- [RFC3875](https://www.rfc-editor.org/rfc/rfc3875.html) - CGI - Common Gateway Interface standard
- [cgi REQUEST_UTI env](https://www.php.net/manual/en/reserved.variables.server.php) - what is REQUEST_URI and why it is kind of a standard in realword CGI though this not found in CGI standard at all
- [REDIRECT_STATUS env](https://stackoverflow.com/questions/24378472/what-is-php-serverredirect-status) - This env is required more php-cgi

### How AI was used in this project
models that were used in this project are
- Gemini 3.1 Pro
- Gemini 3.5 Flash
- NotebookLM by google

**NotebookLM** is the ai that allows you the insert all various documentation into it and when asked it will search through those documentations and quickly tell the part that we want to look for. <u>this was mainly used for finding information on those **RFC**s</u> because it is a lot and takes a lot of time just to find one thing if not assisted by AI.

**Gemini Flash 3.5** were <u>used to generate tester scripts and mostly all the test cases.</u> Assuming that tests are not mandatory and not being mentioned in the subject So i think this is valid. Tester helps me easily diagnose the problem when i change something in the code.

**Gemini Pro 3.1** were used <u>generating CodeDetails.md file</u>, which contains the code overview of my project. though the file was check the correctness throughoutly.

> Other than this i consider it as the daily usage of AI because ai generated contents are already in everywhere, now as of 2026, searching in Google also use AI. does that count?

# What is server?

> definition from [wikipedia](https://en.wikipedia.org/wiki/Server_(computing))

A **server** is a software system that provides data, resources, or services to other computers called "clients". 

So, that means we need to create a program that serves the clients' requests. You fellow 42 Cadets might familiar from the project like minitalk about send and recieve message from other processes, read and write through pipes, byte by byte data checking and manipulation, this project's backbone is also about these, but made easier with those **epoll()** functions to handle I/O events

# What is HTTP?

As I said that in the layer beneaths server and client, It is just processes try to communicate with each other. whether it is the process in the same local machine or from another machine. when we look into raw data in all those connections over the internet. they are just sending bytes data to each other. And after a while when more and more people using internet to make connections someone came up and just said that we might need some standards or **protocols** to be able to communicate with each other easily

> this is from [developer.mozila.org](https://developer.mozilla.org/en-US/docs/Web/HTTP)

**HTTP** is **Hypertext Transfer Protocol**.
It is an <u>application layer</u> protocol for transmitting hypermedia documents, such as **HTML**. It was designed for ***communication between web browsers and web servers***. So It's just the standard way to read the data from the client and know what the client wants.

# what is I/O multiplexing ?
> this information is from [dev.io](https://dev.to/min38/io-multiplexing-3lho)

### Blocking I/O
blocking I/O is a method where the program stops and waits until and I/O operation completes. When the read() or write() is called, the program waits until data arrives. this mean is no data comes, it just keeps waiting indefinitely. In a single-threaded environment, if one socket is blocked, other sockets cannot be processed.

### what multiplexing solve?
I/O Multiplexing is a technique where a single process manages multiple file descriptoes simultaneously. The program monitors file descriptors to determine what type of I/O events have occured and whether each file descripter is in a ready state. Methods for implementing I/O Multiplexing include `select`, `poll`, and `epoll`.

# What is **EPOLL** ??
> this information is from [man7.org](https://www.man7.org/linux/man-pages/man7/epoll.7.html)

`epoll` is used for monitoring multiple file descriptors to see if I/O is possible on any of them. The **epoll** API can be used either as an edge-triggered or level-triggered interface and scales well to large numbes of watched file descriptors.

Perhaps the most important question that might get asked during the evaluation. **Epoll** is an upgraded version of **poll**. It performs a similar task - to monitor multiple file descriptos to see if I/O operations is possible on any of them 

### Why choose EPOLL ?

people said that epoll() is better but i want to know why and this is what i found

### The C10K problem
> this information is from [kegel.com](https://www.kegel.com/c10k.html)

I think around 1990s to early 2000s, the traditional way to handle multiple users was One-Thread-Per-Client. Meaning that the computer needs to spawn new threads everytime new client connects. Some said it almost impossible in that era to handle 10K concurrent connections.

The breakthrough the solved this was to be using event driven and non-blocking I/O. Instead of waiting for slow users, a single thread can menages thousands of connections by asking the operating system to send alert only when any connection actually have something to read

that means `poll` and `select` are worse than `epoll` and `kqueue` because this new one works on event driven and have the time complexity of $O(1)$

# How my webserver works ?

First, my server starts by reading configuration file and parse it into a data structure. then setup the server and run the main loop.

my main server loop will detect the fd events using epoll_wait, or custom event that can also triggered by socket. then each socket that have event will handle itself based on its socket type. these client sockets and CGI sockets will be remove if finished, failed, or timeout. The loop will continue forever until it receives the specified signals.

The most worked class in this project is HTTP class, it handles the buffer parsing, protocols, and fork CGI process inside that one class, it is a lot but very easy to follow.

the utility classes that made this project easier are
- `EnvpWrapper` - which all about easy access and modification to environment data
- `Logger` - which display the log of the server with timestamp made it easy to track.
- `FileDescriptor` - the wrapper of fd with reference counting so we can use fd with ease
- `CgiProcess` - pid wrapper class that makes sure all the process will correctly quit and no zombie process at all.
- `TempFileManager` - this project requires the server to store the data into temporary file first, this class manages that
- `WebservException` - throw exception class
- `Shared` - template class that shares ownership and only call destructor once the reference count reach zero.
- `OptionalData` - same concept as std::optional that if we may want some variable to only allocate when we actually need it. also a good workaround with FileDesctiptor as well.
- `CharTable` - quick lookup table for checking char and some utility methods to work around

`defined_value.hpp` - stores the constant values that the project uses

`webserv_structs.hpp` - all important structs of this project

`utility_function.hpp` - helper functions that were used all in this project.

# Important Notes

This part can be use during evaluation to defense this project. As the subject stated that
> Checking the value of **errno** to adjust the server behaviour is <u>strictly forbidden</u> after performing a read or write operation.

and in my project there are sections that check errno here

1: from `Http::readingContentLengthBody` (line 3533-3556 in Http.cpp) and `Http::readingTransferEncodingChunk` (line 3959-3982 in Http.cpp)
```cpp
ssize_t	writeAmount = write(requestData.bodyData.writeBodyFile->getFd(), &_requestBuffer[_requestBufferOffset], readBodyAmount);
if (writeAmount < 0)
{
	if (errno == EINTR)
		continue;
	else
	{
		// try to correctly remove file if write operation is failed 
		requestData.bodyData.writeBodyFile.clear();
		if (requestData.bodyData.isUseTempFile)
			tempFileManager().removeTempFile(requestData.bodyData.tempRequestBodyFileNum);
		else
			std::remove(requestData.targetData.combinedPath.c_str());
	
		generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::write() operation failed during reading the request body::" + std::string(std::strerror(errno)));
	}
}
else
{
	// _requestBuffer.erase(0, writeAmount);
	_requestBufferOffset += writeAmount;
	requestData.bodyData.curr_body_read += writeAmount;
	break ;
}
```

2: from `Http::multiformDataReadBody` (line 5589-5603 Http.cpp)
```cpp
while (true)
{
	writeAmount = write(targetFileFd->getFd(), writeBuffer.data(), writeBuffer.size());
	if (writeAmount < 0)
	{
		if (errno == EINTR)
			continue ;
		else
		{
			/* fatal error */
			generate4xx5xxErrorReponse(requestData, 500, false, "Internal Error::multipart/form-data::write() failed::" + std::string(std::strerror(errno)));
		}
	}
	break ;
}
```

3: from `HttpResponse::sendHttpResponse` (line 253-270 HttpResponse.cpp)
```cpp
if ((*fileBody)->fail())
{
	/* if eof() then just normal end file */
	if ((*fileBody)->eof())
	{
		isReachEOF = true;
	}
	else if ((*fileBody)->bad() || errno != EINTR)
	{
		/* here would mean fatal error */
		return (-1);
	}
	else
	{
		(*fileBody)->clear();
		continue;
	}
}
```

4: from `HttpResponse::forcePrintAllResponse` (line 416-442 HttpResponse.cpp)
```cpp
if ((*fileBody)->fail() == true)
{
	/* if it failed because reach the eof then it is clean*/
	if ((*fileBody)->eof())
		break ;
	
	/* bad() indicate the fatal error */
	if ((*fileBody)->bad())
	{
		keepAfterResponse = false;
		break ;
	}

	/* then check errno here */
	if (errno == EINTR)
	{
		/* EINTR we can allow it to work on */
		(*fileBody)->clear();
		continue ;
	}
	else
	{
		/* else here are all unhandled errors */
		keepAfterResponse = false;
		break ;
	}
}
```

These are all **errno** checking after read/write operations, but the subject also states that
> I/O that can wait for data (sockets, pipes/FIFOs, etc.) must be
non-blocking and driven by a single poll() (or equivalent). Calling
read/recv or write/send on these descriptors without prior readiness
will result in a grade of 0. <u>**Regular disk files are exempt.**</u>

as all the checking above are regular disk files that doesn't need to be non-block. So my defense is these are allowed in this project.


### How to start?

For me with zero background in engineering and programming. this project is **incredibly overwhelming** for me and it was really a struggle for me to even know how to start and i will guide you here. First try to atleast establish a **poll()** or **epoll()** or equivalent main loop first before anything else, bind with port, listen and accept connections. These are the core functions and concepts that you need to grasp first before even know what the project is even gonna look like.

This documentation i will try to explain each sections by starting from the main() loop and explain the inside steps by steps and layer by layer as much as possible
[code details here](./CodeDetails.md)
