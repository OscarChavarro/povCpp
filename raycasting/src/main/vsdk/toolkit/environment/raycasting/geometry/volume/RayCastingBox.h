#ifndef __POV_BOX__
#define __POV_BOX__

#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"

class RayCastingBox : public RayCastingGeometry {
  public:
    RayCastingBox();
    RayCastingBox(const Vector3Dd &minBounds, const Vector3Dd &maxBounds,
        bool inverted = false);
    RayCastingBox(const RayCastingBox &other);
    ~RayCastingBox() override;

    Vector3Dd* getBounds() { return box.getRawBounds(); }
    const Vector3Dd* getBounds() const { return box.getRawBounds(); }
    const Geometry* getIntrinsicGeometry() const override { return &box; }
    Geometry* copyGeometry() const override { return box.copyGeometry(); }
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }

    static int intersectBoxx(const RayWithTracingState *ray, const RayCastingBox *box,
        double *depth1, double *depth2);

    void getBounds(GeometryBounds& result) const override;
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
    Box box;
    bool inverted;

};

#endif
