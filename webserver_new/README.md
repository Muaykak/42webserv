*This project has been created as part of the 42 cirriculum by srussame, paradari*

# Description
- Webserv is HTTP web server project written in C++98, compliant with HTTP/1.1 standards. Must be non-blocking I/O architecture capable of handling multiple clients simultaneously. The server processes client requests by reading and evaluating data against an configuration file.
# Instructions

# How to run
 clone repository to your machine


## Makefile
 make 
 compile the project

 make ```clean```
 remove object file

 make ```fclean```
 remove object file and executable

 make ```re```
 recompile the project

 make ```test```
 run tester
 
## Usage
 - after compile will get executable name webserv
 - to start server
	```bash ./webserv [config_file.conf]```

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

# HTTP Method
## GET 
	- retrieve a specific data from a server

## POST
	- send data to a server to create a new resource

## DELETE
	- Removes target resource given by a URI