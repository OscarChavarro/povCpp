#ifndef __POV_BOX__
#define __POV_BOX__

#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"

class RayCastingBox : public RayCastingGeometry {
  public:
    RayCastingBox();
    RayCastingBox(const Vector3Dd &minBounds, const Vector3Dd &maxBounds,
        bool inverted = false);
    RayCastingBox(const RayCastingBox &other);
    ~RayCastingBox() override;

    Vector3Dd* getBounds() { return bounds; }
    const Vector3Dd* getBounds() const { return bounds; }
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }

    static int intersectBoxx(const RayWithTracingState *ray, const RayCastingBox *box,
        double *depth1, double *depth2);

    BoundingVolumeHierarchy *createBoundingVolume() const override;

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

  private:
    Vector3Dd bounds[2];
    bool inverted;

    static int closeTo(double x, double y);
};

#endif
