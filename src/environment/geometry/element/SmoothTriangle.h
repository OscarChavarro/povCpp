#ifndef __SMOOTH_TRIANGLE_H__
#define __SMOOTH_TRIANGLE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/Triangle.h"
#include "environment/geometry/element/Triangle.h"

class SmoothTriangle : public Triangle {
  public:
    Vector3Dd n1;
    Vector3Dd n2;
    Vector3Dd n3;
    Vector3Dd perp;
    double baseDelta;

    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  protected:
    void swapVertexNormals() override;
    void finalizeComputation() override;
};

#endif
