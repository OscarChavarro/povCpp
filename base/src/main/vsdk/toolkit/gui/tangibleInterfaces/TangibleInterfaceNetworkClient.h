#ifndef __TANGIBLE_INTERFACE_NETWORK_CLIENT__
#define __TANGIBLE_INTERFACE_NETWORK_CLIENT__

#include "java/util/function/Consumer.h"
#include <pthread.h>

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent.h"
class TangibleInterfaceListener;
namespace java { namespace net { class WebSocket; } }

class TangibleInterfaceNetworkClient {
private:
    java::String serviceUrl;
    java::ArrayList<TangibleInterfaceListener*> listeners;
    pthread_mutex_t listenersMutex;
    pthread_t connectionThread;
    bool threadStarted;
    bool stopRequested;
    java::net::WebSocket* webSocket;
    java::Consumer<bool> connectionListener;

    static void* threadEntry(void* arg);
    void connectAndListen();
    void notifyListeners(const TangibleInterfaceEvent& event);
    void processMessage(const java::String& message);
    bool parseEvent(const java::String& groupJson, TangibleInterfaceEvent* outEvent);
    static bool extractString(const java::String& text, const char* key, java::String* out);
    static bool extractNumbers(const java::String& text, const char* key, double* values, int count);

public:
    explicit TangibleInterfaceNetworkClient(const java::String& serviceUrl);
    ~TangibleInterfaceNetworkClient();

    void addListener(TangibleInterfaceListener* listener);
    void removeListener(TangibleInterfaceListener* listener);
    void run();

    /**
    Starts the client and reports whether the websocket handshake succeeds,
    as `run(Consumer<Boolean>)` does in the Java port. The listener, if
    given, runs in the connection thread.
    @param connectionListener receives true once the client is ready to
    receive websocket messages, or false on failure
    */
    void run(const java::Consumer<bool>& connectionListener);
    void disconnect();
};

#endif
