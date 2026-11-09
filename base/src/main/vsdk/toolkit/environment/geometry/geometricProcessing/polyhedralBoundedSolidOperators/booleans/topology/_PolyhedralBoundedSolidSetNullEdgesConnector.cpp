//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "java/util/Collections.h"
#include "java/lang/Math.h"
#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include "java/util/TreeSet.txx"
#include <cmath>
#include <cstdio>
#include <strings.h>

#include "java/lang/Boolean.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetGeometricPredicateProcessor.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetNullEdgesConnector.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetNullEdgesConnector Connector;

namespace {

const char* const TRACE_PIPELINE_SUMMARY_PROPERTY =
    "vsdk.setop.tracePipelineSummary";
const char* const KEEP_INSERTION_ORDER_PROPERTY =
    "vsdk.setop.connect.keepInsertionOrder";

java::String str(int value)
{
    return java::String::valueOf(value);
}

java::String positionString(const Vector3Dd& p)
{
    java::String* text = p.toString();
    java::String result(text->c_str());
    delete text;
    return result;
}

}

int Connector::debugFlags = 0;
int Connector::operation = 0;
Connector::HalfEdgeList Connector::endsa;
Connector::HalfEdgeList Connector::endsb;
java::TreeMap<int, int> Connector::sonfaPairIndexByFaceId;
java::TreeMap<int, int> Connector::sonfbPairIndexByFaceId;
bool Connector::hasPairIndexMaps = false;
int Connector::lastLooseACount = 0;
int Connector::lastLooseBCount = 0;
int Connector::lastSonfaCount = 0;
int Connector::lastSonfbCount = 0;
int Connector::lastPairCount = 0;
int Connector::currentConnectPairIndex = 0;
int Connector::nextSyntheticPairIndex = 0;
_PolyhedralBoundedSolidSetIntersectionCurveBuilder::Report Connector::lastCurveReport;
java::ArrayList<int> Connector::testOnlyForcedConnectOrder;
bool Connector::hasTestOnlyForcedConnectOrder = false;
bool Connector::curveOrientationApplied = false;
java::ArrayList<java::ArrayList<int> > Connector::curveNeighborPositions;
java::ArrayList<int> Connector::endsPairIndex;

//= Results =========================================================

Connector::ConnectResult::ConnectResult(const FaceList& sonfa,
    const FaceList& sonfb)
    : sonfaValue(sonfa), sonfbValue(sonfb)
{
}

const Connector::FaceList& Connector::ConnectResult::sonfa() const
{
    return sonfaValue;
}

const Connector::FaceList& Connector::ConnectResult::sonfb() const
{
    return sonfbValue;
}

Connector::_PolyhedralBoundedSolidSetNullEdgesConnector()
    : nextNullEdgeIndex(0), sonea(0), soneb(0)
{
}

//= Traces and summaries ============================================

bool Connector::isPipelineSummaryTraceEnabled()
{
    return java::Boolean::getBoolean(TRACE_PIPELINE_SUMMARY_PROPERTY);
}

bool Connector::isKeepInsertionOrderEnabled()
{
    // As Java: an unset property means true, otherwise `Boolean.parseBoolean`
    java::String propertyValue = java::System::getProperty(
        KEEP_INSERTION_ORDER_PROPERTY, "true");
    return strcasecmp(propertyValue.c_str(), "true") == 0;
}

void Connector::tracePipelineSummary(const java::String& message)
{
    if ( !isPipelineSummaryTraceEnabled() ) {
        return;
    }
    printf("[SetOpPipelineTrace] %s\n", message.c_str());
}

java::String Connector::summarizeHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == 0 ) {
        return "null";
    }

    java::String from = "?";
    java::String to = "?";
    java::String face = "?";

    if ( he->startingVertex != 0 ) {
        from = str(he->startingVertex->id);
    }
    if ( he->parentLoop != 0 ) {
        _PolyhedralBoundedSolidHalfEdge* next = he->next();
        if ( next != 0 && next->startingVertex != 0 ) {
            to = str(next->startingVertex->id);
        }
        if ( he->parentLoop->parentFace != 0 ) {
            face = str(he->parentLoop->parentFace->id);
        }
    }
    java::String fromPoint = "?";
    java::String toPoint = "?";

    if ( he->startingVertex != 0 ) {
        fromPoint = positionString(he->startingVertex->position);
    }
    if ( he->parentLoop != 0 ) {
        _PolyhedralBoundedSolidHalfEdge* next = he->next();
        if ( next != 0 && next->startingVertex != 0 ) {
            toPoint = positionString(next->startingVertex->position);
        }
    }
    java::String text = "he(v=" + from + "->" + to + ",f=" + face +
        ",p=" + fromPoint + "->" + toPoint + ")";
    return java::String(text.c_str());
}

java::String Connector::summarizeNullEdge(
    const _PolyhedralBoundedSolidSetOperatorNullEdge& edge)
{
    if ( edge.e == 0 ) {
        return "null";
    }
    return summarizeHalfEdge(edge.e->rightHalf) + " | " +
        summarizeHalfEdge(edge.e->leftHalf);
}

void Connector::setCurrentConnectContext(int pairIndex)
{
    currentConnectPairIndex = pairIndex;
}

int Connector::allocateSyntheticPairIndex()
{
    int pairIndex;

    pairIndex = nextSyntheticPairIndex;
    nextSyntheticPairIndex++;
    return pairIndex;
}

int Connector::getSonfaPairIndex(_PolyhedralBoundedSolidFace* face)
{
    if ( face == 0 || !hasPairIndexMaps ) {
        return -1;
    }
    return sonfaPairIndexByFaceId.getOrDefault(face->id, -1);
}

int Connector::getSonfbPairIndex(_PolyhedralBoundedSolidFace* face)
{
    if ( face == 0 || !hasPairIndexMaps ) {
        return -1;
    }
    return sonfbPairIndexByFaceId.getOrDefault(face->id, -1);
}

java::String Connector::summarizeLooseEnds(const HalfEdgeList& endsA,
    const HalfEdgeList& endsB)
{
    java::String out;
    size_t i;

    out += ("pairs=" + str((int)endsA.size()) + " [").c_str();
    for ( i = 0; i < endsA.size() && i < endsB.size(); i++ ) {
        if ( i > 0 ) {
            out += " | ";
        }
        out += (str((int)i) + ":A=").c_str();
        out += summarizeHalfEdge(endsA[i]);
        out += ",B=";
        out += summarizeHalfEdge(endsB[i]);
    }
    out += "]";
    return out;
}

//= Coincidence helpers =============================================

bool Connector::isSamePoint(_PolyhedralBoundedSolidHalfEdge* first,
    _PolyhedralBoundedSolidHalfEdge* second)
{
    if ( first == 0 || second == 0 ||
         first->startingVertex == 0 || second->startingVertex == 0 ) {
        return false;
    }
    return PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
        first->startingVertex->position,
        second->startingVertex->position,
        numericContext);
}

bool Connector::canCutCoincidentHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    _PolyhedralBoundedSolidEdge* edge;
    _PolyhedralBoundedSolidLoop* loop;

    if ( he == 0 ) {
        return false;
    }
    edge = he->parentEdge;
    loop = he->parentLoop;
    if ( edge == 0 || loop == 0 ) {
        return false;
    }
    if ( edge->rightHalf == 0 || edge->leftHalf == 0 ) {
        return false;
    }
    if ( edge->rightHalf->parentLoop != edge->leftHalf->parentLoop ) {
        return true;
    }
    return loop->halfEdgesList.size() > 2;
}

bool Connector::hasReusableCoincidentCutFace(_PolyhedralBoundedSolidHalfEdge* he)
{
    _PolyhedralBoundedSolidFace* face;

    if ( he == 0 || he->parentLoop == 0 ) {
        return false;
    }
    face = he->parentLoop->parentFace;
    return face != 0 && face->boundariesList.size() > 1;
}

bool Connector::canCutCoincidentFinishFace(_PolyhedralBoundedSolidHalfEdge* he)
{
    _PolyhedralBoundedSolidEdge* edge;
    _PolyhedralBoundedSolidLoop* loop;

    if ( he == 0 ) {
        return false;
    }
    edge = he->parentEdge;
    loop = he->parentLoop;
    if ( edge == 0 || loop == 0 ||
         edge->rightHalf == 0 || edge->leftHalf == 0 ||
         edge->rightHalf->parentLoop == 0 ||
         edge->leftHalf->parentLoop == 0 ) {
        return false;
    }
    // Classic case: both halves in the same loop and loop large enough for
    // an interior cut.
    if ( edge->rightHalf->parentLoop == edge->leftHalf->parentLoop ) {
        return loop->halfEdgesList.size() > 2;
    }
    // Cross-loop intersection edge whose halves still share the same face
    // (typical for null-edges produced by the intersect+classify pipeline
    // on tessellated curved surfaces).
    return edge->rightHalf->parentLoop->parentFace ==
           edge->leftHalf->parentLoop->parentFace;
}

_PolyhedralBoundedSolidFace* Connector::registerCoincidentCutFace(
    _PolyhedralBoundedSolidHalfEdge* he,
    FaceList& target,
    const char* label)
{
    _PolyhedralBoundedSolidFace* face;

    (void)label;
    if ( he == 0 || he->parentLoop == 0 ) {
        return 0;
    }
    face = he->parentLoop->parentFace;
    if ( face == 0 || face->boundariesList.size() <= 1 ) {
        return 0;
    }
    target.add(face);
    return face;
}

bool Connector::canFinalizeCoincidentLooseA(_PolyhedralBoundedSolidHalfEdge* he)
{
    return canCutCoincidentFinishFace(he) || hasReusableCoincidentCutFace(he);
}

bool Connector::canFinalizeCoincidentLooseB(_PolyhedralBoundedSolidHalfEdge* he)
{
    return canCutCoincidentFinishFace(he) || hasReusableCoincidentCutFace(he);
}

bool Connector::isPointLikeHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    return isLiveHalfEdge(he) &&
        he->next() != 0 &&
        he->next()->startingVertex != 0 &&
        isSamePoint(he, he->next());
}

bool Connector::isDegenerateCoincidentLooseClosure(
    _PolyhedralBoundedSolidHalfEdge* firstA,
    _PolyhedralBoundedSolidHalfEdge* secondA,
    _PolyhedralBoundedSolidHalfEdge* firstB,
    _PolyhedralBoundedSolidHalfEdge* secondB)
{
    return isPointLikeHalfEdge(firstA) &&
        isPointLikeHalfEdge(secondA) &&
        isPointLikeHalfEdge(firstB) &&
        isPointLikeHalfEdge(secondB);
}

void Connector::finalizeCoincidentLooseA(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( canCutCoincidentFinishFace(he) ) {
        cutA(he);
    }
    else {
        registerCoincidentCutFace(he, sonfa, "reuse-sonfa");
    }
}

void Connector::finalizeCoincidentLooseB(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( canCutCoincidentFinishFace(he) ) {
        cutB(he);
    }
    else {
        registerCoincidentCutFace(he, sonfb, "reuse-sonfb");
    }
}

void Connector::removeLoosePair(int index)
{
    endsa.remove(index);
    endsb.remove(index);
}

int Connector::getLastLooseACount()
{
    return lastLooseACount;
}

int Connector::getLastLooseBCount()
{
    return lastLooseBCount;
}

int Connector::getLastSonfaCount()
{
    return lastSonfaCount;
}

int Connector::getLastSonfbCount()
{
    return lastSonfbCount;
}

int Connector::getLastPairCount()
{
    return lastPairCount;
}

void Connector::updateLastSnapshot()
{
    lastLooseACount = (int)endsa.size();
    lastLooseBCount = (int)endsb.size();
    lastSonfaCount = (int)sonfa.size();
    lastSonfbCount = (int)sonfb.size();
}

bool Connector::isLiveHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    return he != 0 &&
        he->parentEdge != 0 &&
        he->parentLoop != 0 &&
        he->parentLoop->parentFace != 0;
}

bool Connector::sharesParentFace(_PolyhedralBoundedSolidHalfEdge* first,
    _PolyhedralBoundedSolidHalfEdge* second)
{
    return isLiveHalfEdge(first) &&
        isLiveHalfEdge(second) &&
        first->parentLoop->parentFace == second->parentLoop->parentFace;
}

bool Connector::isOppositeHalfEdgeSide(_PolyhedralBoundedSolidHalfEdge* first,
    _PolyhedralBoundedSolidHalfEdge* second)
{
    if ( first == 0 || second == 0 ||
         first->parentEdge == 0 || second->parentEdge == 0 ) {
        return false;
    }

    return (first == first->parentEdge->rightHalf &&
            second == second->parentEdge->leftHalf) ||
           (first == first->parentEdge->leftHalf &&
            second == second->parentEdge->rightHalf);
}

//= Classic rebind ==================================================

_PolyhedralBoundedSolidFace* Connector::findUniqueClassicRebindTargetFace(
    _PolyhedralBoundedSolidHalfEdge* currentTargetFirst,
    _PolyhedralBoundedSolidHalfEdge* currentTargetSecond,
    _PolyhedralBoundedSolidHalfEdge* currentReferenceFirst,
    _PolyhedralBoundedSolidHalfEdge* currentReferenceSecond,
    const HalfEdgeList& targetEnds,
    const HalfEdgeList& referenceEnds)
{
    _PolyhedralBoundedSolidFace* candidateFace = 0;
    size_t i;
    size_t j;
    size_t pairCount;

    if ( !isLiveHalfEdge(currentTargetFirst) ||
         !isLiveHalfEdge(currentTargetSecond) ||
         !isLiveHalfEdge(currentReferenceFirst) ||
         !isLiveHalfEdge(currentReferenceSecond) ) {
        return 0;
    }

    pairCount = java::Math::min(targetEnds.size(), referenceEnds.size());
    for ( i = 0; i < pairCount; i++ ) {
        _PolyhedralBoundedSolidHalfEdge* looseTargetFirst;
        _PolyhedralBoundedSolidHalfEdge* looseReferenceFirst;

        looseTargetFirst = targetEnds[i];
        looseReferenceFirst = referenceEnds[i];
        if ( !isLiveHalfEdge(looseTargetFirst) ||
             !isLiveHalfEdge(looseReferenceFirst) ||
             !isOppositeHalfEdgeSide(currentTargetFirst, looseTargetFirst) ||
             !neighbor(currentReferenceFirst, looseReferenceFirst) ) {
            continue;
        }

        for ( j = 0; j < pairCount; j++ ) {
            _PolyhedralBoundedSolidHalfEdge* looseTargetSecond;
            _PolyhedralBoundedSolidHalfEdge* looseReferenceSecond;
            _PolyhedralBoundedSolidFace* looseFace;

            if ( i == j ) {
                continue;
            }

            looseTargetSecond = targetEnds[j];
            looseReferenceSecond = referenceEnds[j];
            if ( !isLiveHalfEdge(looseTargetSecond) ||
                 !isLiveHalfEdge(looseReferenceSecond) ||
                 !isOppositeHalfEdgeSide(currentTargetSecond, looseTargetSecond) ||
                 !neighbor(currentReferenceSecond, looseReferenceSecond) ) {
                continue;
            }

            looseFace = looseTargetFirst->parentLoop->parentFace;
            if ( looseFace != looseTargetSecond->parentLoop->parentFace ||
                 looseFace == currentTargetFirst->parentLoop->parentFace ) {
                continue;
            }

            if ( candidateFace == 0 ) {
                candidateFace = looseFace;
            }
            else if ( candidateFace != looseFace ) {
                return 0;
            }
        }
    }
    return candidateFace;
}

bool Connector::isStrictRebindTargetFace(_PolyhedralBoundedSolidFace* targetFace,
    _PolyhedralBoundedSolidHalfEdge* currentTarget)
{
    if ( targetFace == 0 ||
         !isLiveHalfEdge(currentTarget) ||
         currentTarget->startingVertex == 0 ) {
        return false;
    }
    InfinitePlane* plane = targetFace->getContainingPlane();
    if ( plane == 0 ) {
        return false;
    }

    PolyhedralBoundedSolidNumericPolicy::ToleranceContext faceContext =
        PolyhedralBoundedSolidNumericPolicy::forFace(targetFace);
    Vector3Dd point = currentTarget->startingVertex->position;
    double distance = std::fabs(plane->pointDistance(point));
    delete plane;
    if ( distance > faceContext.bigEpsilon() ) {
        return false;
    }

    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor
        ::pointInFace(targetFace, point) == Geometry::INSIDE;
}

void Connector::rebindClassicCurrentNullEdgeIfNeeded(
    _PolyhedralBoundedSolidHalfEdge* currentTargetFirst,
    _PolyhedralBoundedSolidHalfEdge* currentTargetSecond,
    _PolyhedralBoundedSolidHalfEdge* currentReferenceFirst,
    _PolyhedralBoundedSolidHalfEdge* currentReferenceSecond,
    const HalfEdgeList& targetEnds,
    const HalfEdgeList& referenceEnds,
    const char* label)
{
    _PolyhedralBoundedSolidFace* targetFace;
    _PolyhedralBoundedSolidFace* sourceFace;
    _PolyhedralBoundedSolidLoop* sourceLoop;
    PolyhedralBoundedSolid* solid;

    if ( operation != SUBTRACT ||
         !isLiveHalfEdge(currentTargetFirst) ||
         !isLiveHalfEdge(currentTargetSecond) ||
         currentTargetFirst->parentLoop != currentTargetSecond->parentLoop ) {
        return;
    }

    sourceLoop = currentTargetFirst->parentLoop;
    sourceFace = sourceLoop->parentFace;
    if ( sourceLoop->halfEdgesList.size() != 2 ) {
        return;
    }

    targetFace = findUniqueClassicRebindTargetFace(
        currentTargetFirst, currentTargetSecond,
        currentReferenceFirst, currentReferenceSecond,
        targetEnds, referenceEnds);
    if ( targetFace == 0 ||
         targetFace == sourceFace ||
         !isStrictRebindTargetFace(targetFace, currentTargetFirst) ) {
        return;
    }

    solid = currentTargetFirst->parentLoop->parentFace->parentSolid;
    if ( !PolyhedralBoundedSolidEulerOperators::lringmv(
             solid, sourceLoop, targetFace, false) ) {
        tracePipelineSummary(java::String(("connect rebind" + java::String(label) +
            " failed sourceFace=" + str(sourceFace->id) + " targetFace=" +
            str(targetFace->id) + " edge=").c_str()) +
            summarizeHalfEdge(currentTargetFirst));
        return;
    }
    tracePipelineSummary(java::String(("connect rebind" + java::String(label) +
        " sourceFace=" + str(sourceFace->id) + " targetFace=" +
        str(targetFace->id) + " edge=").c_str()) +
        summarizeHalfEdge(currentTargetFirst));
}

void Connector::rebindClassicCurrentNullEdgesIfNeeded(
    _PolyhedralBoundedSolidHalfEdge* currentARight,
    _PolyhedralBoundedSolidHalfEdge* currentALeft,
    _PolyhedralBoundedSolidHalfEdge* currentBLeft,
    _PolyhedralBoundedSolidHalfEdge* currentBRight)
{
    rebindClassicCurrentNullEdgeIfNeeded(
        currentARight, currentALeft,
        currentBLeft, currentBRight,
        endsa, endsb, "A");
    rebindClassicCurrentNullEdgeIfNeeded(
        currentBLeft, currentBRight,
        currentARight, currentALeft,
        endsb, endsa, "B");
}

//= Entry point =====================================================

Connector::ConnectResult Connector::connect(int op, int flags,
    NullEdgeList& inSonea, NullEdgeList& inSoneb)
{
    operation = op;
    debugFlags = flags;
    sonea = &inSonea;
    soneb = &inSoneb;
    lastPairCount = (int)java::Math::min(sonea->size(), soneb->size());
    lastLooseACount = 0;
    lastLooseBCount = 0;
    lastSonfaCount = 0;
    lastSonfbCount = 0;
    nextSyntheticPairIndex = lastPairCount;
    // [MANT1988] section 15.7 Program 15.14: single Connect implementation.
    // The "flexibleEndpointChains" alternative path was removed in section
    // 6.1 of plan-csg-boolean-fix-stage2 because it duplicated setOpConnect
    // with extra heuristics that the book does not require.
    setOpConnect();
    return ConnectResult(sonfa, sonfb);
}

//= Null edges ordering =============================================

void Connector::sortNullEdges()
{
    curveOrientationApplied = false;
    curveNeighborPositions.clear();
    if ( hasTestOnlyForcedConnectOrder &&
         testOnlyForcedConnectOrder.size() == sonea->size() &&
         testOnlyForcedConnectOrder.size() == soneb->size() ) {
        applyOrderPermutation(testOnlyForcedConnectOrder);
        lastCurveReport = _PolyhedralBoundedSolidSetIntersectionCurveBuilder
            ::build(*sonea, *soneb,
                PolyhedralBoundedSolidNumericPolicy::defaultContext()
                    .unitVectorTolerance());
        tracePipelineSummary("connect TEST-ONLY forced order applied");
        return;
    }

    // Always group null-edges by topological ring before any further
    // processing. Without this, the connect loop pairs null-edges from
    // different intersection curves (e.g., the outer and inner boundary
    // circles of a spherical shell), producing non-coplanar faces.
    // Ring grouping is safe for the single-ring case (it is a no-op).
    groupNullEdgesByRing();

    if ( isKeepInsertionOrderEnabled() ) {
        tracePipelineSummary("connect sort skipped; using insertion order");
        return;
    }

    // Geometric sort within each ring, enabled only when keepInsertionOrder
    // is explicitly disabled via the system property.
    sonea->sort(nullptr);
    soneb->sort(nullptr);
}

void Connector::dbgDumpNullEdges(const char* label, const NullEdgeList& sone)
{
    size_t k;
    for ( k = 0; k < sone.size(); k++ ) {
        const _PolyhedralBoundedSolidSetOperatorNullEdge& ne = sone[k];
        _PolyhedralBoundedSolidHalfEdge* rh = ne.e->rightHalf;
        _PolyhedralBoundedSolidHalfEdge* lh = ne.e->leftHalf;
        printf("[DBG-ne] %s[%d] R{v=%d f=%d p=(%.4f,%.4f,%.4f)} "
            "L{v=%d f=%d p=(%.4f,%.4f,%.4f)}\n",
            label, (int)k,
            rh->startingVertex->id, rh->parentLoop->parentFace->id,
            rh->startingVertex->position.x(),
            rh->startingVertex->position.y(),
            rh->startingVertex->position.z(),
            lh->startingVertex->id, lh->parentLoop->parentFace->id,
            lh->startingVertex->position.x(),
            lh->startingVertex->position.y(),
            lh->startingVertex->position.z());
    }
}

/**
Reorders sonea/soneb in lockstep by the given permutation
(`position -> originalIndex`).
@param order permutation covering every index exactly once
*/
void Connector::applyOrderPermutation(const java::ArrayList<int>& order)
{
    NullEdgeList orderedA;
    NullEdgeList orderedB;
    size_t i;

    for ( i = 0; i < order.size(); i++ ) {
        orderedA.add((*sonea)[order[i]]);
        orderedB.add((*soneb)[order[i]]);
    }
    *sonea = orderedA;
    *soneb = orderedB;
}

int Connector::curveComponentFind(java::ArrayList<int>& parent, int x)
{
    while ( parent[x] != x ) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

namespace {

void nullEdgeFaceIds(const _PolyhedralBoundedSolidSetOperatorNullEdge& ne,
    long long ids[2])
{
    ids[0] = ne.e->rightHalf->parentLoop->parentFace->id;
    ids[1] = ne.e->leftHalf->parentLoop->parentFace->id;
}

bool nullEdgesShareFace(const long long* a, const long long* b)
{
    return a[0] == b[0] || a[0] == b[1] || a[1] == b[0] || a[1] == b[1];
}

}

/**
Reorders `sonea`/`soneb` so that null-edge pairs belonging to the same
intersection curve are contiguous and ordered for `scanjoin`, and distinct
curves are separated. Each intersection curve is recovered as a connected
component over the paired null-edge indices (face adjacency); within each
component the classifier's emission order is preserved.
*/
void Connector::groupNullEdgesByRing()
{
    if ( isPipelineSummaryTraceEnabled() ) {
        dbgDumpNullEdges("A", *sonea);
        dbgDumpNullEdges("B", *soneb);
    }

    // Reconstruct the intersection curves for diagnosis. The report is
    // captured before any reordering decision so it always describes the
    // classifier's raw emission.
    lastCurveReport = _PolyhedralBoundedSolidSetIntersectionCurveBuilder
        ::build(*sonea, *soneb,
            PolyhedralBoundedSolidNumericPolicy::defaultContext()
                .unitVectorTolerance());
    tracePipelineSummary(java::String("connect ") + lastCurveReport.summarize());

    int n = (int)java::Math::min(sonea->size(), soneb->size());
    if ( n != (int)sonea->size() || n != (int)soneb->size() || n < 2 ) {
        return;
    }

    // When every null-edge pair lies on a cleanly closed intersection
    // curve, reorder pairs along each curve AND orient every strut
    // consistently with the traversal. On any anomaly fall through to the
    // legacy ordering below.
    if ( lastCurveReport.isCleanlyClosed() ) {
        java::ArrayList<int> curveOrder;
        if ( _PolyhedralBoundedSolidSetIntersectionCurveBuilder
                 ::orderAndOrientAlongCurves(lastCurveReport.cycles, n,
                     *sonea, *soneb, curveOrder) ) {
            applyOrderPermutation(curveOrder);
            curveOrientationApplied = true;
            curveNeighborPositions =
                _PolyhedralBoundedSolidSetIntersectionCurveBuilder
                    ::lastTraversalNeighborPositions;
            tracePipelineSummary(("connect curve-order applied: cycles=" +
                str((int)lastCurveReport.cycles.size())).c_str());
            return;
        }
    }

    // When all null-edge rings are singletons (every pair is a zero-length
    // strut), there is no ring structure to separate, and a spatial
    // signature sort only scrambles the classifier's already-valid emission
    // order. Preserve insertion order in that case.
    java::ArrayList<long long> faceIdsA = java::Collections::nCopies<long long>((size_t)n * 2, 0LL);
    java::ArrayList<long long> faceIdsB = java::Collections::nCopies<long long>((size_t)n * 2, 0LL);
    int k;
    for ( k = 0; k < n; k++ ) {
        nullEdgeFaceIds((*sonea)[k], &faceIdsA[2*k]);
        nullEdgeFaceIds((*soneb)[k], &faceIdsB[2*k]);
    }

    java::ArrayList<int> parent = java::Collections::nCopies<int>((size_t)n, 0);
    for ( k = 0; k < n; k++ ) {
        parent[k] = k;
    }
    int i;
    int j;
    for ( i = 0; i < n; i++ ) {
        for ( j = i + 1; j < n; j++ ) {
            if ( nullEdgesShareFace(&faceIdsA[2*i], &faceIdsA[2*j]) ||
                 nullEdgesShareFace(&faceIdsB[2*i], &faceIdsB[2*j]) ) {
                parent[curveComponentFind(parent, i)] =
                    curveComponentFind(parent, j);
            }
        }
    }

    // Count distinct curve components.
    java::TreeSet<int> roots;
    for ( k = 0; k < n; k++ ) {
        roots.add(curveComponentFind(parent, k));
    }
    int componentCount = (int)roots.size();

    // Decide whether to reorder: only when at least one shared vertex id
    // exists, which signals a multi-curve intersection where distinct
    // curves must be kept contiguous. Vertex ids (not positions) are used
    // because after weldCoincidentVertices the coincident pairs are already
    // merged.
    java::TreeSet<int> vidsA;
    bool hasSharedA = false;
    int ki;
    for ( ki = 0; ki < n && !hasSharedA; ki++ ) {
        int v1 = (*sonea)[ki].e->rightHalf->startingVertex->id;
        int v2 = (*sonea)[ki].e->leftHalf->startingVertex->id;
        if ( !vidsA.add(v1) || (v1 != v2 && !vidsA.add(v2)) ) {
            hasSharedA = true;
        }
    }
    bool hasSharedB = false;
    if ( !hasSharedA ) {
        java::TreeSet<int> vidsB;
        for ( ki = 0; ki < n && !hasSharedB; ki++ ) {
            int v1 = (*soneb)[ki].e->rightHalf->startingVertex->id;
            int v2 = (*soneb)[ki].e->leftHalf->startingVertex->id;
            if ( !vidsB.add(v1) || (v1 != v2 && !vidsB.add(v2)) ) {
                hasSharedB = true;
            }
        }
    }
    if ( !hasSharedA && !hasSharedB ) {
        tracePipelineSummary("connect ring-group: all singletons; "
            "preserving insertion order");
        return;
    }

    // Multiple pairs share face-adjacency: reconstruct curve order, with
    // components in order of first appearance (as Java `LinkedHashMap`).
    java::ArrayList<int> componentRoots;
    java::ArrayList<java::ArrayList<int> > components;
    for ( k = 0; k < n; k++ ) {
        int root = curveComponentFind(parent, k);
        size_t c;
        for ( c = 0; c < componentRoots.size(); c++ ) {
            if ( componentRoots[c] == root ) {
                break;
            }
        }
        if ( c == componentRoots.size() ) {
            componentRoots.add(root);
            components.add(java::ArrayList<int>());
        }
        components[c].add(k);
    }

    NullEdgeList orderedA;
    NullEdgeList orderedB;
    for ( size_t c = 0; c < components.size(); c++ ) {
        for ( size_t m = 0; m < components[c].size(); m++ ) {
            int idx = components[c][m];
            orderedA.add((*sonea)[idx]);
            orderedB.add((*soneb)[idx]);
        }
    }
    *sonea = orderedA;
    *soneb = orderedB;

    tracePipelineSummary(("connect curve-components: count=" +
        str(componentCount)).c_str());
}

/**
Implements sgetnextnulledge per [MANT1988] section 15.7, Program 15.14:
advances the internal cursor over `sonea`/`soneb` and fills `out` with the
next (nea, neb) pair.
*/
bool Connector::sgetnextnulledge(NullEdgePair& out)
{
    if ( nextNullEdgeIndex >= (int)sonea->size() ||
         nextNullEdgeIndex >= (int)soneb->size() ) {
        return false;
    }
    out.nea = &(*sonea)[nextNullEdgeIndex];
    out.neb = &(*soneb)[nextNullEdgeIndex];
    out.pairIndex = nextNullEdgeIndex;
    nextNullEdgeIndex++;
    return true;
}

/**
Implements scanjoin per [MANT1988] section 15.7, Program 15.13: finds the
loose pair `i` such that `hea` is a neighbor of `endsa[i]` AND `heb` is a
neighbor of `endsb[i]`, removes and returns it. Otherwise `hea` and `heb`
are appended to the loose lists and false (Java null) is returned.
*/
bool Connector::scanjoin(_PolyhedralBoundedSolidHalfEdge* hea,
    _PolyhedralBoundedSolidHalfEdge* heb,
    _PolyhedralBoundedSolidHalfEdge** outA,
    _PolyhedralBoundedSolidHalfEdge** outB)
{
    size_t i;
    bool condition1;
    bool condition2;

    for ( i = 0; i < endsa.size(); i++ ) {
        condition1 = neighbor(hea, endsa[i]);
        condition2 = neighbor(heb, endsb[i]);

        if ( (debugFlags & DEBUG_05_CONNECT) != 0x00 ) {
            printf("    . Testing for neighborhood A[%d/%d] vs. A[%d/%d]: %s "
                "ParentFaces: %d / %d\n",
                hea->startingVertex->id, hea->next()->startingVertex->id,
                endsa[i]->startingVertex->id,
                endsa[i]->next()->startingVertex->id,
                condition1 ? "true" : "false",
                hea->parentLoop->parentFace->id,
                endsa[i]->parentLoop->parentFace->id);
            printf("    . Testing for neighborhood B[%d/%d] vs. B[%d/%d]: %s "
                "ParentFaces: %d / %d\n",
                heb->startingVertex->id, heb->next()->startingVertex->id,
                endsb[i]->startingVertex->id,
                endsb[i]->next()->startingVertex->id,
                condition2 ? "true" : "false",
                heb->parentLoop->parentFace->id,
                endsb[i]->parentLoop->parentFace->id);
        }

        if ( condition1 && condition2 ) {
            *outA = endsa[i];
            *outB = endsb[i];
            endsa.remove(i);
            endsb.remove(i);
            if ( i < endsPairIndex.size() ) {
                endsPairIndex.remove((long)i);
            }
            return true;
        }
    }

    // Curve-ordered path only: rescue a unique near-miss before declaring
    // this pair loose (an earlier division can re-parent a pending strut
    // ring away from the face where its junction partner waits).
    if ( curveOrientationApplied ) {
        if ( rescueRingFaceNearMiss(hea, heb, outA, outB) ) {
            return true;
        }
    }

    endsa.add(hea);
    endsb.add(heb);
    endsPairIndex.add(currentConnectPairIndex);
    return false;
}

bool Connector::rolesOpposite(_PolyhedralBoundedSolidHalfEdge* h1,
    _PolyhedralBoundedSolidHalfEdge* h2)
{
    if ( h1 == 0 || h2 == 0 || h1->parentEdge == 0 || h2->parentEdge == 0 ) {
        return false;
    }
    return (h1 == h1->parentEdge->rightHalf &&
            h2 == h2->parentEdge->leftHalf) ||
           (h1 == h1->parentEdge->leftHalf &&
            h2 == h2->parentEdge->rightHalf);
}

/**
True when the half-edge dangles in a pending two-half-edge strut ring (an
inner loop holding only the null edge, as created by the vertex/face
classifier's makeRing).
*/
bool Connector::isPendingStrutRing(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == 0 || he->parentLoop == 0 ||
         he->parentLoop->parentFace == 0 ||
         he->parentLoop->parentFace->boundariesList.size() == 0 ) {
        return false;
    }
    if ( he->parentLoop->halfEdgesList.size() != 2 ) {
        return false;
    }
    return he->parentLoop != he->parentLoop->parentFace->boundariesList.get(0);
}

/**
Near-miss rescue: finds the unique loose index (pushed by a curve neighbor
of the current pair) that matches on one solid and fails only the face
equality on the other, with a pending strut ring on the mismatched side;
re-parents that ring and completes the match.
*/
bool Connector::rescueRingFaceNearMiss(_PolyhedralBoundedSolidHalfEdge* hea,
    _PolyhedralBoundedSolidHalfEdge* heb,
    _PolyhedralBoundedSolidHalfEdge** outA,
    _PolyhedralBoundedSolidHalfEdge** outB)
{
    int candidate = -1;
    bool mismatchOnA = false;
    size_t i;

    for ( i = 0; i < endsa.size(); i++ ) {
        // Only true curve neighbors of the current pair may be rescued.
        if ( curveNeighborPositions.isEmpty() ||
             currentConnectPairIndex < 0 ||
             currentConnectPairIndex >= (int)curveNeighborPositions.size() ||
             curveNeighborPositions[currentConnectPairIndex].isEmpty() ||
             i >= endsPairIndex.size() ) {
            continue;
        }
        int pusherPosition = endsPairIndex[i];
        if ( pusherPosition != curveNeighborPositions[currentConnectPairIndex][0] &&
             pusherPosition != curveNeighborPositions[currentConnectPairIndex][1] ) {
            continue;
        }
        bool aOk = neighbor(hea, endsa[i]);
        bool bOk = neighbor(heb, endsb[i]);
        bool nearMissA = !aOk && bOk &&
            rolesOpposite(hea, endsa[i]) &&
            (isPendingStrutRing(hea) || isPendingStrutRing(endsa[i]));
        bool nearMissB = aOk && !bOk &&
            rolesOpposite(heb, endsb[i]) &&
            (isPendingStrutRing(heb) || isPendingStrutRing(endsb[i]));
        if ( nearMissA || nearMissB ) {
            if ( candidate >= 0 ) {
                tracePipelineSummary("connect ring-rescue ambiguous; skipped");
                return false;
            }
            candidate = (int)i;
            mismatchOnA = nearMissA;
        }
    }
    if ( candidate < 0 ) {
        return false;
    }

    _PolyhedralBoundedSolidHalfEdge* query = mismatchOnA ? hea : heb;
    _PolyhedralBoundedSolidHalfEdge* stored = mismatchOnA ?
        endsa[candidate] : endsb[candidate];
    _PolyhedralBoundedSolidHalfEdge* ringSide;
    _PolyhedralBoundedSolidHalfEdge* anchorSide;
    if ( isPendingStrutRing(query) ) {
        ringSide = query;
        anchorSide = stored;
    }
    else {
        ringSide = stored;
        anchorSide = query;
    }
    _PolyhedralBoundedSolidFace* targetFace = anchorSide->parentLoop->parentFace;
    if ( targetFace == 0 || targetFace->parentSolid == 0 ) {
        return false;
    }
    if ( !PolyhedralBoundedSolidEulerOperators::lringmv(
             targetFace->parentSolid, ringSide->parentLoop,
             targetFace, false) ) {
        return false;
    }
    if ( !neighbor(hea, endsa[candidate]) ||
         !neighbor(heb, endsb[candidate]) ) {
        tracePipelineSummary(
            "connect ring-rescue re-parent did not complete the match");
        return false;
    }
    tracePipelineSummary(("connect ring-rescue applied: ring v=" +
        (ringSide->startingVertex == 0 ? java::String("?") :
         str(ringSide->startingVertex->id)) +
        " -> face " + str(targetFace->id)).c_str());
    *outA = endsa[candidate];
    *outB = endsb[candidate];
    endsa.remove(candidate);
    endsb.remove(candidate);
    if ( candidate < (int)endsPairIndex.size() ) {
        endsPairIndex.remove((long)candidate);
    }
    return true;
}

bool Connector::isLooseA(_PolyhedralBoundedSolidHalfEdge* he)
{
    return endsa.contains(he);
}

bool Connector::isLooseB(_PolyhedralBoundedSolidHalfEdge* he)
{
    return endsb.contains(he);
}

//= Cuts ============================================================

_PolyhedralBoundedSolidFace* Connector::cutA(_PolyhedralBoundedSolidHalfEdge* he)
{
    PolyhedralBoundedSolid* s;
    _PolyhedralBoundedSolidFace* addedFace = 0;
    bool withDebug = ((debugFlags & DEBUG_99_SHOWOPERATIONS) != 0x0) &&
                     ((debugFlags & DEBUG_05_CONNECT) != 0x00);

    if ( withDebug ) {
        printf("       -> CUTA:\n");
        printf("          . He: %s\n", he->toString().c_str());
    }

    s = he->parentLoop->parentFace->parentSolid;
    if ( he->parentEdge->rightHalf->parentLoop ==
         he->parentEdge->leftHalf->parentLoop ) {
        addedFace = he->parentLoop->parentFace;
        PolyhedralBoundedSolidEulerOperators::lkemr(s,
            he->parentEdge->rightHalf, he->parentEdge->leftHalf);
        if ( addedFace->boundariesList.size() >= 2 ) {
            sonfa.add(addedFace);
            if ( hasPairIndexMaps ) {
                sonfaPairIndexByFaceId.put(addedFace->id, currentConnectPairIndex);
            }
        }
        else {
            addedFace = 0;
        }
    }
    else {
        PolyhedralBoundedSolidEulerOperators::lkef(s,
            he->parentEdge->rightHalf, he->parentEdge->leftHalf);
    }
    return addedFace;
}

_PolyhedralBoundedSolidFace* Connector::cutB(_PolyhedralBoundedSolidHalfEdge* he)
{
    PolyhedralBoundedSolid* s;
    _PolyhedralBoundedSolidFace* addedFace = 0;
    bool withDebug = ((debugFlags & DEBUG_99_SHOWOPERATIONS) != 0x0) &&
                     ((debugFlags & DEBUG_05_CONNECT) != 0x00);

    if ( withDebug ) {
        printf("       -> CUTB:\n");
        printf("          . He: %s\n", he->toString().c_str());
    }

    s = he->parentLoop->parentFace->parentSolid;
    if ( he->parentEdge->rightHalf->parentLoop ==
         he->parentEdge->leftHalf->parentLoop ) {
        addedFace = he->parentLoop->parentFace;
        PolyhedralBoundedSolidEulerOperators::lkemr(s,
            he->parentEdge->rightHalf, he->parentEdge->leftHalf);
        if ( addedFace->boundariesList.size() >= 2 ) {
            sonfb.add(addedFace);
            if ( hasPairIndexMaps ) {
                sonfbPairIndexByFaceId.put(addedFace->id, currentConnectPairIndex);
            }
        }
        else {
            addedFace = 0;
        }
    }
    else {
        PolyhedralBoundedSolidEulerOperators::lkef(s,
            he->parentEdge->rightHalf, he->parentEdge->leftHalf);
    }
    return addedFace;
}

void Connector::removeLastCutFaceIfSame(FaceList& faces,
    _PolyhedralBoundedSolidFace* face)
{
    if ( face == 0 || faces.isEmpty() ) {
        return;
    }
    if ( faces[faces.size() - 1] == face ) {
        faces.remove(faces.size() - 1);
    }
}

//= Connect loop ====================================================

/**
Neighbor null edges connector for the set operations algorithm
(big phase 3). Following section [MANT1988].15.7. and program
[MANT1988].15.14.
*/
void Connector::setOpConnect()
{
    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("- 3. ------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    }

    sortNullEdges();

    int i;

    if ( (debugFlags & DEBUG_05_CONNECT) != 0x00 ) {
        printf("SORTED SET OF %d NULL EDGES PAIRS TO BE CONNECTED\n",
            (int)sonea->size());
    }
    tracePipelineSummary(("connect start pairsA=" + str((int)sonea->size()) +
        " pairsB=" + str((int)soneb->size())).c_str());
    if ( isPipelineSummaryTraceEnabled() ) {
        int sameLoopA = 0;
        int diffLoopA = 0;
        for ( size_t k = 0; k < sonea->size(); k++ ) {
            _PolyhedralBoundedSolidEdge* e = (*sonea)[k].e;
            if ( e->rightHalf != 0 && e->leftHalf != 0 &&
                 e->rightHalf->parentLoop == e->leftHalf->parentLoop ) {
                sameLoopA++;
            }
            else {
                diffLoopA++;
            }
        }
        int sameLoopB = 0;
        int diffLoopB = 0;
        for ( size_t k = 0; k < soneb->size(); k++ ) {
            _PolyhedralBoundedSolidEdge* e = (*soneb)[k].e;
            if ( e->rightHalf != 0 && e->leftHalf != 0 &&
                 e->rightHalf->parentLoop == e->leftHalf->parentLoop ) {
                sameLoopB++;
            }
            else {
                diffLoopB++;
            }
        }
        tracePipelineSummary(("connect null-edge-loops A:sameLoop=" +
            str(sameLoopA) + " diffLoop=" + str(diffLoopA) +
            " B:sameLoop=" + str(sameLoopB) +
            " diffLoop=" + str(diffLoopB)).c_str());
    }

    _PolyhedralBoundedSolidEdge* nextedgea;
    _PolyhedralBoundedSolidEdge* nextedgeb;
    _PolyhedralBoundedSolidHalfEdge* h1a = 0;
    _PolyhedralBoundedSolidHalfEdge* h2a = 0;
    _PolyhedralBoundedSolidHalfEdge* h1b = 0;
    _PolyhedralBoundedSolidHalfEdge* h2b = 0;
    _PolyhedralBoundedSolidHalfEdge* r0;
    _PolyhedralBoundedSolidHalfEdge* r1;
    bool allowRingMoveOnAJoin = (operation == INTERSECTION);
    bool withDebug = ((debugFlags & DEBUG_99_SHOWOPERATIONS) != 0x0) &&
                     ((debugFlags & DEBUG_05_CONNECT) != 0x00);

    endsa.clear();
    endsb.clear();
    endsPairIndex.clear();

    sonfa.clear();
    sonfb.clear();
    sonfaPairIndexByFaceId.clear();
    sonfbPairIndexByFaceId.clear();
    hasPairIndexMaps = true;
    setCurrentConnectContext(-1);
    size_t j;

    if ( sonea->size() != soneb->size() ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        printf("**** Not paired null edges!\n");
    }

    // [MANT1988] Program 15.14:
    //   while (sgetnextnulledge(&nea, &neb)) { ... }
    // The cursor is set to 0 here so each call to setOpConnect()
    // restarts iteration from the first pair of the sorted set.
    nextNullEdgeIndex = 0;
    NullEdgePair pair;
    while ( sgetnextnulledge(pair) ) {
        _PolyhedralBoundedSolidSetOperatorNullEdge* nea = pair.nea;
        _PolyhedralBoundedSolidSetOperatorNullEdge* neb = pair.neb;
        i = pair.pairIndex;
        _PolyhedralBoundedSolidHalfEdge* ha;
        _PolyhedralBoundedSolidHalfEdge* ham;
        _PolyhedralBoundedSolidHalfEdge* hb;
        _PolyhedralBoundedSolidHalfEdge* hbm;
        _PolyhedralBoundedSolidHalfEdge* tmp;

        ha = nea->e->rightHalf;
        ham = nea->e->leftHalf;
        hb = neb->e->rightHalf;
        hbm = neb->e->leftHalf;
        if ( isPipelineSummaryTraceEnabled() ) {
            tracePipelineSummary(java::String(("connect pair[" + str(i) +
                "] A{").c_str()) + summarizeNullEdge(*nea) + "} B{" +
                summarizeNullEdge(*neb) + "}");
        }

        if ( (debugFlags & DEBUG_05_CONNECT) != 0x00 ) {
            printf("  - %d = %d+%d loose ends before processing pair [%d]:\n",
                (int)(endsa.size() + endsb.size()), (int)endsa.size(),
                (int)endsb.size(), i);

            for ( j = 0; j < endsa.size(); j++ ) {
                _PolyhedralBoundedSolidHalfEdge* hat = endsa[j];
                _PolyhedralBoundedSolidHalfEdge* hbt = endsb[j];
                printf("    . [%d]: He(A): %d/%d | He(B): %d/%d\n", (int)j,
                    hat->startingVertex->id, hat->next()->startingVertex->id,
                    hbt->startingVertex->id, hbt->next()->startingVertex->id);
            }

            if ( ha->startingVertex->id > ham->startingVertex->id ) {
                printf("********* FORCING ORDER!\n");
            }

            printf("  - Processing pair [%d]: He(A1): %d/%d He(A2): %d/%d "
                "He(B1): %d/%d He(B2): %d/%d\n", i,
                ha->startingVertex->id, ha->next()->startingVertex->id,
                ham->startingVertex->id, ham->next()->startingVertex->id,
                hb->startingVertex->id, hb->next()->startingVertex->id,
                hbm->startingVertex->id, hbm->next()->startingVertex->id);
        }

        nextedgea = nea->e;
        nextedgeb = neb->e;
        h1a = 0;
        h2a = 0;
        h1b = 0;
        h2b = 0;
        // Legacy strut orientation: vertex-id order encodes classifier
        // emission order. When sortNullEdges already oriented the struts
        // along the intersection curves, that orientation must be respected.
        if ( !curveOrientationApplied ) {
            if ( ha->startingVertex->id > ham->startingVertex->id ) {
                tmp = nextedgea->rightHalf;
                nextedgea->rightHalf = nextedgea->leftHalf;
                nextedgea->leftHalf = tmp;
            }
            if ( hb->startingVertex->id > hbm->startingVertex->id ) {
                tmp = nextedgeb->rightHalf;
                nextedgeb->rightHalf = nextedgeb->leftHalf;
                nextedgeb->leftHalf = tmp;
            }
        }

        rebindClassicCurrentNullEdgesIfNeeded(
            nextedgea->rightHalf,
            nextedgea->leftHalf,
            nextedgeb->leftHalf,
            nextedgeb->rightHalf);

        setCurrentConnectContext(i);
        if ( scanjoin(nextedgea->rightHalf, nextedgeb->leftHalf, &r0, &r1) ) {
            h1a = r0;
            h2b = r1;
            join(h1a, nextedgea->rightHalf, withDebug, allowRingMoveOnAJoin);
            if ( !isLooseA(h1a->mirrorHalfEdge()) ) {
                cutA(h1a);
            }
            join(h2b, nextedgeb->leftHalf, withDebug);
            if ( !isLooseB(h2b->mirrorHalfEdge()) ) {
                cutB(h2b);
            }
        }

        setCurrentConnectContext(i);
        if ( scanjoin(nextedgea->leftHalf, nextedgeb->rightHalf, &r0, &r1) ) {
            h2a = r0;
            h1b = r1;
            join(h2a, nextedgea->leftHalf, withDebug, allowRingMoveOnAJoin);
            if ( !isLooseA(h2a->mirrorHalfEdge()) ) {
                cutA(h2a);
            }
            join(h1b, nextedgeb->rightHalf, withDebug);
            if ( !isLooseB(h1b->mirrorHalfEdge()) ) {
                cutB(h1b);
            }
        }

        if ( h1a != 0 && h1b != 0 && h2a != 0 && h2b != 0 ) {
            cutA(nextedgea->rightHalf);
            cutB(nextedgeb->rightHalf);
            if ( isPipelineSummaryTraceEnabled() ) {
                tracePipelineSummary(java::String(("connect pair[" + str(i) +
                    "] produced cuts h1a=").c_str()) + summarizeHalfEdge(h1a) +
                    " h2a=" + summarizeHalfEdge(h2a) +
                    " h1b=" + summarizeHalfEdge(h1b) +
                    " h2b=" + summarizeHalfEdge(h2b));
            }
        }
        else {
            PolyhedralBoundedSolidStatistics::recordJoinIncompleteCase();
            PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
            if ( isPipelineSummaryTraceEnabled() ) {
                tracePipelineSummary(java::String(("connect pair[" + str(i) +
                    "] incomplete h1a=").c_str()) + summarizeHalfEdge(h1a) +
                    " h2a=" + summarizeHalfEdge(h2a) +
                    " h1b=" + summarizeHalfEdge(h1b) +
                    " h2b=" + summarizeHalfEdge(h2b) +
                    (" looseA=" + str((int)endsa.size()) +
                     " looseB=" + str((int)endsb.size())).c_str());
            }
        }
    }

    if ( (debugFlags & DEBUG_05_CONNECT) != 0x00 ) {
        printf("  . Pending null edges to connect:\n");
        for ( j = 0; j < endsa.size(); j++ ) {
            printf("    . A[%d]: %s\n", (int)(j+1), endsa[j]->toString().c_str());
        }
        for ( j = 0; j < endsb.size(); j++ ) {
            printf("    . B[%d]: %s\n", (int)(j+1), endsb[j]->toString().c_str());
        }
    }
    tracePipelineSummary(("connect end sonfa=" + str((int)sonfa.size()) +
        " sonfb=" + str((int)sonfb.size()) +
        " looseA=" + str((int)endsa.size()) +
        " looseB=" + str((int)endsb.size())).c_str());
    // Section 6.1-B: post-loop safety nets removed. Per [MANT1988]
    // Program 15.14, the main loop must leave looseA == looseB == 0 by
    // itself; any survivor is the visible symptom of an upstream defect.
    tracePipelineSummary(("connect post-pass sonfa=" + str((int)sonfa.size()) +
        " sonfb=" + str((int)sonfb.size()) +
        " looseA=" + str((int)endsa.size()) +
        " looseB=" + str((int)endsb.size())).c_str());
    updateLastSnapshot();

    if ( isPipelineSummaryTraceEnabled() ) {
        for ( j = 0; j < endsa.size() && j < endsb.size(); j++ ) {
            tracePipelineSummary(java::String(("connect loose[" + str((int)j) +
                "] A=").c_str()) + summarizeHalfEdge(endsa[j]) + " B=" +
                summarizeHalfEdge(endsb[j]));
        }
    }
}
