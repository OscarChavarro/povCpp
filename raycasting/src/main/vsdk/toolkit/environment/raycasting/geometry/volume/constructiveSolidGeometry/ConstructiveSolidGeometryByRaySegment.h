#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY_BY_RAY_SEGMENT__
#define __CONSTRUCTIVE_SOLID_GEOMETRY_BY_RAY_SEGMENT__

#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.h"

/**
 * [ROTH1982] Scott D. Roth, "Ray Casting for Modeling Solids", CGIP 18,
 * 109-144 (1982), section 3.3: each child's ray crossings are classified
 * in/out and merged per the boolean operator (Figs. 8-9, Table 3).
 */
class ConstructiveSolidGeometryByRaySegment : public ConstructiveSolidGeometry {
  private:
    static CsgSegmentClassification<IntersectionCandidate> buildRaySegments(
        RayWithTracingState *ray, CsgOperand *child, Material *materialOverride);

    bool topLevel = false;

  public:
    // The steps of RAYCAST shared with the baked render path. The point whose
    // containment classifies the ray before an operand's first crossing in
    // its sorted crossings (or just past the origin when it has none).
    static Vector3Dd classificationPoint(
        const RayWithTracingState &ray, java::PriorityQueue<IntersectionCandidate> &crossings);
    // Drains the sorted crossings into alternating in/out segments; segments
    // are merged by the shared CsgSegmentClassification::merge.
    static CsgSegmentClassification<IntersectionCandidate> segmentsFromCrossings(
        java::PriorityQueue<IntersectionCandidate> &crossings, bool initialInside);

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
