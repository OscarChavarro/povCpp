#ifndef __EXECUTION_EXCEPTION__
#define __EXECUTION_EXCEPTION__

#include "java/lang/Exception.h"
#include "java/lang/String.h"
#include "java/lang/System.h"

namespace java {

/**
Emulation of `java.util.concurrent.ExecutionException`.
*/
class ExecutionException : public Exception {
public:
    explicit ExecutionException(const java::String& msg) : Exception(msg)
    {
        java::System::out.println(msg.c_str());
    }
};

}

#endif
