#include "java/net/Socket.h"
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
namespace java {
namespace net {

int Socket::connectToHost(const java::String& host, int port)
{
    char portText[32];
    std::snprintf(portText, sizeof(portText), "%d", port);

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = 0;
    if ( getaddrinfo(host.c_str(), portText, &hints, &result) != 0 ) {
        return -1;
    }

    int fd = -1;
    for ( addrinfo* rp = result; rp != 0; rp = rp->ai_next ) {
        fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if ( fd < 0 ) {
            continue;
        }
        if ( ::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0 ) {
            break;
        }
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(result);
    return fd;
}

Socket::Socket(const java::String& host, int port)
    : fd_(connectToHost(host, port)), in_(fd_), out_(fd_), ownsSocket_(true)
{
}

void Socket::close() {
    if (fd_ >= 0) {
        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
        fd_ = -1;
    }
}

} // net
} // java
