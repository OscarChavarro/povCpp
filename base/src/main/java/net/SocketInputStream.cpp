#include <cstdint>

#include "java/net/SocketInputStream.h"
#include <sys/socket.h>
#include <unistd.h>
namespace java {
namespace net {

int SocketInputStream::read() {
    uint8_t b;
    ssize_t n = ::recv(fd_, &b, 1, 0);
    return (n <= 0) ? -1 : static_cast<int>(b);
}

int SocketInputStream::read(unsigned char* buffer, int offset, int length) {
    ssize_t n = ::recv(fd_, buffer + offset, static_cast<size_t>(length), 0);
    return (n <= 0) ? -1 : static_cast<int>(n);
}

void SocketInputStream::close() {
    ::close(fd_);
}

} // net
} // java
