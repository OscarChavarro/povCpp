#ifndef ___SET_OPERATION_TRACE__
#define ___SET_OPERATION_TRACE__

#include "java/lang/String.h"

/**
Centralized, property-gated diagnostic tracing for the boolean set-operation
pipeline.

Tracing is enabled per category through system properties (see
`java::System::setProperty`), so it stays inert in normal runs and is
reachable for incident diagnosis:
  - `vsdk.setop.traceCoplanarTangential`
  - `vsdk.setop.tracePipelineSummary`

C++ counterpart of Java's `_SetOperationTrace`.
*/
class _SetOperationTrace {
public:
    static bool isCoplanarTangentialTraceEnabled();
    static bool isPipelineSummaryTraceEnabled();
    static void traceCoplanarTangential(const java::String& message);
    static void tracePipelineSummary(const java::String& message);

private:
    _SetOperationTrace() {}
};

#endif
