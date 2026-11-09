#ifndef __WEB_SOCKET__
#define __WEB_SOCKET__

#include "java/lang/String.h"

namespace java {
namespace net {

class Socket;

class WebSocket {
private:
    struct ParsedUrl {
        java::String host;
        int port;
        java::String path;
    };

    Socket* socket_;
    bool open_;

    explicit WebSocket(Socket* socket);

    static bool parseUrl(const java::String& url, ParsedUrl* outUrl);
    static bool extractStringPrefix(const java::String& text, const char* prefix);
    static bool readHttpHeader(Socket* socket, java::String* outHeader);
    static bool sendHandshake(Socket* socket, const ParsedUrl& url);
    static bool readExact(Socket* socket, unsigned char* buffer, int length);
    static bool discard(Socket* socket, unsigned long long length);

public:
    ~WebSocket();

    static WebSocket* connect(const java::String& serviceUrl);
    bool readText(java::String* outPayload);
    void close();
    bool isOpen() const;
};

} // net
} // java

#endif
