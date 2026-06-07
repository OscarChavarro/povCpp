#ifndef __VSDK_TOOLKIT_COMMON_LOGGING_LOGGER_H__
#define __VSDK_TOOLKIT_COMMON_LOGGING_LOGGER_H__


#include <exception>
#include "java/lang/String.h"
#include "java/lang/String.h"
#include "java/lang/String.h"

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


#endif // __VSDK_TOOLKIT_COMMON_LOGGING_LOGGER_H__
