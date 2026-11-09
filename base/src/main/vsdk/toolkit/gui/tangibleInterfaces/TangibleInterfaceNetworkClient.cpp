#include "java/lang/String.h"
#include "java/util/function/Consumer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "java/net/WebSocket.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceListener.h"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceNetworkClient.h"

static int findText(const java::String& text, const char* needle, int fromIndex = 0)
{
    const char* base = text.c_str();
    const char* p = std::strstr(base + fromIndex, needle);
    if ( p == 0 ) {
        return java::String::npos;
    }
    return static_cast<int>(p - base);
}

TangibleInterfaceNetworkClient::TangibleInterfaceNetworkClient(const java::String& serviceUrl)
    : serviceUrl(serviceUrl),
      threadStarted(false),
      stopRequested(false),
      webSocket(0)
{
    pthread_mutex_init(&listenersMutex, 0);
}

TangibleInterfaceNetworkClient::~TangibleInterfaceNetworkClient()
{
    disconnect();
    pthread_mutex_destroy(&listenersMutex);
}

void TangibleInterfaceNetworkClient::addListener(TangibleInterfaceListener* listener)
{
    if ( listener == 0 ) {
        return;
    }

    pthread_mutex_lock(&listenersMutex);
    listeners.add(listener);
    pthread_mutex_unlock(&listenersMutex);
}

void TangibleInterfaceNetworkClient::removeListener(TangibleInterfaceListener* listener)
{
    pthread_mutex_lock(&listenersMutex);
    listeners.remove(listener);
    pthread_mutex_unlock(&listenersMutex);
}

void TangibleInterfaceNetworkClient::run()
{
    run(java::Consumer<bool>());
}

void TangibleInterfaceNetworkClient::run(
    const java::Consumer<bool>& connectionListener)
{
    if ( threadStarted ) {
        return;
    }
    this->connectionListener = connectionListener;
    threadStarted = pthread_create(&connectionThread, 0, threadEntry, this) == 0;
}

void TangibleInterfaceNetworkClient::disconnect()
{
    stopRequested = true;
    if ( webSocket != 0 ) {
        webSocket->close();
    }
    if ( threadStarted && !pthread_equal(pthread_self(), connectionThread) ) {
        pthread_join(connectionThread, 0);
        threadStarted = false;
    }
    delete webSocket;
    webSocket = 0;
}

void* TangibleInterfaceNetworkClient::threadEntry(void* arg)
{
    TangibleInterfaceNetworkClient* client = static_cast<TangibleInterfaceNetworkClient*>(arg);
    if ( client != 0 ) {
        client->connectAndListen();
    }
    return 0;
}

void TangibleInterfaceNetworkClient::connectAndListen()
{
    webSocket = java::net::WebSocket::connect(serviceUrl);
    if ( webSocket == 0 ) {
        std::printf("Tangible interface server not found at %s\n", serviceUrl.c_str());
        if ( connectionListener ) {
            connectionListener.accept(false);
        }
        return;
    }
    if ( stopRequested ) {
        webSocket->close();
        return;
    }
    if ( connectionListener ) {
        connectionListener.accept(true);
    }

    while ( !stopRequested ) {
        java::String payload;
        if ( !webSocket->readText(&payload) ) {
            break;
        }
        processMessage(payload);
    }
    if ( webSocket != 0 ) {
        webSocket->close();
    }
}

void TangibleInterfaceNetworkClient::notifyListeners(const TangibleInterfaceEvent& event)
{
    java::ArrayList<TangibleInterfaceListener*> snapshot;

    pthread_mutex_lock(&listenersMutex);
    for ( long i = 0; i < listeners.size(); i++ ) {
        snapshot.add(listeners.get(i));
    }
    pthread_mutex_unlock(&listenersMutex);

    for ( long i = 0; i < snapshot.size(); i++ ) {
        TangibleInterfaceListener* listener = snapshot.get(i);
        if ( listener != 0 ) {
            listener->tangibleInterfaceEventReceived(event);
        }
    }
}

void TangibleInterfaceNetworkClient::processMessage(const java::String& message)
{
    int len = message.length();
    for ( int i = 0; i < len; i++ ) {
        if ( message[i] != '{' ) {
            continue;
        }
        int start = i;
        while ( i < len && message[i] != '}' ) {
            i++;
        }
        if ( i >= len ) {
            break;
        }
        java::String groupJson = message.substr(start, i - start + 1);
        TangibleInterfaceEvent event;
        if ( parseEvent(groupJson, &event) ) {
            notifyListeners(event);
        }
    }
}

bool TangibleInterfaceNetworkClient::parseEvent(const java::String& groupJson, TangibleInterfaceEvent* outEvent)
{
    java::String id;
    double positionValues[3];
    double quaternionValues[4];

    if ( !extractString(groupJson, "label", &id) ||
         !extractNumbers(groupJson, "position", positionValues, 3) ||
         !extractNumbers(groupJson, "quaternion", quaternionValues, 4) ) {
        return false;
    }

    Vector3Dd position(positionValues[0], positionValues[1], positionValues[2]);
    Vector3Dd direction(quaternionValues[1], quaternionValues[2], quaternionValues[3]);
    Quaterniond rotation(direction, quaternionValues[0]);
    *outEvent = TangibleInterfaceEvent(id, position, rotation);
    return true;
}

bool TangibleInterfaceNetworkClient::extractString(const java::String& text, const char* key, java::String* out)
{
    java::String needle = java::String("\"") + key + "\"";
    int p = findText(text, needle.c_str());
    if ( p == java::String::npos ) {
        return false;
    }
    p = text.find(':', p + needle.length());
    if ( p == java::String::npos ) {
        return false;
    }
    p = text.find('"', p);
    if ( p == java::String::npos ) {
        return false;
    }
    int e = text.find('"', p + 1);
    if ( e == java::String::npos ) {
        return false;
    }
    *out = text.substr(p + 1, e - p - 1);
    return true;
}

bool TangibleInterfaceNetworkClient::extractNumbers(const java::String& text, const char* key, double* values, int count)
{
    java::String needle = java::String("\"") + key + "\"";
    int p = findText(text, needle.c_str());
    if ( p == java::String::npos ) {
        return false;
    }
    p = text.find('[', p + needle.length());
    int e = text.find(']', p);
    if ( p == java::String::npos || e == java::String::npos ) {
        return false;
    }

    java::String payload = text.substr(p + 1, e - p - 1);
    int start = 0;
    for ( int i = 0; i < count; i++ ) {
        int comma = payload.find(',', start);
        java::String token = (i == count - 1) ?
            payload.substr(start) : payload.substr(start, comma - start);
        if ( i != count - 1 && comma == java::String::npos ) {
            return false;
        }
        values[i] = std::atof(token.c_str());
        start = comma + 1;
    }
    return true;
}
