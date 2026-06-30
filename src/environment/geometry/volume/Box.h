#ifndef __BOX__
#define __BOX__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"

class Box : public Geometry {
  public:
    Box();
    Box(const Vector3Dd &minBounds, const Vector3Dd &maxBounds,
        bool inverted = false);
    Box(const Box &other);
    ~Box() override;

    Vector3Dd* getBounds() { return bounds; }
    const Vector3Dd* getBounds() const { return bounds; }
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }

    static int intersectBoxx(const RayWithSegments *ray, const Box *box,
        double *depth1, double *depth2);

    AxisAlignedBox getMinMax() const override;

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    bool doIntersectionFirstHitNoQueue(
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void invertGeometry() override;

  private:
    Vector3Dd bounds[2];
    bool inverted;

    static int closeTo(double x, double y);
};

#endif
