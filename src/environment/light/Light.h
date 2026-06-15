#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Light : public Geometry {
  public:
    Vector3Dd Center;
    Vector3Dd pointsAt;
    Light *Next_Light_Source;
    bool Inverted;
    double Coeff;
    double Radius;
    double Falloff;

    static double attenuateLight(
        const Light *lightSource, const RayWithSegments *lightSourceRay);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  private:
    static double cubicSpline(double low, double high, double pos);
};

#endif
