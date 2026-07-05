#ifndef __SPHERE__
#define __SPHERE__

#include "environment/geometry/Geometry.h"

class GeometryStatistics;

// Sphere centered at the origin in its own local space, with an intrinsic
// radius (default 1.0). Rotation/translation/skew on top of that are still
// owned by the caller (SimpleBody/CsgOperand), which transforms the ray into
// local space before invoking this geometry; only the size is native here.
class Sphere : public Geometry {
  private:
    double radius;
    bool inverted;

    static bool intersectSphere(const RayWithTracingState *ray, const Sphere *sphere,
        double *depth1, double *depth2);

  public:
    static bool intersectSphereLocalSpace(
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        GeometryStatistics *stats,
        double radius,
        double *depth1,
        double *depth2);

  public:
    Sphere();
    explicit Sphere(bool inverted);
    explicit Sphere(double radius);
    Sphere(double radius, bool inverted);
    Sphere(const Sphere &other);

    double getRadius() const { return radius; }
    void setRadius(double newRadius) { radius = newRadius; }

    bool isInverted() const { return inverted; }
    void toggleInverted() { inverted ^= true; }

    AxisAlignedBoundingBox getMinMax() const override;

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    bool doIntersectionFirstHit(
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
};

#endif
