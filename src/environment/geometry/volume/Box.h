#ifndef __BOX_H__
#define __BOX_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Box : public Geometry {
  public:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    Vector3Dd bounds[2];
    bool Inverted;

    static int intersectBoxx(const RayWithSegments *ray, const Box *box,
        double *depth1, double *depth2);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  private:
    static int closeTo(double x, double y);
};

#endif
