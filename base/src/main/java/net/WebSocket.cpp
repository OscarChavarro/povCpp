#include "java/lang/String.h"
#include <cstdlib>
#include <cstring>

#include "java/io/InputStream.h"
#include "java/io/OutputStream.h"
#include "java/net/Socket.h"
#include "java/net/WebSocket.h"

namespace java {
namespace net {

static int findText(const java::String& text, const char* needle, int fromIndex = 0)
{
    const char* base = text.c_str();
    const char* p = std::strstr(base + fromIndex, needle);
    if ( p == 0 ) {
        return java::String::npos;
    }
    return static_cast<int>(p - base);
}

WebSocket::WebSocket(Socket* socket)
    : socket_(socket), open_(socket != 0 && socket->isConnected())
{
}

WebSocket::~WebSocket()
{
    close();
    delete socket_;
}

WebSocket* WebSocket::connect(const java::String& serviceUrl)
{
    ParsedUrl url;
    if ( !parseUrl(serviceUrl, &url) ) {
        return 0;
    }

    Socket* socket = new Socket(url.host, url.port);
    if ( !socket->isConnected() ) {
        delete socket;
        return 0;
    }

    bool handshakeOk = false;
    try {
        handshakeOk = sendHandshake(socket, url);
    }
    catch ( ... ) {
        handshakeOk = false;
    }
    if ( !handshakeOk ) {
        delete socket;
        return 0;
    }

    return new WebSocket(socket);
}

bool WebSocket::readText(java::String* outPayload)
{
    if ( socket_ == 0 || !open_ || outPayload == 0 ) {
        return false;
    }

    unsigned char header[2];
    if ( !readExact(socket_, header, 2) ) {
        open_ = false;
        return false;
    }

    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    unsigned long long payloadLen = header[1] & 0x7F;

    if ( payloadLen == 126 ) {
        unsigned char ext[2];
        if ( !readExact(socket_, ext, 2) ) {
            open_ = false;
            return false;
        }
        payloadLen = (static_cast<unsigned long long>(ext[0]) << 8) | ext[1];
    }
    else if ( payloadLen == 127 ) {
        unsigned char ext[8];
        if ( !readExact(socket_, ext, 8) ) {
            open_ = false;
            return false;
        }
        payloadLen = 0;
        for ( int i = 0; i < 8; i++ ) {
            payloadLen = (payloadLen << 8) | ext[i];
        }
    }

    unsigned char mask[4] = {0, 0, 0, 0};
    if ( masked && !readExact(socket_, mask, 4) ) {
        open_ = false;
        return false;
    }

    if ( opcode == 8 ) {
        open_ = false;
        return false;
    }
    if ( opcode != 1 || payloadLen > 1024 * 1024 ) {
        return discard(socket_, payloadLen);
    }

    char* payload = new char[static_cast<size_t>(payloadLen) + 1];
    if ( !readExact(socket_, reinterpret_cast<unsigned char*>(payload), static_cast<int>(payloadLen)) ) {
        delete[] payload;
        open_ = false;
        return false;
    }
    for ( unsigned long long i = 0; i < payloadLen; i++ ) {
        if ( masked ) {
            payload[i] = static_cast<char>(payload[i] ^ mask[i % 4]);
        }
    }
    payload[payloadLen] = '\0';
    *outPayload = java::String(payload);
    delete[] payload;
    return true;
}

void WebSocket::close()
{
    open_ = false;
    if ( socket_ != 0 ) {
        socket_->close();
    }
}

bool WebSocket::isOpen() const
{
    return open_;
}

bool WebSocket::parseUrl(const java::String& url, ParsedUrl* outUrl)
{
    const char* prefix = "ws://";
    if ( !extractStringPrefix(url, prefix) ) {
        return false;
    }

    int hostStart = 5;
    int pathStart = url.find('/', hostStart);
    java::String hostPort = pathStart == java::String::npos ?
        url.substr(hostStart) : url.substr(hostStart, pathStart - hostStart);
    outUrl->path = pathStart == java::String::npos ? java::String("/") : url.substr(pathStart);

    int colon = hostPort.find(':');
    if ( colon == java::String::npos ) {
        outUrl->host = hostPort;
        outUrl->port = 80;
    }
    else {
        outUrl->host = hostPort.substr(0, colon);
        outUrl->port = std::atoi(hostPort.substr(colon + 1).c_str());
    }
    return !outUrl->host.isEmpty() && outUrl->port > 0;
}

bool WebSocket::extractStringPrefix(const java::String& text, const char* prefix)
{
    return findText(text, prefix) == 0;
}

bool WebSocket::readHttpHeader(Socket* socket, java::String* outHeader)
{
    char buffer[8192];
    int n = 0;
    java::InputStream* input = socket->getInputStream();
    while ( n < 8191 ) {
        int c = input->read();
        if ( c < 0 ) {
            return false;
        }
        buffer[n++] = static_cast<char>(c);
        if ( n >= 4 && buffer[n - 4] == '\r' && buffer[n - 3] == '\n' &&
             buffer[n - 2] == '\r' && buffer[n - 1] == '\n' ) {
            break;
        }
    }
    buffer[n] = '\0';
    *outHeader = java::String(buffer);
    return true;
}

bool WebSocket::sendHandshake(Socket* socket, const ParsedUrl& url)
{
    java::String request = java::String("GET ") + url.path + " HTTP/1.1\r\n"
        + "Host: " + url.host + "\r\n"
        + "Upgrade: websocket\r\n"
        + "Connection: Upgrade\r\n"
        + "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        + "Sec-WebSocket-Version: 13\r\n\r\n";

    java::OutputStream* output = socket->getOutputStream();
    output->write(reinterpret_cast<const unsigned char*>(request.c_str()), 0, request.length());
    output->flush();

    java::String response;
    return readHttpHeader(socket, &response) && findText(response, "101") != java::String::npos;
}

bool WebSocket::readExact(Socket* socket, unsigned char* buffer, int length)
{
    int done = 0;
    java::InputStream* input = socket->getInputStream();
    while ( done < length ) {
        int n = input->read(buffer, done, length - done);
        if ( n <= 0 ) {
            return false;
        }
        done += n;
    }
    return true;
}

bool WebSocket::discard(Socket* socket, unsigned long long length)
{
    unsigned char buffer[512];
    while ( length > 0 ) {
        int chunk = length > sizeof(buffer) ? static_cast<int>(sizeof(buffer)) : static_cast<int>(length);
        if ( !readExact(socket, buffer, chunk) ) {
            return false;
        }
        length -= static_cast<unsigned long long>(chunk);
    }
    return true;
}

} // net
} // java
