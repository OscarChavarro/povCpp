#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"

class Light : public Geometry {
  public:
    GeometryTypes geometryType;
    Vector3Dd Center;
    Vector3Dd pointsAt;
    Light *Next_Light_Source;
    bool Inverted;
    double Coeff;
    double Radius;
    double Falloff;

    virtual double attenuate(const RayWithSegments *lightSourceRay) const = 0;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void *copy() override = 0;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
    void copyStateInto(Light *dst) const;
};

#endif
