#ifndef __SERVER_SOCKET__
#define __SERVER_SOCKET__

#include "java/net/Socket.h"
namespace java {
namespace net {

// Mirrors java.net.ServerSocket: binds a port, accepts client connections.
class ServerSocket {
    int listenFd_;
    int port_;
public:
    explicit ServerSocket(int port);
    ~ServerSocket() { close(); }

    // Blocks until a client connects. Caller owns the returned Socket*.
    Socket* accept();
    void close();
    bool isOpen() const { return listenFd_ >= 0; }
    int port() const { return port_; }
};

} // net
} // java

#endif
