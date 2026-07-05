#ifndef __DETAIL_MASK__
#define __DETAIL_MASK__

// Single source of truth for the DETAIL_* bit values shared by
// RayWithTracingState (the mask's pre-hit transport) and PovRayHit (the mask's
// authority at doExtraInformation time). Kept as a dependency-free header so
// both classes can reference it without entangling their own include graphs
// (PovRayHit.h transitively pulls in RayWithTracingState.h via Geometry.h, so
// RayWithTracingState.h cannot include PovRayHit.h back without a cycle).
namespace DetailMask {
    constexpr int NONE = 0;
    constexpr int POINT = 1 << 0;
    constexpr int NORMAL = 1 << 1;
    constexpr int UV = 1 << 2;
    constexpr int TANGENT = 1 << 3;
    constexpr int ALL = POINT | NORMAL | UV | TANGENT;
}

#endif
