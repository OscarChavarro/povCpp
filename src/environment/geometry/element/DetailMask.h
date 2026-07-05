#ifndef __DETAIL_MASK__
#define __DETAIL_MASK__

enum DetailMask {
    NONE = 0,
    POINT = 1 << 0,
    NORMAL = 1 << 1,
    UV = 1 << 2,
    TANGENT = 1 << 3,
    ALL = POINT | NORMAL | UV | TANGENT
};

#endif
