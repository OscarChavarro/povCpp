#ifndef __POLY_H__
#define __POLY_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/Geometry.h"

class PolynomialShape : public Geometry {
  public:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool inverted;
    int order;
    int sturmFlag;
    double *Coeffs;

    int getSturmFlag() const { return sturmFlag; }
    void setSturmFlag(int flag) { sturmFlag = flag; }

    static const int *termCountsByOrder();

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    static void transform(int order, double *coeffs, Matrix4x4d *q);
    static int roll(int order, int x, int y, int z);
    static void unroll(int order, int index, int *x, int *y, int *z, int *w);
    static int intersect(const RayWithSegments *ray, int order,
        const double *coeffs, double *depths);
    static int intersectQuartic(const RayWithSegments *ray,
        const PolynomialShape *shape, double *depths);
    static void quarticNormal(Vector3Dd *result, BoundedGeometry *object,
        const Vector3Dd *intersectionPoint);
    static double evaluatePolynomial(
        const Vector3Dd *point, int order, const double *coeffs);
    static void normalp(Vector3Dd *result, int order, const double *coeffs,
        const Vector3Dd *intersectionPoint);
    static double doPartialTerm(
        const Matrix4x4d *q, int row, int pwr, int i, int j, int k, int l);
};

#endif
