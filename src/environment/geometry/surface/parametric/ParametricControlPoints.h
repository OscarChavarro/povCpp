#ifndef __PARAMETRIC_CONTROL_POINTS_H__
#define __PARAMETRIC_CONTROL_POINTS_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ParametricControlPoints {
  public:
    Vector3Dd *getVertices();
    const Vector3Dd *getVertices() const;

  private:
    Vector3Dd vertices[4];
};

inline Vector3Dd *
ParametricControlPoints::getVertices()
{
    return vertices;
}

inline const Vector3Dd *
ParametricControlPoints::getVertices() const
{
    return vertices;
}

#endif
