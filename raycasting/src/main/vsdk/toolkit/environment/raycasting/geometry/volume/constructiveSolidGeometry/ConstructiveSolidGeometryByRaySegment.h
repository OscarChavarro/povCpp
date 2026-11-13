#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY_BY_RAY_SEGMENT__
#define __CONSTRUCTIVE_SOLID_GEOMETRY_BY_RAY_SEGMENT__

#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/RaySegments.h"

/**
 * [ROTH1982] Scott D. Roth, "Ray Casting for Modeling Solids", CGIP 18,
 * 109-144 (1982), section 3.3: each child's ray crossings are classified
 * in/out and merged per the boolean operator (Figs. 8-9, Table 3).
 */
class ConstructiveSolidGeometryByRaySegment : public ConstructiveSolidGeometry {
  private:
    static RaySegments buildRaySegments(
        RayWithTracingState *ray, CsgOperand *child, Material *materialOverride);
    static RaySegments mergeUnion(const RaySegments &left, const RaySegments &right);
    static RaySegments mergeIntersection(const RaySegments &left, const RaySegments &right);
    static RaySegments mergeDifference(const RaySegments &left, const RaySegments &right);
    static RaySegments mergeByMembership(
        const RaySegments &left,
        const RaySegments &right,
        BooleanSetOperations operation);

    bool topLevel = false;

  public:
    explicit ConstructiveSolidGeometryByRaySegment(
        BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ConstructiveSolidGeometryByRaySegment(const ConstructiveSolidGeometryByRaySegment &other);

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

    void setTopLevel(bool value) { topLevel = value; }
    bool isTopLevel() const { return topLevel; }
};

#endif
