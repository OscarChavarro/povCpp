#ifndef VSDK_HEIGHT_FIELD_TRACE_RESULT_H
#define VSDK_HEIGHT_FIELD_TRACE_RESULT_H

// Outcome of HeightFieldSolid::traceFirstHit: the ray missed the bounding
// box, crossed it without hitting the surface, or hit the surface.
enum class HeightFieldTraceResult { MissedBounds, MissedSurface, Hit };
#endif
