#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY_BY_MORGAN_RULES__
#define __CONSTRUCTIVE_SOLID_GEOMETRY_BY_MORGAN_RULES__

#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

class ConstructiveSolidGeometryByMorganRules : public ConstructiveSolidGeometry {
  private:
    static int insideCsgChild(Vector3Dd *point, TransformedGeometry *shape);
    int allCsgUnionIntersections(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);
    int allCsgIntersectIntersections(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);
    int insideCsgUnion(Vector3Dd *point);
    int insideCsgIntersection(Vector3Dd *point);

  public:
    explicit ConstructiveSolidGeometryByMorganRules(
        BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ConstructiveSolidGeometryByMorganRules(const ConstructiveSolidGeometryByMorganRules &other);

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;
};

#endif
