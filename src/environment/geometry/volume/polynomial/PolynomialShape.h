#ifndef __POLYNOMIAL_SHAPE__
#define __POLYNOMIAL_SHAPE__

#include "environment/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"

// PovRay adapter of the VITRAL PolynomialSurface: it keeps the PovRay
// candidate emission, shadow-ray root threshold and inversion flag.
class PolynomialShape : public RayCastingGeometry {
  public:
    explicit PolynomialShape(int initialOrder);
    PolynomialShape(int initialOrder, int sturmFlagValue);
    PolynomialShape(const PolynomialShape &other);
    ~PolynomialShape() override;

    bool isInverted() const { return traceState.isInverted(); }
    int getOrder() const { return surface.getOrder(); }
    int getSturmFlag() const { return surface.usesSturmSequences() ? 1 : 0; }
    double *getCoeffs() { return surface.getCoefficients(); }
    const PolynomialSurface &getSurface() const { return surface; }
    // The VITRAL solid for typed queries; inversion is reported separately.
    const Geometry *getIntrinsicGeometry() const override { return &surface; }
    Geometry *getDetailSurface() override { return &surface; }
    SurfaceDetailFamily getDetailFamily() const override { return SurfaceDetailFamily::Polynomial; }
    SurfaceTraceState *getTraceState() override { return &traceState; }
    // Typed ray classification of the VITRAL solid, complemented when inverted.
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;

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

  private:
    PolynomialSurface surface;
    SurfaceTraceState traceState;
};

#endif
