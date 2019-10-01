#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mimetypes.h"
#include "config.h"

/* Default Config Parameters */
#ifndef CONF_INDEX
#define CONF_INDEX "index.html"
#endif

#ifndef CONF_404
#define CONF_404 "404.html"
#endif

#ifndef CONF_SERVER_NAME
#define CONF_SERVER_NAME "cppserver"
#endif

#ifndef CONF_BACKLOG
#define CONF_BACKLOG 10
#elif CONF_BACKLOG <= 0
#error "CONF_BACKLOG must be greater than zero!"
#endif

static std::string rootdir = ".";
static std::string ip = "127.0.0.1";
static uint16_t port = 8080;
const static std::string servername = CONF_SERVER_NAME;

void printusage(char* exe) {
    std::fprintf(stderr,
            "Usage: %s <port> [root]\n"
            "   port: the port to listen on\n"
            "   root: the directory which should correspond to \"/\"\n"
            "         (defaults to \".\")\n",
            exe);
}

// listen for connections and serve requests
int runserver();
void handle_request(int, std::string);

int main(int argc, char** argv) {
    if (argc <= 2) {
        printusage(argv[0]);
        return 1;
    }

    if (argc >= 1) {
        port = (uint16_t)atoi(argv[1]);
    }
    if (argc >= 2) {
        rootdir = std::string(argv[2]);
        struct stat s;
        if (stat(rootdir.c_str(), &s) == 0) {
            if (!(s.st_mode & S_IFDIR)) {
                std::fprintf(stderr,
                        "Path is not a directory: %s\n",
                        rootdir.c_str());
                return 1;
            }
        } else {
            perror("Error parsing root directory");
            return errno;
        }
    }

    // listen for connections
    return runserver();
}

int runserver() {
    struct sockaddr_in myaddr;
    int sockfd;
    int yes = 1;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
    }

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(myaddr.sin_zero), 0, 8);

    if (bind(sockfd, (struct sockaddr*)&myaddr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(sockfd, CONF_BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    int newfd;
    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    char in[4096];
    
    while (true) {
        if ((newfd = accept(sockfd,
                        (struct sockaddr *)&client_addr,
                        &sin_size)) == -1) {
            perror("accept");
            continue;
        }

        // fork to child process
        // TODO make a thread pool with a configurable number of workers in it rather than using fork()
        if (!fork()) {
            close(sockfd);
            if (read(newfd, in, 4096) == -1) {
                perror("receive");
                exit(1);
            }

            handle_request(newfd, in);

            exit(0);
        }

        close(newfd);
    }

    return 0;
}

#ifdef CONF_DEBUG
// for the "/sleep" endpoint
#include <thread>
#include <chrono>
#endif

void handle_request(int fd, std::string request) {
    std::ifstream infile;
    std::string status, filecontents;
    std::stringstream packet;
    bool OK = false;

    std::string rqheader = request;
    rqheader.erase(rqheader.find("HTTP/1.1"), rqheader.length() - 1);

    std::printf("Received request: %s\n", rqheader.c_str());

    if (request.starts_with("GET")) {
        request.erase(0, 4).erase(request.find_first_of(' '), request.length() - 1);
        std::string fpath = request;
        
        if (fpath == "/") {
            fpath = "/" CONF_INDEX;
        }

#ifdef CONF_DEBUG
        if (fpath == "/sleep") {
            std::printf("Sleeping for 5 seconds, then serving index.\n");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            fpath = "/" CONF_INDEX;
        }
#endif

        std::string fext = fpath;
        fext.erase(0, fpath.find_last_of('.'));

        fpath = rootdir + fpath;

        if (fpath.find("../") != std::string::npos) {
            status = "403 Forbidden";
        } else if (0 == access(fpath.c_str(), F_OK)) {
            infile.open(fpath,
                    std::ios_base::in | std::ios_base::binary);
        
            OK = (bool)infile;
            
            status = "200 OK";

            if (!OK) {
                status = "500 Internal Server Error";
            } else {
                while (!infile.eof()) {
                    filecontents += infile.get();
                }
            }
        } else {
            status = "404 Not Found";
#ifdef CONF_404
            fpath = rootdir + CONF_404;
            fext = fpath;
            fext.erase(0, fpath.find_last_of('.'));

            infile.open(rootdir + "/" + CONF_404,
                    std::ios_base::in | std::ios_base::binary);
        
            OK = (bool)infile;

            if (!OK) {
                status = "500 Internal Server Error";
            } else {
                while (!infile.eof()) {
                    filecontents += infile.get();
                }
            }
#endif
        }

        packet << "HTTP/1.1 " << status << "\r\n";
        packet << "Server: " << servername << "\r\n";

        if (OK) {
            if (MIME_TYPES.find(fext) != MIME_TYPES.end()) {
                packet << "Content-Type: " << MIME_TYPES.at(fext) << "\r\n";
            } else {
                packet << "Content-Type: application/octet-stream\r\n";
            }
            packet << "Content-Length: " << filecontents.length() - 1 << "\r\n";
        }

        packet << "\r\n";

        if (OK) {
            packet << filecontents << "\r\n";
            infile.close();
        }

        write(fd, packet.str().c_str(), packet.str().length());
    }
}
