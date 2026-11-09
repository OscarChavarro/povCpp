//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_INTERSECTION_CURVE_BUILDER__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_INTERSECTION_CURVE_BUILDER__

#include "java/util/ArrayList.h"

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.h"

class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;

/**
Reconstructs the intersection curves of a boolean set operation from the
paired null-edge lists `sonea`/`soneb` produced by the classify stage
([MANT1988].15.6), before the connect stage ([MANT1988].15.7) consumes them.

The intersection of the boundaries of two closed 2-manifolds is a set of
closed space polylines. Between two consecutive intersection points the curve
runs along the intersection line of one specific pair (faceA, faceB), so two
paired null edges are candidate neighbors on a curve if and only if they
share at least one face of A and at least one face of B. When more than two
points lie on the same face pair, the chord structure is recovered by sorting
along the planes' intersection direction `dir = nA x nB` and pairing
entry/exit points by parity.

The resulting report orders each closed curve (cycle) as the connect stage
requires, and exposes structural anomalies: open chains, isolated nodes
(tangential grazing contacts) and pinch nodes (figure-8 cusps).

C++ counterpart of Java's `_PolyhedralBoundedSolidSetIntersectionCurveBuilder`.
*/
class _PolyhedralBoundedSolidSetIntersectionCurveBuilder {
public:
    typedef java::ArrayList<_PolyhedralBoundedSolidSetOperatorNullEdge> NullEdgeList;
    typedef java::ArrayList<int> IndexList;

    /**
    Result of `build`. Node indexes refer to positions in the index-aligned
    `sonea`/`soneb` lists given to `build`.
    */
    class Report {
    public:
        /** Closed curves; each list holds node indexes in traversal order. */
        java::ArrayList<IndexList> cycles;
        /** Open curve fragments (defect: the curve should close). */
        java::ArrayList<IndexList> openChains;
        /** Nodes with no curve neighbor (tangential strut candidates). */
        IndexList isolatedNodes;
        /** Nodes with more than two curve neighbors (cusp / figure-8). */
        IndexList pinchNodes;
        /** Face-pair groups with an odd point count (tangency on the pair). */
        int oddFacePairGroupCount;
        /** Face-pair groups whose planes were parallel or degenerate. */
        int degenerateDirectionGroupCount;
        /** Total number of null-edge pairs examined. */
        int nodeCount;

        Report();

        /**
        @return true when every node lies on a closed cycle and no structural
        anomaly was detected
        */
        bool isCleanlyClosed() const;

        /**
        @return one-line structural summary for pipeline traces
        */
        java::String summarize() const;
    };

    /**
    For each processing position of the most recent `orderAndOrientAlongCurves`
    result, the two processing positions of its cycle-adjacent pairs. Empty
    (Java null) until an order is computed.
    */
    static java::ArrayList<IndexList> lastTraversalNeighborPositions;

    /**
    Builds the intersection-curve report for the given index-aligned
    null-edge lists. The lists are not modified.
    @param sonea null edges on solid A, index-aligned with soneb
    @param soneb null edges on solid B, index-aligned with sonea
    @param unitVectorTolerance tolerance below which a cross product is
    considered degenerate (parallel face planes)
    @return structural report
    */
    static Report build(const NullEdgeList& sonea, const NullEdgeList& soneb,
        double unitVectorTolerance);

    /**
    Computes the curve-traversal processing order for the connect stage from
    a set of closed cycles ([MANT1988] section 15.7).
    @param cycles disjoint closed cycles in traversal order
    @param nodeCount total number of null-edge pairs
    @param outPermutation receives `result[position] = originalIndex`
    @return false (Java null) when the cycles do not cover the index range
    exactly
    */
    static bool computeTraversalOrder(const java::ArrayList<IndexList>& cycles,
        int nodeCount, IndexList& outPermutation);

    /**
    Computes the connect-stage processing order along the intersection curves
    and orients every strut consistently with that traversal, in one coherent
    operation. Each cycle direction is chosen by majority vote of the existing
    vertex-id orientations and the cycles are interleaved round-robin.
    @param cycles disjoint closed cycles (from `build`)
    @param nodeCount total number of null-edge pairs
    @param sonea null edges on solid A; struts may be flipped in place
    @param soneb null edges on solid B; struts may be flipped in place
    @param outPermutation receives `result[position] = originalIndex`
    @return false (Java null) when the cycles do not cover the index range
    exactly, in which case nothing is mutated
    */
    static bool orderAndOrientAlongCurves(const java::ArrayList<IndexList>& cycles,
        int nodeCount, NullEdgeList& sonea, NullEdgeList& soneb,
        IndexList& outPermutation);

private:
    _PolyhedralBoundedSolidSetIntersectionCurveBuilder();

    static _PolyhedralBoundedSolidFace* halfEdgeFace(
        _PolyhedralBoundedSolidHalfEdge* he);
    static void collectFaces(const _PolyhedralBoundedSolidSetOperatorNullEdge& ne,
        java::ArrayList<_PolyhedralBoundedSolidFace*>& outFaces);
    static bool nodePosition(const _PolyhedralBoundedSolidSetOperatorNullEdge& ne,
        Vector3Dd& outPosition);
    static bool faceNormal(_PolyhedralBoundedSolidFace* face,
        Vector3Dd& outNormal);
    static void link(java::ArrayList<IndexList>& neighbors, int i, int j);

    /**
    @return 1 (true) / 0 (false) for an unambiguous two-face strut whose half
    shared with `successor` is the right one; -1 (Java null) otherwise
    */
    static int successorSharedHalfIsRight(
        const _PolyhedralBoundedSolidSetOperatorNullEdge& current,
        const _PolyhedralBoundedSolidSetOperatorNullEdge& successor);
    static void orientTowardSuccessor(
        _PolyhedralBoundedSolidSetOperatorNullEdge& current,
        const _PolyhedralBoundedSolidSetOperatorNullEdge& successor,
        bool successorSideIsRight);
    static int minOf(const IndexList& values);
};

#endif
