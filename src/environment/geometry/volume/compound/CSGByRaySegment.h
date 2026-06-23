#ifndef __CSG_BY_RAY_SEGMENT__
#define __CSG_BY_RAY_SEGMENT__

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/RaySegments.h"

/**
 * CSG evaluated by boolean ray-segment classification, in the style of
 * [ROTH1982] Scott D. Roth, "Ray Casting for Modeling Solids", Computer
 * Graphics and Image Processing 18, 109-144 (1982): each child contributes
 * an ordered list of ray crossings with in/out classification
 * ([ROTH1982].3.3, "In-Out Classification"), and the boolean operator is
 * applied as the three-step merge/classify/simplify process of
 * [ROTH1982].3.3 ("Combining Left and Right Classifications", Figs. 8-9,
 * Table 3), rather than by testing point membership of surface hits against
 * sibling solids (CSG's approach). Selected at parse time by
 * ParserContext::usesCsgRoth(); see doc/CSGByRaySegments.md. DIFFERENCE is
 * first-class here (no invert()).
 */
class CSGByRaySegment : public CSG {
  private:
    static RaySegments buildRaySegments(RayWithSegments *ray, TransformableElement *child);
    static RaySegments mergeUnion(const RaySegments &left, const RaySegments &right);
    static RaySegments mergeIntersection(const RaySegments &left, const RaySegments &right);
    static RaySegments mergeDifference(const RaySegments &left, const RaySegments &right);
    static RaySegments mergeByMembership(
        const RaySegments &left,
        const RaySegments &right,
        bool (*combine)(bool insideLeft, bool insideRight));

  private:
    // Set by CsgParser only for a union/intersection/difference parsed
    // directly from an `object { ... }` body or a top-level `#declare`, never
    // for one nested inside another CSG block's body (see CsgParser::parse's
    // isNested parameter). allIntersections() uses it, together with
    // isUnionOfBarePlanes, to gate the legacy concatenation fallback for
    // POV's classic "infinite half-space planes unioned into a mirror
    // corner" idiom (doc comment on isUnionOfBarePlanes in the .cpp): that
    // fallback offers raw, possibly non-alternating crossings, which is only
    // safe when nothing above this node depends on its in/out classification
    // (true for a `object { union { plane plane } }` mirror backdrop, false
    // for a union of planes nested inside an outer intersection/difference,
    // e.g. chess.pov's notch-carving sub-unions).
    bool topLevel = false;

  public:
    explicit CSGByRaySegment(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    CSGByRaySegment(const CSGByRaySegment &other);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

    void setTopLevel(bool value) { topLevel = value; }
    bool isTopLevel() const { return topLevel; }
};

#endif
