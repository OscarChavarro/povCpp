#ifndef __QUADRIC_H__
#define __QUADRIC_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Quadric : public Geometry {
  public:
    Vector3Dd object2Terms;
    Vector3Dd objectMixedTerms;
    Vector3Dd objectTerms;
    double objectConstant;
    double objectVpConstant;
    bool constantCached;
    bool nonZeroSquareTerm;

    static int intersectQuadric(
        RayWithSegments *ray, Quadric *shape, double *depth1, double *depth2);

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

  private:
    static void quadricToMatrix(const Quadric *quadric, Matrix4x4d *matrix);
    static void matrixToQuadric(const Matrix4x4d *matrix, Quadric *quadric);
    static void transformQuadric(
        Quadric *shape, const Matrix4x4d *transformationInverse);
};
#endif
