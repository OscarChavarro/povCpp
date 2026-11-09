//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_NULL_EDGES_CONNECTOR__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_NULL_EDGES_CONNECTOR__

#include "java/util/ArrayList.h"
#include "java/util/TreeMap.h"

#include "java/lang/String.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersectionCurveBuilder.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.h"

class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;

/**
Connect stage (big phase 3) for set operations: null-edges pairing and joins,
following section [MANT1988].15.7 and programs [MANT1988].15.13 and
[MANT1988].15.14.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetNullEdgesConnector`.
The connector reorders the given `sonea`/`soneb` lists in place, as the Java
version does.
*/
class _PolyhedralBoundedSolidSetNullEdgesConnector
    : public _PolyhedralBoundedSolidOperator {
public:
    typedef java::ArrayList<_PolyhedralBoundedSolidSetOperatorNullEdge> NullEdgeList;
    typedef java::ArrayList<_PolyhedralBoundedSolidFace*> FaceList;
    typedef java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> HalfEdgeList;

    class ConnectResult {
    public:
        ConnectResult(const FaceList& sonfa, const FaceList& sonfb);
        const FaceList& sonfa() const;
        const FaceList& sonfb() const;
    private:
        FaceList sonfaValue;
        FaceList sonfbValue;
    };

    /**
    Structural report of the intersection curves reconstructed from the
    sonea/soneb pairs of the most recent `connect` call.
    */
    static _PolyhedralBoundedSolidSetIntersectionCurveBuilder::Report
        lastCurveReport;

    /**
    Test-only injection point: when `hasTestOnlyForcedConnectOrder` is set and
    the permutation matches the pair count, `sortNullEdges` reorders
    sonea/soneb by this permutation (entry `position -> originalIndex`)
    instead of the production ordering. Always unset in production.
    */
    static java::ArrayList<int> testOnlyForcedConnectOrder;
    static bool hasTestOnlyForcedConnectOrder;

    _PolyhedralBoundedSolidSetNullEdgesConnector();

    /**
    Neighbor null edges connector for the set operations algorithm
    (big phase 3), following section [MANT1988].15.7 and program
    [MANT1988].15.14.
    @param op UNION, INTERSECTION or SUBTRACT
    @param flags debug flags
    @param inSonea null edges of A, reordered in place
    @param inSoneb null edges of B, reordered in place
    @return the faces cut from each solid (sonfa, sonfb)
    */
    ConnectResult connect(int op, int flags, NullEdgeList& inSonea,
        NullEdgeList& inSoneb);

    static int getSonfaPairIndex(_PolyhedralBoundedSolidFace* face);
    static int getSonfbPairIndex(_PolyhedralBoundedSolidFace* face);
    static int getLastLooseACount();
    static int getLastLooseBCount();
    static int getLastSonfaCount();
    static int getLastSonfbCount();
    static int getLastPairCount();

private:
    /**
    Mutable carrier for the (nea, neb) pair returned by `sgetnextnulledge`,
    mirroring the out-param style used by [MANT1988] Program 15.14 in C.
    */
    struct NullEdgePair {
        _PolyhedralBoundedSolidSetOperatorNullEdge* nea;
        _PolyhedralBoundedSolidSetOperatorNullEdge* neb;
        int pairIndex;
    };

    static const int DEBUG_01_STRUCTURE = 0x01;
    static const int DEBUG_05_CONNECT = 0x10;
    static const int DEBUG_99_SHOWOPERATIONS = 0x40;
    static const int ENDPOINT_SOLID_A = 0;
    static const int ENDPOINT_SOLID_B = 1;

    static int debugFlags;
    static int operation;
    int nextNullEdgeIndex;
    NullEdgeList* sonea;
    NullEdgeList* soneb;
    static HalfEdgeList endsa;
    static HalfEdgeList endsb;
    FaceList sonfa;
    FaceList sonfb;
    static java::TreeMap<int, int> sonfaPairIndexByFaceId;
    static java::TreeMap<int, int> sonfbPairIndexByFaceId;
    static bool hasPairIndexMaps;
    static int lastLooseACount;
    static int lastLooseBCount;
    static int lastSonfaCount;
    static int lastSonfbCount;
    static int lastPairCount;
    static int currentConnectPairIndex;
    static int nextSyntheticPairIndex;

    /**
    True when `sortNullEdges` already oriented every strut along the
    intersection curves, so the connect loop must respect that orientation
    instead of re-normalizing edge halves by vertex-id comparison.
    */
    static bool curveOrientationApplied;

    /**
    Junction adjacency in processing-position space when the curve order is
    active; empty (Java null) otherwise.
    */
    static java::ArrayList<java::ArrayList<int> > curveNeighborPositions;

    /**
    Processing pair index that pushed each loose entry, parallel to
    `endsa`/`endsb`.
    */
    static java::ArrayList<int> endsPairIndex;

    static bool isPipelineSummaryTraceEnabled();
    static bool isKeepInsertionOrderEnabled();
    static void tracePipelineSummary(const java::String& message);
    static java::String summarizeHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    static java::String summarizeNullEdge(
        const _PolyhedralBoundedSolidSetOperatorNullEdge& edge);
    static void setCurrentConnectContext(int pairIndex);
    static int allocateSyntheticPairIndex();
    static java::String summarizeLooseEnds(const HalfEdgeList& endsA,
        const HalfEdgeList& endsB);
    static bool isSamePoint(_PolyhedralBoundedSolidHalfEdge* first,
        _PolyhedralBoundedSolidHalfEdge* second);
    static bool canCutCoincidentHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    static bool hasReusableCoincidentCutFace(_PolyhedralBoundedSolidHalfEdge* he);
    static bool canCutCoincidentFinishFace(_PolyhedralBoundedSolidHalfEdge* he);
    static _PolyhedralBoundedSolidFace* registerCoincidentCutFace(
        _PolyhedralBoundedSolidHalfEdge* he, FaceList& target,
        const char* label);
    static bool canFinalizeCoincidentLooseA(_PolyhedralBoundedSolidHalfEdge* he);
    static bool canFinalizeCoincidentLooseB(_PolyhedralBoundedSolidHalfEdge* he);
    static bool isPointLikeHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    static bool isDegenerateCoincidentLooseClosure(
        _PolyhedralBoundedSolidHalfEdge* firstA,
        _PolyhedralBoundedSolidHalfEdge* secondA,
        _PolyhedralBoundedSolidHalfEdge* firstB,
        _PolyhedralBoundedSolidHalfEdge* secondB);
    void finalizeCoincidentLooseA(_PolyhedralBoundedSolidHalfEdge* he);
    void finalizeCoincidentLooseB(_PolyhedralBoundedSolidHalfEdge* he);
    static void removeLoosePair(int index);
    void updateLastSnapshot();
    static bool isLiveHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    static bool sharesParentFace(_PolyhedralBoundedSolidHalfEdge* first,
        _PolyhedralBoundedSolidHalfEdge* second);
    static bool isOppositeHalfEdgeSide(_PolyhedralBoundedSolidHalfEdge* first,
        _PolyhedralBoundedSolidHalfEdge* second);
    static _PolyhedralBoundedSolidFace* findUniqueClassicRebindTargetFace(
        _PolyhedralBoundedSolidHalfEdge* currentTargetFirst,
        _PolyhedralBoundedSolidHalfEdge* currentTargetSecond,
        _PolyhedralBoundedSolidHalfEdge* currentReferenceFirst,
        _PolyhedralBoundedSolidHalfEdge* currentReferenceSecond,
        const HalfEdgeList& targetEnds,
        const HalfEdgeList& referenceEnds);
    static bool isStrictRebindTargetFace(_PolyhedralBoundedSolidFace* targetFace,
        _PolyhedralBoundedSolidHalfEdge* currentTarget);
    static void rebindClassicCurrentNullEdgeIfNeeded(
        _PolyhedralBoundedSolidHalfEdge* currentTargetFirst,
        _PolyhedralBoundedSolidHalfEdge* currentTargetSecond,
        _PolyhedralBoundedSolidHalfEdge* currentReferenceFirst,
        _PolyhedralBoundedSolidHalfEdge* currentReferenceSecond,
        const HalfEdgeList& targetEnds,
        const HalfEdgeList& referenceEnds,
        const char* label);
    static void rebindClassicCurrentNullEdgesIfNeeded(
        _PolyhedralBoundedSolidHalfEdge* currentARight,
        _PolyhedralBoundedSolidHalfEdge* currentALeft,
        _PolyhedralBoundedSolidHalfEdge* currentBLeft,
        _PolyhedralBoundedSolidHalfEdge* currentBRight);
    void sortNullEdges();
    static void dbgDumpNullEdges(const char* label, const NullEdgeList& sone);
    void applyOrderPermutation(const java::ArrayList<int>& order);
    static int curveComponentFind(java::ArrayList<int>& parent, int x);
    void groupNullEdgesByRing();
    bool sgetnextnulledge(NullEdgePair& out);
    bool scanjoin(_PolyhedralBoundedSolidHalfEdge* hea,
        _PolyhedralBoundedSolidHalfEdge* heb,
        _PolyhedralBoundedSolidHalfEdge** outA,
        _PolyhedralBoundedSolidHalfEdge** outB);
    static bool rolesOpposite(_PolyhedralBoundedSolidHalfEdge* h1,
        _PolyhedralBoundedSolidHalfEdge* h2);
    static bool isPendingStrutRing(_PolyhedralBoundedSolidHalfEdge* he);
    bool rescueRingFaceNearMiss(_PolyhedralBoundedSolidHalfEdge* hea,
        _PolyhedralBoundedSolidHalfEdge* heb,
        _PolyhedralBoundedSolidHalfEdge** outA,
        _PolyhedralBoundedSolidHalfEdge** outB);
    static bool isLooseA(_PolyhedralBoundedSolidHalfEdge* he);
    static bool isLooseB(_PolyhedralBoundedSolidHalfEdge* he);
    _PolyhedralBoundedSolidFace* cutA(_PolyhedralBoundedSolidHalfEdge* he);
    _PolyhedralBoundedSolidFace* cutB(_PolyhedralBoundedSolidHalfEdge* he);
    static void removeLastCutFaceIfSame(FaceList& faces,
        _PolyhedralBoundedSolidFace* face);
    void setOpConnect();
};

#endif
