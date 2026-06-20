#ifndef JAVA_UTIL_CONCURRENT_EXECUTIONEXCEPTION_H
#define JAVA_UTIL_CONCURRENT_EXECUTIONEXCEPTION_H

#include "java/lang/String.h"
#include "java/lang/System.h"
#include <exception>
namespace java {

class ExecutionException : public std::exception {
private:
    java::String message_;

public:
    explicit ExecutionException(const java::String& msg) : message_(msg)
    {
        java::System::out.println(message_.c_str());
    }

    const char* what() const noexcept override
    {
        return message_.c_str();
    }
};

}

#endif
