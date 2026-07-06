#ifndef __POLYNOMIAL_SHAPE__
#define __POLYNOMIAL_SHAPE__

#include "environment/geometry/Geometry.h"

class PolynomialShape : public Geometry {
  public:
    explicit PolynomialShape(int initialOrder);
    PolynomialShape(int initialOrder, int sturmFlagValue);
    PolynomialShape(const PolynomialShape &other);
    ~PolynomialShape() override;

    bool isInverted() const { return inverted; }
    bool hasInternalTransformation() const { return transformation != nullptr; }
    int getOrder() const { return order; }
    int getSturmFlag() const { return sturmFlag; }
    double *getCoeffs() const { return Coeffs; }

    static const int *termCountsByOrder();

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    PolynomialShape *copyWithSturmFlag(int flag) const;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;

  private:
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    bool inverted;
    const int order;
    const int sturmFlag;
    double *const Coeffs;

    static void transform(int order, double *coeffs, Matrix4x4d *q);
    static int roll(int order, int x, int y, int z);
    static void unroll(int order, int index, int *x, int *y, int *z, int *w);
    static int intersect(const RayWithTracingState *ray, int order,
        const double *coeffs, double *depths);
    static int intersectQuartic(const RayWithTracingState *ray,
        const PolynomialShape *shape, double *depths);
    static void quarticNormal(Vector3Dd *result, const PolynomialShape *shape,
        const Vector3Dd *intersectionPoint);
    static double evaluatePolynomial(
        const Vector3Dd *point, int order, const double *coeffs);
    static void normalp(Vector3Dd *result, int order, const double *coeffs,
        const Vector3Dd *intersectionPoint);
    static double doPartialTerm(
        const Matrix4x4d *q, int row, int pwr, int i, int j, int k, int l);
};

#endif
