#ifndef __JAVA_UNSUPPORTED_OPERATION_EXCEPTION__
#define __JAVA_UNSUPPORTED_OPERATION_EXCEPTION__

#include "java/lang/RuntimeException.h"

namespace java {

/**
Emulation of `java.lang.UnsupportedOperationException`.
*/
class UnsupportedOperationException : public RuntimeException {
  public:
    UnsupportedOperationException() : RuntimeException() {}
    explicit UnsupportedOperationException(const String &message) : RuntimeException(message) {}
    virtual ~UnsupportedOperationException() {}
};

}

#endif
