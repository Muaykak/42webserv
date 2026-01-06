#ifndef DEFINED_VALUE_HPP
# define DEFINED_VALUE_HPP


// CONSTANT VALUE

//----------- LIMITS
// Soft limit of file descriptor allowed in this project
#define MAX_FD 1024
#define MAX_LISTENSOCKET_CONNECTION 100
#define MAX_EPOLL_EVENT 300
#define HTTP_RECV_BUFFER 4096

// Color Text
#define LC_RED "\033[31m"
#define LC_YELLOW "\033[33m"
#define LC_BOLD "\033[1m"
#define LC_RESET "\033[0m"
#define LC_ERROR "\033[31m\033[1m"
#define LC_DEBUG "\033[1m\033[34m"

#define LC_SYSTEM "\033[34m\033[1m"
#define LC_MINOR_SYSTEM "\033[34m\033[1m"
#define LC_INFO "\033[36m"
#define LC_NOTE "\033[33m"
#define LC_MINOR_NOTE "\033[30m "

#define LC_REQ_LOG "\033[33m"
#define LC_RES_OK_LOG "\033[32m"
#define LC_RES_NOK_LOG "\033[31m"
#define LC_RES_INT_LOG "\033[36m"
#define LC_RES_FND_LOG "\033[34m"
#define LC_CON_FAIL "\033[31m"
#define LC_CONN_LOG "\033[35m"


#endif