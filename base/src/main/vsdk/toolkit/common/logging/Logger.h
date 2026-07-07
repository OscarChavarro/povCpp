#ifndef __LOGGER__
#define __LOGGER__


#include "java/lang/String.h"
#include <exception>
class Logger {
public:
    static const int WARNING = 1;
    static const int ERROR = 2;
    static const int FATAL_ERROR = 3;

    static void setWithSystemExit(bool flag);
    static void setWithFatalExceptions(bool flag);

    static void reportMessage(const java::String& className, int level, const java::String& method, const java::String& message);
    static void reportMessageWithException(const java::String& className, int level, const java::String& method, const java::String& message, const std::exception* cause);

private:
    static bool withSystemExit_;
    static bool withFatalExceptions_;
    static void processFatalError(const java::String& method, const java::String& message, const std::exception* cause);
};


#endif
