#ifndef __BOX__
#define __BOX__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"

class Box : public TransformedGeometry {
  public:
    Box();
    Box(const Vector3Dd &minBounds, const Vector3Dd &maxBounds,
        bool inverted = false);
    Box(Matrix4x4d *transformation, Matrix4x4d *transformationInverse,
        const Vector3Dd &minBounds, const Vector3Dd &maxBounds, bool inverted);
    Box(const Box &other);
    ~Box() override;

    Vector3Dd* getBounds() { return bounds; }
    const Vector3Dd* getBounds() const { return bounds; }
    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }

    static int intersectBoxx(const RayWithSegments *ray, const Box *box,
        double *depth1, double *depth2);

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
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
