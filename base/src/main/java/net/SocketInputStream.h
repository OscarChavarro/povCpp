#ifndef __SOCKET_INPUT_STREAM__
#define __SOCKET_INPUT_STREAM__

#include "java/io/InputStream.h"
namespace java {
namespace net {

class SocketInputStream : public java::InputStream {
    int fd_;
public:
    explicit SocketInputStream(int fd) : fd_(fd) {}
    int read() override;
    int read(unsigned char* buffer, int offset, int length) override;
    void close() override;
};

} // net
} // java

#endif
