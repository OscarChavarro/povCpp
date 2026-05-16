#ifndef __TRIANGLE_CLASS_H__
#define __TRIANGLE_CLASS_H__

#include "common/frame.h"
#include "common/vector.h"
#include "geom/geometry.h"

class Triangle : public Geometry {
  public:
    Vector3D Normal_Vector;
    DBL Distance;
    DBL VPNormDotOrigin;
    unsigned int VPCached : 1;
    unsigned int Dominant_Axis : 2;
    unsigned int Inverted : 1;
    unsigned int vAxis : 2;
    Vector3D P1, P2, P3;
    short int Degenerate_Flag;
};

#endif
