#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class SmoothTriangle;

class Triangle : public Geometry {
  public:
    Vector3Dd normalVector;
    double distance;
    double vpNormDotOrigin;
    unsigned int vpCached : 1;
    unsigned int dominantAxis : 2;
    unsigned int inverted : 1;
    unsigned int vAxis : 2;
    Vector3Dd p1;
    Vector3Dd p2;
    Vector3Dd p3;
    bool degenerateFlag;

    double getDistance() const { return distance; }
    void setDistance(double d) { distance = d; }

    static int computeTriangle(Triangle *triangle);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  protected:
    virtual void swapVertexNormals() {}
    virtual void finalizeComputation() {}
    static void computeSmoothTriangle(SmoothTriangle *triangle);

  private:
    static int max3Axis(double x, double y, double z);
    static void findTriangleDominantAxis(Triangle *triangle);
    static int intersectTriangle(
        RayWithSegments *ray, Triangle *triangle, double *depth);
};

#include "environment/geometry/elements/SmoothTriangle.h"

#endif
