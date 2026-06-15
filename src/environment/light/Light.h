#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"

class Light : public Geometry {
  public:
    GeometryTypes geometryType;
    Vector3Dd center;
    Vector3Dd pointsAt;
    Light *nextLightSource;
    bool inverted;
    double coeff;
    double radius;
    double falloff;

    virtual double attenuate(const RayWithSegments *lightSourceRay) const = 0;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void *copy() override = 0;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
    void translateGeometry(Vector3Dd *vector);
    void rotateGeometry(Vector3Dd *vector);
    void scaleGeometry(Vector3Dd *vector);
    void invertGeometry();
    void copyStateInto(Light *dst) const;
};

#endif
