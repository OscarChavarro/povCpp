#ifndef __SMOOTH_TRIANGLE_H__
#define __SMOOTH_TRIANGLE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/Triangle.h"

class SmoothTriangle : public Triangle {
  public:
    Vector3Dd N1;
    Vector3Dd N2;
    Vector3Dd N3;
    Vector3Dd Perp;
    double BaseDelta;

    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
};

#endif
