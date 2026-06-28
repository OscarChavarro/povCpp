#ifndef __SPHERE__
#define __SPHERE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

// Canonical unit sphere centered at the origin. Placement and scaling are
// owned by the caller (SimpleBody/CsgOperand), which transforms the ray into
// local space before invoking this geometry.
class Sphere : public Geometry {
  private:
    bool inverted;

    static bool intersectSphere(const RayWithSegments *ray, const Sphere *sphere,
        double *depth1, double *depth2);

  public:
    Sphere();
    explicit Sphere(bool inverted);
    Sphere(const Sphere &other);

    bool isInverted() const { return inverted; }
    void toggleInverted() { inverted ^= true; }

    AxisAlignedBox getMinMax() const override;

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void invertGeometry() override;
};

#endif
