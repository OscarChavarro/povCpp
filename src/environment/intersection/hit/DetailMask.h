#ifndef __DETAIL_MASK__
#define __DETAIL_MASK__

#include "vsdk/toolkit/environment/geometry/element/SurfaceDetail.h"

enum DetailMask {
    NONE = SurfaceDetail::NONE,
    POINT = SurfaceDetail::POINT,
    NORMAL = SurfaceDetail::NORMAL,
    UV = SurfaceDetail::UV,
    TANGENT = SurfaceDetail::TANGENT,
    ALL = SurfaceDetail::ALL
};

#endif
