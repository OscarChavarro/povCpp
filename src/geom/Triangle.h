#ifndef __TRIANGLE_H__
#define __TRIANGLES_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

#include "geom/SmoothTriangle.h"
#include "geom/TriangleClass.h"

extern Methods Triangle_Methods;
extern Triangle *getTriangleShape(void);
extern SmoothTriangle *getSmoothTriangleShape(void);
extern Methods Smooth_Triangle_Methods;

#endif
