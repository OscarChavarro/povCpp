#ifndef __POLYNOMIAL_SHAPE__
#define __POLYNOMIAL_SHAPE__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/Geometry.h"

class PolynomialShape : public Geometry {
  public:
    explicit PolynomialShape(int initialOrder);
    PolynomialShape(int initialOrder, int sturmFlagValue);
    PolynomialShape(const PolynomialShape &other);
    ~PolynomialShape() override;

    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }
    bool isInverted() const { return inverted; }
    int getOrder() const { return order; }
    int getSturmFlag() const { return sturmFlag; }
    double *getCoeffs() const { return Coeffs; }

    static const int *termCountsByOrder();

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    PolynomialShape *copyWithSturmFlag(int flag) const;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    bool inverted;
    const int order;
    const int sturmFlag;
    double *const Coeffs;

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
