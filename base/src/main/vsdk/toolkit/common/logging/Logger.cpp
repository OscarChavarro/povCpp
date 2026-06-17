#include <cstdio>
#include <cstdlib>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
bool Logger::withSystemExit_ = true;
bool Logger::withFatalExceptions_ = true;

void Logger::setWithSystemExit(bool flag) { withSystemExit_ = flag; }
void Logger::setWithFatalExceptions(bool flag) { withFatalExceptions_ = flag; }

void Logger::processFatalError(const java::String& method, const java::String& message, const std::exception* cause)
{
    if ( withSystemExit_ ) {
        std::exit(1);
    }
    if ( !withFatalExceptions_ ) {
        return;
    }
    java::String m = method.empty() ? "VSDK fatal error" : ("VSDK fatal error at " + method);
    if ( !message.empty() ) {
        m += ": " + message;
    }
    if ( cause != nullptr ) {
        m += " | cause: ";
        m += cause->what();
    }
    throw VSDKFatalException(m);
}

void Logger::reportMessage(const java::String& className, int level, const java::String& method, const java::String& message)
{
    fprintf(stderr, "[VSDK][%s] %s: %s\n", className.c_str(), method.c_str(), message.c_str());
    if ( level == FATAL_ERROR ) {
        processFatalError(method, message, nullptr);
    }
}

void Logger::reportMessageWithException(const java::String& className, int level, const java::String& method, const java::String& message, const std::exception* cause)
{
    if ( cause != nullptr ) {
        fprintf(stderr, "[VSDK][%s] %s: %s | exception: %s\n", className.c_str(), method.c_str(), message.c_str(), cause->what());
    } else {
        fprintf(stderr, "[VSDK][%s] %s: %s\n", className.c_str(), method.c_str(), message.c_str());
    }
    if ( level == FATAL_ERROR ) {
        processFatalError(method, message, cause);
    }
}

