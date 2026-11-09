#include "java/lang/RuntimeException.h"
#include <cstdint>

#include "java/net/SocketOutputStream.h"
#include <sys/socket.h>
#include <unistd.h>
namespace java {
namespace net {

void SocketOutputStream::write(int value) {
    uint8_t b = static_cast<uint8_t>(value & 0xFF);
    if (::send(fd_, &b, 1, 0) <= 0)
        throw java::RuntimeException("socket write error");
}

void SocketOutputStream::write(const unsigned char* buffer, int offset, int length) {
    const unsigned char* p = buffer + offset;
    int remaining = length;
    while (remaining > 0) {
        ssize_t sent = ::send(fd_, p, static_cast<size_t>(remaining), 0);
        if (sent <= 0)
            throw java::RuntimeException("socket write error");
        p += sent;
        remaining -= static_cast<int>(sent);
    }
}

void SocketOutputStream::close() {
    ::close(fd_);
}

} // net
} // java
