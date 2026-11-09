#ifndef __SOCKET__
#define __SOCKET__

#include "java/lang/String.h"
#include "java/net/SocketInputStream.h"
#include "java/net/SocketOutputStream.h"
namespace java {
namespace net {

class Socket {
    int fd_;
    SocketInputStream  in_;
    SocketOutputStream out_;
    bool ownsSocket_;

    static int connectToHost(const java::String& host, int port);

public:
    Socket(const java::String& host, int port);
    explicit Socket(int fd) : fd_(fd), in_(fd), out_(fd), ownsSocket_(true) {}
    ~Socket() { if (ownsSocket_) close(); }

    java::InputStream*  getInputStream()  { return &in_;  }
    java::OutputStream* getOutputStream() { return &out_; }
    bool isConnected() const { return fd_ >= 0; }
    int fd() const { return fd_; }
    int releaseFd() { ownsSocket_ = false; return fd_; }
    void close();
};

} // net
} // java

#endif
