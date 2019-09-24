#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string_view>

static std::string rootdir = ".";
static std::string ip = "127.0.0.1";
static uint16_t port = 8080;

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

    if (listen(sockfd, 10) == -1) {
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
        if (!fork()) {
            close(sockfd);
            if (read(newfd, in, 4096) == -1) {
                perror("receive");
                exit(1);
            }
            
            std::printf("%s\n", in);

            handle_request(newfd, in);

            exit(0);
        }

        close(newfd);
    }

    return 0;
}

void handle_request(int fd, std::string request) {
    if (request.starts_with("GET")) {
        request.erase(0, 4).erase(request.find_first_of(' '), request.length() - 1);
        std::string fpath = rootdir + request;
        std::cout << fpath << '\n';
    }
}
