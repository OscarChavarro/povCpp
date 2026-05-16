#ifndef __SMOOTH_TRIANGLE_H__
#define __SMOOTH_TRIANGLE_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class SmoothTriangle : public Geometry {
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
    Vector3D N1, N2, N3, Perp;
    DBL BaseDelta;
};

#endif
