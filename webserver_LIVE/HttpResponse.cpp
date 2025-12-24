/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nusamank <nusamank@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/13 12:56:59 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/25 13:22:12 by nusamank         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "HttpResponse.hpp"
#include "Webserv.hpp"


HttpResponse::HttpResponse():status(200)
{
	setHeader("Content-Type", "text/html");
}
HttpResponse::HttpResponse(HttpResponse const &other)
{
	if (this != &other)
		*this = other;
}
HttpResponse &HttpResponse::operator=(HttpResponse const &other)
{
	if (this != &other)
		*this = other;
	return (*this);
}
HttpResponse::~HttpResponse()
{
//	Logger::log(LC_RED, "HttpResponse instance is being destroyed");
}

std::string HttpResponse::getHeader(std::string name) const
{
	std::map<std::string, std::string>::const_iterator pos = headers.find(name);
	if (pos != headers.end())
		return pos->second;
	else
		return "";
}

bool HttpResponse::setHeader(std::string name, std::string value , bool overwriteExisting)
{
	std::map<std::string, std::string>::const_iterator pos = headers.find(name);
	// if (pos != headers.end() && !overwriteExisting)
	// 	return false;
	// else 
	if (pos != headers.end() && !overwriteExisting)
	{
		while (pos != headers.end())
		{
			name += " ";
			pos = headers.find(name);

		}
		headers[name] = value;
		return true;
	}
	else
	{
		headers[name] = value;
		return true;
	}
}

std::map<std::string, std::string> 	&HttpResponse::getHeaders(void)
{
	return headers;
}


int	HttpResponse::getStatus()
{
	return status;
}

bool	HttpResponse::setStatus(int newStatusCode)
{
	if (newStatusCode < 100 || newStatusCode > 599)
		return false;
	status = newStatusCode;
	return true;
}

std::string	HttpResponse::getBody() const
{
	return body;
}


bool	HttpResponse::setBody(std::string newBody)
{
 	body = newBody; 
	return true;
}

std::string HttpResponse::getStatusText(int statusCode)
{
	switch (statusCode)
	{
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 202:
			return "Accepted";
		case 203:
			return "Non-Authoritative Information";
		case 204:
			return "No Content";
		case 301:
			return "Moved Permanently";
		case 302:
			return "Found";
		case 304:
			return "Not Modified";
		case 400:
			return "Bad Request";
		case 401:
			return "Unauthorized";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 408:
			return "Request Timeout";
		case 411:
			return "Length required";	
		case 413:
			return "Payload Too Large";
		case 415:
			return "Unsupported Media Type";
		
		case 500:
			return "Internal Server Error";
		case 503:
			return "Service Unavailable";

		default:
			return "Unknown Status";
	}
}

std::string HttpResponse::getErrorPage(int statusCode, ServerConfig server)
{
	std::map<int, std::string> errorPages = server.getErrorPages();
	std::map<int, std::string>::iterator it = errorPages.find(statusCode);
	if (it != errorPages.end())
	{
		std::ifstream file(it->second.c_str());
		if (!file.is_open())
			return getDefaultErrorPage(statusCode);
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string errorPage = buffer.str();
		return errorPage;
	}
	return getDefaultErrorPage(statusCode);
}

std::string	HttpResponse::getDefaultErrorPage(int statusCode)
{
	std::string errorText = getStatusText(statusCode);
	
	std::ifstream file("errors/errorPage.html");
	if (!file.is_open())

        return "<html><body><h1>Error " + Util::toString(statusCode) + "</h1><p>" + errorText + "</p></body></html>";
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string errorPage = buffer.str();

	std::map<std::string, std::string> replacements;
	replacements["{{statusCode}}"] = Util::toString(statusCode);
    replacements["{{errorText}}"] = errorText;

	std::map<std::string, std::string>::iterator it;
	for (it = replacements.begin(); it != replacements.end(); ++it) {
        size_t pos = 0;
        while ((pos = errorPage.find(it->first, pos)) != std::string::npos) {
            errorPage.replace(pos, it->first.length(), it->second);
            pos += it->second.length();
        }
    }
    return errorPage;
}

std::string HttpResponse::serialize()
{
	std::ostringstream  	oss;
	oss << "HTTP/1.1 " << status << " " << getStatusText(status) << "\r\n";
	
	setHeader("Server", WEBS_APP_NAME, true);
	setHeader("Content-Length", Util::toString(body.size() ) , true);

	for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		oss << Util::trim(it->first) << ": " << it->second << "\r\n";
	// single set of \r\n since the header already sent the first set
	oss << "\r\n" << body ;

	std::string color = LC_RES_OK_LOG;
	if(status >= 300)
		color = LC_RES_FND_LOG;
	if(status >= 400)
		color = LC_RES_NOK_LOG;
	if(status >= 500)
		color = LC_RES_INT_LOG;
	Logger::log(color, "[RESPOND]\t%d - %s " , status, getStatusText(status).c_str() );

	if (WEBS_DEBUG_RESPONSE)
	{
		std::cout << LC_DEBUG << "===================================" << std::endl
		<< "response, size = " << oss.str().length() << std::endl
		<< "===================================" << LC_RESET << std::endl
		<< oss.str() << std::endl
		<< "===================================" << std::endl;
	}

	return oss.str();
	
}

std::string HttpResponse::getMimeType(const std::string & extension)
{
	std::map<std::string, std::string> mimeTypes;
	mimeTypes[".html"] = "text/html";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".css"] = "text/css";
    mimeTypes[".js"] = "application/javascript";
    mimeTypes[".json"] = "application/json";
    mimeTypes[".png"] = "image/png";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".ico"] = "image/x-icon";
    mimeTypes[".xml"] = "application/xml";
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".txt"] = "text/plain";
	std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
	if (it != mimeTypes.end())
		return it->second;
	return "application/octet-stream";
}

bool	HttpResponse::getStaticFile(std::string const &filePath )
{
	Logger::log(LC_MINOR_NOTE, " in getStaticFile() , filePath = " , filePath.c_str());

	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open())
	{		
		struct stat fileStat;

		Logger::log(LC_MINOR_NOTE, " getStaticFile ==> filePath = %s" , filePath.c_str());
		if (stat(filePath.c_str(), &fileStat) != 0)
		{
			if (errno == ENOENT)
				throw RequestException(404, "File not found");
			else if (errno == EACCES)
				throw RequestException(403, "Forbidden");
			else
				throw RequestException(405, "Method not allowed");
		}
		return false;
	}
	setStatus(200);

	size_t dotPos = filePath.find_last_of(".");
	std::string extension;
	if (dotPos != std::string::npos)
		extension = filePath.substr(dotPos);
	else
		extension = "";
	
	setHeader("Content-Type", getMimeType(extension), true);
	
	std::stringstream buffer;
	buffer << file.rdbuf();
	setBody(buffer.str());
	return true; 
}



void HttpResponse::debug() const
 {
		std::cout << "========================\n HttpResponse object:\n========================" << std::endl;
        std::cout << " - Status: \t" << std::endl;
        std::cout << " - Body Length: \t" << body.length() << std::endl;
        std::cout << " - Header: \t" << std::endl;
        for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
            std::cout << "\t - " << it->first << " -> " << it->second << std::endl;
        }
		std::cout << std::endl;
}

bool HttpResponse::generateDirectoryListing(const HttpRequest& request, const std::string& path)
{
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		Logger::log(LC_DEBUG, " Getting Here???");
		std::cerr << "Error: " << strerror(errno) << std::endl;
		setStatus(403);
		return false;
	}
	std::string relativePath = request.getPath();
	if (relativePath.empty() || relativePath[relativePath.size() - 1] != '/')
    {
        relativePath += '/';
    }

	std::ostringstream html;
	html << "<!DOCTYPE html>" << std::endl;
	html << "<html><head><title>Directory Listing</title>" << std::endl;
    html << "<style>" << std::endl;
    html << "body { font-family: Arial, sans-serif; background-color: #121212; color: #f5f5f5; margin: 0; padding: 0; }" << std::endl;
    html << ".container { max-width: 800px; margin: 2rem auto; padding: 1rem; background-color: #1e1e1e; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }" << std::endl;
    html << "h1 { font-size: 2rem; text-align: center; margin-bottom: 1rem; color: #f5f5f5; }" << std::endl;
    html << "ul { list-style: none; padding: 0; }" << std::endl;
    html << "ul li { padding: 0.5rem 1rem; border-bottom: 1px solid #333; transition: background-color 0.3s ease; }" << std::endl;
    html << "ul li:last-child { border-bottom: none; }" << std::endl;
    html << "ul li a { text-decoration: none; color: #4dabf7; font-weight: bold; transition: color 0.3s ease; }" << std::endl;
    html << "ul li a:hover { color: #82caff; }" << std::endl;
    html << "ul li:hover { background-color: #2a2a2a; }" << std::endl;
    html << "</style>" << std::endl;
    html << "</head><body>" << std::endl;
    html << "<div class=\"container\">" << std::endl;
	html << "<h1>Directory Listing for " << relativePath << "</h1>" << std::endl;
	html << "<ul>" << std::endl;

	html << "<li><a href=\"..\">.. (UP)</a></li>" << std::endl;
	
	struct dirent* entry;
	rewinddir(dir);
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name != "." && name != "..")
		{
			std::string fullPath = path + "/" + name;
			struct stat s;
			if (stat(fullPath.c_str(), &s) == 0)
			{
				if (s.st_mode & S_IFDIR)
				{
					html << "<li><a href=\"" << relativePath << name << "/\">&#128193; " << name << "</a></li>" << std::endl;
				}
			}
		}
	}
	rewinddir(dir);
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name != "." && name != "..")
		{
			std::string fullPath = path + "/" + name;
			struct stat s;
			if (stat(fullPath.c_str(), &s) == 0)
			{
				if (!(s.st_mode & S_IFDIR))
				{
					html << "<li><a href=\"" << relativePath << name << "\">&#128196; " << name << "</a></li>" << std::endl;
				}
			}
		}
	}

	html << "</ul>" << std::endl;
	html << "</div></body></html>" << std::endl;

	closedir(dir);

	setStatus(200);
	setBody(html.str());
	return true;
}

void HttpResponse::processCGI(std::string command, std::string scriptFile, HttpRequest request, ServerConfig server, RouteConfig route, std::vector<char> &rawBytes)
{

	(void)server;
	(void)route;

	if(!Util::fileExists(scriptFile))
		throw RequestException(404, "File not found");
	if(!Util::fileHasPermission(scriptFile))
		throw RequestException(403, "Forbidden");


	
	char *const argv[] = {
		const_cast<char *>(command.c_str()), 
		const_cast<char *>(scriptFile.c_str())
		, NULL
	};
	std::string method = "REQUEST_METHOD=" + request.getMethod();
	std::string query = "QUERY_STRING=" + request.getRawQueryString();
	std::string contentType = "CONTENT_TYPE=" + request.getHeader("Content-Type");
	std::string contentLength = "CONTENT_LENGTH=" + Util::toString(request.getContentLength());
	std::string fileSize = "HTTP_FILESIZE=" + Util::toString(request.getContentLength());
	std::string scriptFilename = "SCRIPT_FILENAME=" + scriptFile;

	std::string path = "PATH=/usr/local/bin:/usr/bin:/bin";
	std::string serverSoftware = "SERVER_SOFTWARE=" + std::string(WEBS_APP_NAME);
	std::string envCGI = "GATEWAY_INTERFACE=CGI/1.1"; 
	std::string cookie = "HTTP_COOKIE=" + request.getHeader("Cookie");
	std::string accept = "HTTP_ACCEPT=" + request.getHeader("Accept");
	std::string status = "REDIRECT_STATUS=200";
	std::string uploadDir = "UPLOAD_DIR=/tmp";

	char * envp[] = {	
		const_cast<char *>(method.c_str()),
		const_cast<char *>(query.c_str()),
		const_cast<char *>(contentType.c_str()),
		const_cast<char *>(contentLength.c_str()),
		const_cast<char *>(uploadDir.c_str()),
		const_cast<char *>(fileSize.c_str()),
		const_cast<char *>(status.c_str()),
		const_cast<char *>(scriptFilename.c_str()),

		const_cast<char *>(path.c_str()), 
		const_cast<char *>(serverSoftware.c_str()), 
		const_cast<char *>(envCGI.c_str()), 
		const_cast<char *>(cookie.c_str()), 
		const_cast<char *>(accept.c_str()), 


		NULL
	};

	// Create pipes for stdin and stdout
	int pipe_stdin[2];
	int pipe_stdout[2];

	if (pipe(pipe_stdin) == -1 || pipe(pipe_stdout) == -1)
	{
		std::cerr << "Error creating pipes: " << strerror(errno) << std::endl;
		throw RequestException(500,"Internal Server Error");
	}
	pid_t pid = fork();
	if (pid == -1)
	{
		std::cerr << "Error forking process: " << strerror(errno) << std::endl;
		throw RequestException(500,"Internal Server Error");
	}
	if (pid == 0)
	{
		// sleep(10);
		// Child process
		if (dup2(pipe_stdin[0], STDIN_FILENO) == -1)
		{
			std::cerr << "Error redirecting stdin: " << strerror(errno) << std::endl;
			exit(errno) ;
		}
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);

		// Redirect stdout
		// Logger::log(LC_RED, "RESTORE CGI pipeout HERE");
		if (dup2(pipe_stdout[1], STDOUT_FILENO) == -1)
		{
			std::cerr << "Error redirecting stdout: " << strerror(errno) << std::endl;
			exit(errno) ;
		}
		if (dup2(pipe_stdout[1], STDERR_FILENO) == -1)
		{
			std::cerr << "Error redirecting stderr: " << strerror(errno) << std::endl;
			exit(errno) ;
		}
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		if (execve(argv[0], argv, envp) == -1)
		{
			std::cerr << "Error executing script: " << strerror(errno) << std::endl;
			exit(errno) ;
		}
	}
	else
	{
		// Parent process
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);
	
		fcntl(pipe_stdout[0], F_SETFL, O_NONBLOCK);
		int  bytesWritten = write(pipe_stdin[1], rawBytes.data() , rawBytes.size());
		if(bytesWritten < 0)
		{
			close(pipe_stdin[1]);
            close(pipe_stdout[0]);
			throw RequestException(500,"Internal Server Error");
		}
		close(pipe_stdin[1]);

		// Read from the child's stdout
		char buffer[CGI_READ_BUFFER_SIZE];
		std::string  output = "";
		bool timed_out = false;
		int elapsed_time = 0;

		while (true)
        {
            ssize_t bytesRead = read(pipe_stdout[0], buffer, sizeof(buffer) - 1);
			while (bytesRead > 0)
			{
				buffer[bytesRead] = '\0';
				output += std::string(buffer);
				bytesRead = read(pipe_stdout[0], buffer, sizeof(buffer) - 1);
			}

			// Wait for the child process to finish
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result == 0)
			{
				usleep(CGI_SLEEP_MICROSEC);
				elapsed_time += 100;
				if (elapsed_time >= CGI_TIMEOUT_SEC  * 1000)
				{
					// std::cerr << "Error: CGI script timed out" << std::endl;
					kill(pid, SIGKILL);
					timed_out = true;
					break ;
				}
			}
			else if (result == -1)
			{
				// std::cerr << "Error waiting for child process: " << strerror(errno) << std::endl;
				close(pipe_stdout[0]);
				throw RequestException(500,"Internal Server Error");
			}
			else
			{
				if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
				{
					setCGIResponse(output, output.length());
				}
				else
				{
					std::cerr << "Child process exited with error status: " << WEXITSTATUS(status) << std::endl;
					setStatus(500);
				}
				break ;
			}
		}
		close(pipe_stdout[0]);
		if (timed_out)
            throw RequestException(500,"Internal Server Error");
		
	}
}

bool	HttpResponse::isRepeatableHeader(std::string const &str)
{
	if(str.find("Set-Cookie") != std::string::npos)
		return true;
	if(str == "Cookie" || str == "User-Agent")
		return (true);
	return (false);
}


size_t	HttpResponse::setCGIResponse(std::string &output, size_t length)
{

	// char	buffer[5000];
	// memset(buffer,0,5000);

	int sepLength = 4;
	size_t splitPos = output.find("\r\n\r\n");
	if (splitPos == std::string::npos)
	{
		splitPos = output.find("\n\n");
		sepLength = 2; 
		if (splitPos == std::string::npos)
		{
			setBody("");
			setStatus(500);
			return (500);
		}
	}

	
	std::string buffer ;
	
	std::istringstream stream(output.substr(0, splitPos));
	
	std::string		headerName , headerValue ; 
    while (std::getline(stream, buffer))
	{
		std::istringstream token(buffer); 
		if( token >> headerName >> headerValue) 
		{
			headerName = headerName.substr(0 , headerName.length() - 1);
			// std::cout << " _" << headerName << "_ , _" << headerValue << "_ " << std::endl;
			if(headerName == "Status")
			{
				setStatus(Util::toInt(headerValue));
			} else if(isRepeatableHeader(headerName))
				setHeader(headerName, headerValue, false);
			else 
				setHeader(headerName, headerValue, true);
		}
	}
	setBody( output.substr(splitPos + sepLength, length - (splitPos + sepLength)));
	return 200; 	
}

bool HttpResponse::checkFileAvailibity(std::string &filePath, bool isFileOnly)
{
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0)
    {
        if (errno == ENOENT)
            throw RequestException(404, "File not found");
        else if (errno == EACCES)
            throw RequestException(403, "Forbidden");
        else
            throw RequestException(400, "Bad Request");
    }

    if (isFileOnly)
        return S_ISREG(fileStat.st_mode);
    return true;
}

bool	HttpResponse::handleDeleteMethod(std::string &localPath)
{
	Logger::log(LC_MINOR_NOTE, "ABOUT TO DELETE THE FILE " , localPath.c_str());
	if(!checkFileAvailibity(localPath, true))
		throw RequestException(403, "Forbidden");	

	if (remove(localPath.c_str()) != 0)
		throw RequestException(403, "Forbidden");
	
	setStatus(204);
	setBody("");
	return (true);
}

bool	HttpResponse::handleUploadedFiles(Connection *conn, RouteConfig *route , HttpRequest &httpRequest)
{

	Logger::log(LC_MINOR_NOTE, " Inside handleUploadedFiles()");
	(void) httpRequest;
	
	std::string boundary = conn->getBoundary();
	if (boundary.empty())
		throw RequestException(400, "Bad Request");
	std::string content = Util::vectorCharToString(conn->getRawPostBody());
	size_t	fileCount = 0; // indicate number of actual files received via the form
	size_t	success = 0;

	
	std::vector<std::string> tokens = Util::split(content, boundary);
	for(size_t i = 0; i < tokens.size(); ++i)
	{
		std::istringstream		streamLine (tokens[i]);
		std::string				str; 
		std::string				fileName = "";
		while(std::getline(streamLine, str))
		{
			size_t	fileNamePos = str.find("filename=\"");
			if(str.find("Content-Disposition") == 0 &&  fileNamePos != std::string::npos)
			{
				fileNamePos += 10; 
				size_t len = str.find_last_of("\"");
				if (len != std::string::npos)
				{
					len -= fileNamePos;
					fileName = str.substr( fileNamePos ,  len); 
				}
			}
		}
		if(fileName.empty())
		{
			// std::cout << LC_YELLOW << " ^ SKIPPING THIS ONE SINCE IT IS NOT attachment" << LC_RESET << std::endl;
		}
		else
		{
			// std::cout << LC_YELLOW << " fileName = " << fileName  << LC_RESET << std::endl;
			fileCount ++; 
			std::string	targetFile = route->getUploadStore() + "/" + fileName;
			std::cout << " targetFile = " << targetFile << std::endl;
			if(Util::fileExists(targetFile))
			{
				Logger::log(LC_MINOR_NOTE, " filename %s is already exists", targetFile.c_str());
				throw RequestException(403, "Forbidden");

			}
			std::string ext = Util::getFileExtension(fileName);
			std::cout << " ext = " << ext  << std::endl;
			std::map<std::string, std::string> cgis = conn->getServerConfig().getAllRouteCGIs();
			
			if( cgis.find(ext) !=  cgis.end())
			{
				// is one of the CGIs, skip 
				Logger::log(LC_MINOR_NOTE, " %s is one of the CGI files, skip for security reason", fileName.c_str());
				throw RequestException(415, "Unsupported Media Type");
			}
			else
			{
				Logger::log(LC_MINOR_NOTE, "SEEMS OK, proceed to create the file");

				size_t contentStart = tokens[i].find("\r\n\r\n");
				if(contentStart != std::string::npos)
				{
					size_t 	actualSize = tokens[i].size() - contentStart - 4;
					if(actualSize > route->getClientMaxBodySize() * WEBS_MB)
						throw RequestException(413, "Payload Too Large");

					contentStart += 4; 
					std::string	targetFile = route->getUploadStore() + "/" + fileName;
					if( Util::createFile(targetFile, tokens[i].begin() + contentStart , tokens[i].length() - contentStart - 4))
					{
						success ++;
					}
				}
			}
		}
	}
		
	Logger::log(LC_MINOR_NOTE, "Total File count = %d" , fileCount);
	if(fileCount == 0)
		throw RequestException(400, "Bad Request");
	// multiple statuses, but at least one success anyway!
	if(fileCount != success)
		throw RequestException(201, "Created");
	throw RequestException(201 , "Seems OK!");
}
