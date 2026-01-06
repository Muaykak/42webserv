#include "../include/classes/WebServ.hpp"

#include <iostream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cerrno>

class CaptureStdout {
private:
    std::string readStdout;
    std::string readStderr;
    int     _pipe[2];
    int     _pipeErr[2];
    int     orig_stdout;
    int     dup_stdout;
    int     orig_stderr;
    int     dup_stderr;

public:
    CaptureStdout() {
        // --- STEP 1: Setup STDOUT ---
        if (pipe(_pipe) != 0) {
            throw std::runtime_error("CaptureStdout::pipe() failed");
        }
        orig_stdout = STDOUT_FILENO;
        dup_stdout = dup(orig_stdout);
        if (dup_stdout < 0) {
            close(_pipe[0]); close(_pipe[1]);
            throw std::runtime_error("CaptureStdout::dup() failed");
        }

        // Make read end non-blocking
        int flags = fcntl(_pipe[0], F_GETFL, 0);
        fcntl(_pipe[0], F_SETFL, flags | O_NONBLOCK);

        fflush(stdout);
        if (dup2(_pipe[1], orig_stdout) < 0) {
            close(dup_stdout); close(_pipe[0]); close(_pipe[1]);
            throw std::runtime_error("CaptureStdout::dup2() failed");
        }
        close(_pipe[1]); // Close write end (stdout holds the ref now)

        // --- STEP 2: Setup STDERR ---
        if (pipe(_pipeErr) != 0) {
            // ROLLBACK STDOUT! (Crucial fix)
            restoreStdout();
            throw std::runtime_error("CaptureStdout::pipe(err) failed");
        }

        orig_stderr = STDERR_FILENO;
        dup_stderr = dup(orig_stderr);
        if (dup_stderr < 0) {
            // ROLLBACK STDOUT + CLEANUP NEW PIPE
            close(_pipeErr[0]); close(_pipeErr[1]);
            restoreStdout();
            throw std::runtime_error("CaptureStdout::dup(err) failed");
        }

        flags = fcntl(_pipeErr[0], F_GETFL, 0);
        fcntl(_pipeErr[0], F_SETFL, flags | O_NONBLOCK);

        fflush(stderr);
        if (dup2(_pipeErr[1], orig_stderr) < 0) {
            // ROLLBACK EVERYTHING
            close(dup_stderr); 
            close(_pipeErr[0]); close(_pipeErr[1]);
            restoreStdout();
            throw std::runtime_error("CaptureStdout::dup2(err) failed");
        }
        close(_pipeErr[1]); 
    }

    // Helper to undo Step 1 (used in destructor AND error handling)
    void restoreStdout() {
        fflush(stdout);
        dup2(dup_stdout, orig_stdout);
        close(dup_stdout);
        close(_pipe[0]);
    }

    // Helper to undo Step 2
    void restoreStderr() {
        fflush(stderr);
        dup2(dup_stderr, orig_stderr);
        close(dup_stderr);
        close(_pipeErr[0]);
    }

    ~CaptureStdout() {
        restoreStdout();
        restoreStderr();
    }

    // Generic read function to avoid code duplication
    std::string readFromPipe(int fd, std::string& buffer) {
        char readbuf[1024];
        ssize_t readReturn;

        while (true) {
            readReturn = read(fd, readbuf, sizeof(readbuf));
            if (readReturn > 0) {
                buffer.append(readbuf, readReturn);
            } else {
                if (readReturn == 0) break; // EOF
                if (errno == EAGAIN || errno == EWOULDBLOCK) break; // Empty
                throw std::runtime_error("read() failed");
            }
        }
        return buffer;
    }

    std::string getStringOut() {
        fflush(stdout); // Push buffered C-data to pipe
        return readFromPipe(_pipe[0], readStdout);
    }

    std::string getStringErr() {
        fflush(stderr); // Push buffered C-data to pipe
        return readFromPipe(_pipeErr[0], readStderr);
    }
};

// need to overload << and == for each of custom function/class to test
template <typename T>
bool	normalFunctionTester(const T& expectedResult, const T&actualResult, const std::string& testName){

	if (expectedResult == actualResult) {
        std::cout << LC_RES_OK_LOG "[PASS] " << testName << LC_RESET <<std::endl;
		return (true);
    } else {
        std::cout << LC_RES_NOK_LOG "[FAIL] " << testName 
                  << " -> Expected: \"" LC_RESET << expectedResult
                  << LC_RES_NOK_LOG "\", Got: \"" LC_YELLOW << actualResult << LC_RES_NOK_LOG "\"" << LC_RESET << std::endl;
		return (false);
    }
}

void	toStringTester(){
	normalFunctionTester(std::string("10"), toString(9),
	"toString((int)10)");
}

void	testerFileDesciptor(){

	{
			std::string outStringStdout;
			std::string outStringStderr;
		{
			CaptureStdout	capture;
			try {
				FileDescriptor	test1 = 10;
			} catch ( std::exception &e) {
				outStringStdout.append(e.what());
			}
			outStringStderr.append(capture.getStringErr());
		}
		normalFunctionTester(std::string("").empty(), outStringStdout.empty(),
		"FileDescriptorTest:Parameter Constructor#1");
		normalFunctionTester(std::string("CloseError").empty(), outStringStderr.empty(),
		"FileDescriptorTest:Parameter Constructor#1");
	}

	{
			std::string outString;
		try {
			CaptureStdout	capture;
			FileDescriptor	test1 = 2000;
		} catch ( std::exception &e) {
			outString.append(e.what());
		}
		normalFunctionTester(std::string("FileDescriptor::fd out of range::"), outString,
		"FileDescriptorTest:Parameter Constructor#2");
	}

	{
			std::string outString;
		try {
			CaptureStdout	capture;
			FileDescriptor	test1 = open("./main.cpp", O_RDONLY);
			FileDescriptor	test2 = test1;
		} catch ( std::exception &e) {
			outString.append(e.what());
		}
		normalFunctionTester(std::string(""), outString,
		"FileDescriptorTest:Parameter Constructor#3");
	}

	{
		std::string outString;
		try {
			CaptureStdout	capture;
			FileDescriptor	test1 = open("./falsePath", O_RDONLY);
			FileDescriptor	test2 = test1;
		} catch ( std::exception &e) {
			outString.append(e.what());
		}
		normalFunctionTester(std::string("FileDescriptor::fd out of range::"), outString,
		"FileDescriptorTest:Parameter Constructor#3");
	}

	{
		std::string outString;
		try {
			CaptureStdout	capture;
			FileDescriptor	test1 = open("./falsePath", O_RDONLY);
			FileDescriptor	test2 = test1;
		} catch ( std::exception &e) {
			outString.append(e.what());
		}
		normalFunctionTester(std::string("FileDescriptor::fd out of range::"), outString,
		"FileDescriptorTest:Parameter Constructor#3");
	}

}

int	main(){
	//testerFileDesciptor();
	try {
		ConfigData	test("default.conf");

		test.printConfigData();
	} catch (std::exception &e){
		std::cout << "ERROR::" << e.what() << std::endl;
	}
}