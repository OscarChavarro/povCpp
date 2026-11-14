#ifndef __POLYNOMIAL_SHAPE__
#define __POLYNOMIAL_SHAPE__

#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"

// PovRay adapter of the VITRAL PolynomialSurface: it keeps the PovRay
// candidate emission, shadow-ray root threshold and inversion flag.
class PolynomialShape : public RayCastingGeometry {
  public:
    explicit PolynomialShape(int initialOrder);
    PolynomialShape(int initialOrder, int sturmFlagValue);
    PolynomialShape(const PolynomialShape &other);
    ~PolynomialShape() override;

    bool isInverted() const { return inverted; }
    int getOrder() const { return surface.getOrder(); }
    int getSturmFlag() const { return surface.usesSturmSequences() ? 1 : 0; }
    double *getCoeffs() { return surface.getCoefficients(); }
    const PolynomialSurface &getSurface() const { return surface; }

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
    PolynomialSurface surface;
    bool inverted;

    int traceCrossings(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride, const GeometryIntersectionEmissionContext *context);
};

#endif
