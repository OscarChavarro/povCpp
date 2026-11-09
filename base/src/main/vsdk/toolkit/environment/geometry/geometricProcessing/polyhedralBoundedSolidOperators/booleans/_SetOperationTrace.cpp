#include <cstdio>

#include "java/lang/Boolean.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationTrace.h"

namespace {
const char* const TRACE_COPLANAR_TANGENTIAL_PROPERTY =
    "vsdk.setop.traceCoplanarTangential";
const char* const TRACE_PIPELINE_SUMMARY_PROPERTY =
    "vsdk.setop.tracePipelineSummary";
}

bool _SetOperationTrace::isCoplanarTangentialTraceEnabled()
{
    return java::Boolean::getBoolean(TRACE_COPLANAR_TANGENTIAL_PROPERTY);
}

bool _SetOperationTrace::isPipelineSummaryTraceEnabled()
{
    return java::Boolean::getBoolean(TRACE_PIPELINE_SUMMARY_PROPERTY);
}

void _SetOperationTrace::traceCoplanarTangential(const java::String& message)
{
    if ( !isCoplanarTangentialTraceEnabled() ) {
        return;
    }
    printf("[SetOpCoplanarTrace] %s\n", message.c_str());
}

void _SetOperationTrace::tracePipelineSummary(const java::String& message)
{
    if ( !isPipelineSummaryTraceEnabled() ) {
        return;
    }
    printf("[SetOpPipelineTrace] %s\n", message.c_str());
}
