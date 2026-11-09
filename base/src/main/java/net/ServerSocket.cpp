#include <cerrno>
#include <cstdio>
#include <cstring>

#include "java/net/ServerSocket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
namespace java {
namespace net {

ServerSocket::ServerSocket(int port) : listenFd_(-1), port_(port) {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) { std::perror("ServerSocket: socket"); return; }

    int yes = 1;
    ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(static_cast<uint16_t>(port));

    if (::bind(listenFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("ServerSocket: bind");
        ::close(listenFd_); listenFd_ = -1; return;
    }
    if (::listen(listenFd_, 8) < 0) {
        std::perror("ServerSocket: listen");
        ::close(listenFd_); listenFd_ = -1;
    }
}

Socket* ServerSocket::accept() {
    int clientFd = ::accept(listenFd_, NULL, NULL);
    if (clientFd < 0) {
        if (errno != EINTR && errno != EBADF && errno != EINVAL && errno != ECONNABORTED) {
            std::perror("ServerSocket: accept");
        }
        return NULL;
    }
    return new Socket(clientFd);
}

void ServerSocket::close() {
    if (listenFd_ >= 0) {
        ::close(listenFd_);
        listenFd_ = -1;
    }
}

} // net
} // java
