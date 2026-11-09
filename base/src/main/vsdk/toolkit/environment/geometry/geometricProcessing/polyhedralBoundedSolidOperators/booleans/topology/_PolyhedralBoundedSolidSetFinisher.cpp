//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "java/util/Collections.h"
#include "java/lang/String.h"
#include <cmath>
#include <cstdio>

#include "java/lang/Boolean.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetFinisher.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetNullEdgesConnector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetFinisher Finisher;
typedef PolyhedralBoundedSolidNumericPolicy::ToleranceContext ToleranceContext;

namespace {

const char* const TRACE_PIPELINE_SUMMARY_PROPERTY =
    "vsdk.setop.tracePipelineSummary";

java::String str(int value)
{
    return java::String::valueOf(value);
}

}

int Finisher::lastLegacyFallbackCount = 0;
int Finisher::lastTriangulatedFaceCount = 0;

Finisher::_PolyhedralBoundedSolidSetFinisher()
{
}

int Finisher::getLastLegacyFallbackCount()
{
    return lastLegacyFallbackCount;
}

int Finisher::getLastTriangulatedFaceCount()
{
    return lastTriangulatedFaceCount;
}

bool Finisher::isPipelineSummaryTraceEnabled()
{
    return java::Boolean::getBoolean(TRACE_PIPELINE_SUMMARY_PROPERTY);
}

void Finisher::tracePipelineSummary(const java::String& message)
{
    if ( !isPipelineSummaryTraceEnabled() ) {
        return;
    }
    printf("[SetOpPipelineTrace] %s\n", message.c_str());
}

bool Finisher::hasUsableIntegrationRing(_PolyhedralBoundedSolidFace* face)
{
    _PolyhedralBoundedSolidLoop* ring;
    _PolyhedralBoundedSolidHalfEdge* start;
    _PolyhedralBoundedSolidHalfEdge* current;
    Vector3Dd reference;
    long int guard;

    if ( face == 0 ||
         !hasCompleteHalfEdgeConnectivity(face) ||
         face->boundariesList.size() < 2 ||
         face->boundariesList.get(1) == 0 ) {
        return false;
    }
    ring = face->boundariesList.get(1);
    if ( ring->halfEdgesList.size() < 2 ) {
        return false;
    }
    start = ring->boundaryStartHalfEdge;
    if ( start == 0 || start->startingVertex == 0 ) {
        return false;
    }
    reference = start->startingVertex->position;
    ToleranceContext context = PolyhedralBoundedSolidNumericPolicy::forFace(face);
    current = start->next();
    guard = 0;
    while ( current != 0 && current != start &&
            guard <= ring->halfEdgesList.size() ) {
        if ( current->startingVertex != 0 &&
             !PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                 reference, current->startingVertex->position, context) ) {
            return true;
        }
        current = current->next();
        guard++;
    }
    return false;
}

bool Finisher::hasCompleteHalfEdgeConnectivity(_PolyhedralBoundedSolidFace* face)
{
    long int i;

    if ( face == 0 ) {
        return false;
    }
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* start;
        _PolyhedralBoundedSolidHalfEdge* current;
        long int guard;

        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
            return false;
        }
        start = loop->boundaryStartHalfEdge;
        current = start;
        guard = 0;
        do {
            if ( current == 0 ||
                 current->parentEdge == 0 ||
                 current->parentLoop == 0 ||
                 current->startingVertex == 0 ||
                 current->mirrorHalfEdge() == 0 ||
                 current->next() == 0 ||
                 current->previous() == 0 ) {
                return false;
            }
            current = current->next();
            guard++;
        } while ( current != start &&
                  guard <= loop->halfEdgesList.size() + 1 );
        if ( current != start ) {
            return false;
        }
    }
    return true;
}

java::String Finisher::integrationRingSummary(_PolyhedralBoundedSolidFace* face)
{
    java::String text;

    if ( face == 0 ) {
        return "null";
    }
    if ( face->boundariesList.size() < 2 ||
         face->boundariesList.get(1) == 0 ) {
        text = "face=" + str(face->id) + " boundaries=" +
            str((int)face->boundariesList.size());
        return java::String(text.c_str());
    }
    text = "face=" + str(face->id) + " ringSize=" +
        str((int)face->boundariesList.get(1)->halfEdgesList.size()) +
        " usable=" + (hasUsableIntegrationRing(face) ? "true" : "false") +
        " connected=" + (hasCompleteHalfEdgeConnectivity(face) ? "true" : "false") +
        " pair=" +
        str(_PolyhedralBoundedSolidSetNullEdgesConnector::getSonfaPairIndex(face)) +
        "/" +
        str(_PolyhedralBoundedSolidSetNullEdgesConnector::getSonfbPairIndex(face));
    return java::String(text.c_str());
}

int Finisher::sanitizePairedFaces(FaceList& sonfa, FaceList& sonfb)
{
    FaceList matchedA;
    FaceList matchedB;
    size_t i;
    size_t j;

    lastLegacyFallbackCount = 0;

    java::ArrayList<bool> usedB = java::Collections::nCopies<bool>(sonfb.size(), false);

    for ( i = 0; i < sonfa.size(); i++ ) {
        _PolyhedralBoundedSolidFace* faceA = sonfa[i];
        int pairIndexA;
        bool validA = hasUsableIntegrationRing(faceA);

        if ( !validA ) {
            if ( isPipelineSummaryTraceEnabled() ) {
                tracePipelineSummary(java::String("finish sanitize skip A ") +
                    integrationRingSummary(faceA));
            }
            continue;
        }

        pairIndexA = _PolyhedralBoundedSolidSetNullEdgesConnector
            ::getSonfaPairIndex(faceA);
        for ( j = 0; j < sonfb.size(); j++ ) {
            _PolyhedralBoundedSolidFace* faceB = sonfb[j];
            int pairIndexB;
            bool validB;

            if ( usedB[j] ) {
                continue;
            }
            validB = hasUsableIntegrationRing(faceB);
            if ( !validB ) {
                if ( isPipelineSummaryTraceEnabled() ) {
                    tracePipelineSummary(java::String("finish sanitize skip B ") +
                        integrationRingSummary(faceB));
                }
                continue;
            }
            pairIndexB = _PolyhedralBoundedSolidSetNullEdgesConnector
                ::getSonfbPairIndex(faceB);
            if ( pairIndexA != -1 && pairIndexA == pairIndexB ) {
                if ( isPipelineSummaryTraceEnabled() ) {
                    tracePipelineSummary(java::String("finish sanitize match A ") +
                        integrationRingSummary(faceA) + " B " +
                        integrationRingSummary(faceB));
                }
                matchedA.add(faceA);
                matchedB.add(faceB);
                usedB[j] = true;
                break;
            }
        }
    }

    if ( matchedA.isEmpty() && !sonfa.isEmpty() && sonfa.size() == sonfb.size() ) {
        // Section 9.1 instrumentation: legacy ordering fallback - no
        // pairIndex matches found. Count it so tests can assert it stays 0.
        lastLegacyFallbackCount++;
        Logger::reportMessage(java::String(), Logger::WARNING,
            "sanitizePairedFaces",
            ("Legacy ordering fallback taken (pairIndex matching found no "
             "pairs for sonfa=" + str((int)sonfa.size()) + " sonfb=" +
             str((int)sonfb.size()) +
             "). This indicates Connect did not tag faces with pairIndex.").c_str());
        tracePipelineSummary("finish sanitize kept legacy ordering");
        return (int)sonfa.size();
    }

    sonfa = matchedA;
    sonfb = matchedB;
    tracePipelineSummary(("finish sanitize matched=" +
        str((int)matchedA.size())).c_str());
    return (int)matchedA.size();
}

_PolyhedralBoundedSolidHalfEdge* Finisher::findNonDegenerateEar(
    _PolyhedralBoundedSolidHalfEdge* start,
    int loopSize,
    const ToleranceContext& context)
{
    _PolyhedralBoundedSolidHalfEdge* candidate;
    _PolyhedralBoundedSolidHalfEdge* nextHe;
    _PolyhedralBoundedSolidHalfEdge* prevHe;
    _PolyhedralBoundedSolidHalfEdge* bestCandidate;
    Vector3Dd p0;
    Vector3Dd p1;
    Vector3Dd p2;
    Vector3Dd a;
    Vector3Dd b;
    double bestSinTheta;
    int safety;

    if ( start == 0 || loopSize <= 0 ) {
        return 0;
    }
    bestCandidate = 0;
    bestSinTheta = 0.0;
    candidate = start;
    safety = 0;
    do {
        nextHe = candidate->next();
        prevHe = candidate->previous();
        if ( nextHe != 0 && prevHe != 0 && nextHe != prevHe &&
             candidate->parentLoop != 0 &&
             nextHe->parentLoop == candidate->parentLoop &&
             prevHe->parentLoop == candidate->parentLoop &&
             candidate->startingVertex != 0 &&
             nextHe->startingVertex != 0 &&
             prevHe->startingVertex != 0 ) {
            p0 = prevHe->startingVertex->position;
            p1 = candidate->startingVertex->position;
            p2 = nextHe->startingVertex->position;
            a = p1.subtract(p0);
            b = p2.subtract(p0);
            // Use the same normalized-vector collinearity test as
            // validateFacePointsAreCoplanar to guarantee the ear
            // triangle will pass planarity after the split.
            if ( a.length() > context.epsilon() &&
                 b.length() > context.epsilon() ) {
                Vector3Dd an = a.normalized();
                Vector3Dd bn = b.normalized();
                double aDotB = std::fabs(an.dotProduct(bn));
                double sinTheta = an.crossProduct(bn).length();
                if ( aDotB < 1.0 - context.unitVectorTolerance() ) {
                    return candidate;
                }
                if ( sinTheta > bestSinTheta ) {
                    bestSinTheta = sinTheta;
                    bestCandidate = candidate;
                }
            }
        }
        candidate = candidate->next();
        safety++;
    } while ( candidate != 0 && candidate != start &&
              safety <= loopSize + 1 );
    // No ear with sufficient angle found - fall back to the widest ear
    // above epsilon for cases where the polygon is nearly degenerate.
    if ( bestCandidate != 0 &&
         bestSinTheta > context.unitVectorTolerance() / 10.0 ) {
        return bestCandidate;
    }
    return 0;
}

/**
Returns true when the loop has at least one vertex whose position coincides
with another vertex earlier in the loop (i.e., the boundary is
self-touching / figure-8). A self-touching inner ring cannot be extracted as
a valid face via lmfkrh because the resulting face would be a degenerate
inverted membrane.
*/
bool Finisher::hasSelfTouchingVertex(_PolyhedralBoundedSolidLoop* loop,
    const ToleranceContext& tol)
{
    if ( loop == 0 ) {
        return false;
    }
    long int size = loop->halfEdgesList.size();
    long int i;
    long int j;
    for ( i = 1; i < size; i++ ) {
        for ( j = 0; j < i; j++ ) {
            if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                    loop->halfEdgesList.get(i)->startingVertex->position,
                    loop->halfEdgesList.get(j)->startingVertex->position,
                    tol) ) {
                return true;
            }
        }
    }
    return false;
}

void Finisher::extractInnerLoopsOfNonPlanarFace(PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face)
{
    long int safety;
    long int maxLoops;

    if ( face == 0 ) {
        return;
    }
    maxLoops = face->boundariesList.size();
    safety = 0;
    ToleranceContext tol = PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
    while ( face->boundariesList.size() > 1 && safety <= maxLoops ) {
        _PolyhedralBoundedSolidLoop* innerLoop;

        safety++;
        innerLoop = face->boundariesList.get(1);
        if ( innerLoop == 0 ) {
            break;
        }
        // Section 3 guard: a self-touching inner loop would produce an
        // inverted membrane face when extracted via lmfkrh. Retain it as-is.
        if ( hasSelfTouchingVertex(innerLoop, tol) ) {
            Logger::reportMessage("PolyhedralBoundedSolid", Logger::WARNING,
                "extractInnerLoopsOfNonPlanarFace",
                ("finish: skipped self-touching inner loop in face " +
                 str(face->id) +
                 "; retaining loop to avoid membrane artifact").c_str());
            break;
        }
        if ( PolyhedralBoundedSolidEulerOperators::lmfkrh(solid,
                innerLoop, solid->getMaxFaceId() + 1) == 0 ) {
            break;
        }
    }
}

void Finisher::triangulateNonPlanarFaces(PolyhedralBoundedSolid* solid)
{
    long int i;
    long int safetyCount;
    long int maxIterations;
    long int initialCount;

    lastTriangulatedFaceCount = 0;
    i = 0;
    safetyCount = 0;
    initialCount = solid->getPolygonsList().size();
    maxIterations = 50 * (initialCount + 1);
    while ( i < solid->getPolygonsList().size() &&
            safetyCount < maxIterations ) {
        _PolyhedralBoundedSolidFace* face;
        _PolyhedralBoundedSolidHalfEdge* scan;
        _PolyhedralBoundedSolidHalfEdge* ear;
        _PolyhedralBoundedSolidHalfEdge* next;
        _PolyhedralBoundedSolidHalfEdge* prev;
        int loopSize;
        int newFaceId;

        safetyCount++;
        face = solid->getPolygonsList().get(i);
        // Section 3: for multi-loop faces, check whether any inner loop is
        // self-touching BEFORE the planarity check, as a self-touching inner
        // ring can be coplanar with the outer boundary.
        if ( face->boundariesList.size() > 1 ) {
            bool hasSelfTouching = false;
            ToleranceContext tol =
                PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
            long int li;
            for ( li = 1; li < face->boundariesList.size(); li++ ) {
                if ( hasSelfTouchingVertex(face->boundariesList.get(li), tol) ) {
                    hasSelfTouching = true;
                    break;
                }
            }
            if ( hasSelfTouching ) {
                extractInnerLoopsOfNonPlanarFace(solid, face);
                if ( face->boundariesList.size() != 1 ) {
                    i++;
                    continue;
                }
                // All inner loops were extracted or guarded; face now has
                // 1 loop - fall through to planarity check.
            }
        }
        if ( PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(face) ) {
            i++;
            continue;
        }
        if ( face->boundariesList.size() > 1 ) {
            extractInnerLoopsOfNonPlanarFace(solid, face);
            if ( face->boundariesList.size() != 1 ) {
                i++;
                continue;
            }
        }
        loopSize = (int)face->boundariesList.get(0)->halfEdgesList.size();
        if ( loopSize <= 3 ) {
            // Degenerate or collinear small face.
            _PolyhedralBoundedSolidLoop* loop0 = face->boundariesList.get(0);
            // Section 9.5-degenerate: size-1 self-referential face with
            // orphaned mirror cannot be killed via lkef. Prune it directly.
            if ( loopSize == 1 &&
                 loop0->halfEdgesList.get(0) != 0 &&
                 loop0->halfEdgesList.get(0)->next() ==
                     loop0->halfEdgesList.get(0) ) {
                _PolyhedralBoundedSolidHalfEdge* h0 = loop0->halfEdgesList.get(0);
                if ( h0->mirrorHalfEdge() == 0 ||
                     h0->mirrorHalfEdge()->parentLoop == 0 ) {
                    solid->getPolygonsList().remove(i);
                    if ( h0->parentEdge != 0 ) {
                        long int edgeIdx;
                        for ( edgeIdx = 0;
                              edgeIdx < solid->getEdgesList().size();
                              edgeIdx++ ) {
                            if ( solid->getEdgesList().get(edgeIdx) ==
                                 h0->parentEdge ) {
                                solid->getEdgesList().remove(edgeIdx);
                                break;
                            }
                        }
                    }
                    continue; // don't increment i: next face slides to i
                }
            }
            // Try to absorb into adjacent face via lkef. When lkef
            // succeeds, restart the scan from index 0 so faces that absorbed
            // the triangle are re-checked.
            bool killed = false;
            long int k;
            for ( k = 0; k < loop0->halfEdgesList.size(); k++ ) {
                _PolyhedralBoundedSolidHalfEdge* h = loop0->halfEdgesList.get(k);
                if ( h != 0 &&
                     h->mirrorHalfEdge() != 0 &&
                     h->mirrorHalfEdge()->parentLoop != 0 &&
                     h->mirrorHalfEdge()->parentLoop->parentFace != face ) {
                    PolyhedralBoundedSolidEulerOperators::lkef(
                        solid, h->mirrorHalfEdge(), h);
                    killed = true;
                    break;
                }
            }
            if ( killed ) {
                i = 0;
            }
            else {
                i++;
            }
            continue;
        }
        scan = face->boundariesList.get(0)->boundaryStartHalfEdge;
        if ( scan == 0 ) {
            i++;
            continue;
        }
        ToleranceContext context = PolyhedralBoundedSolidNumericPolicy::forFace(face);
        ear = findNonDegenerateEar(scan, loopSize, context);
        if ( ear == 0 ) {
            i++;
            continue;
        }
        next = ear->next();
        prev = ear->previous();
        if ( next == 0 || prev == 0 || next == prev ||
             next->parentLoop != ear->parentLoop ||
             prev->parentLoop != ear->parentLoop ) {
            i++;
            continue;
        }
        newFaceId = (idNamespace != 0) ?
            idNamespace->nextFaceId(solid) :
            solid->getMaxFaceId() + 1;
        if ( PolyhedralBoundedSolidEulerOperators::lmef(solid, next, prev,
                newFaceId) == 0 ) {
            i++;
        }
        else {
            lastTriangulatedFaceCount++;
        }
    }
}

void Finisher::finish(PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    PolyhedralBoundedSolid* outRes,
    int op,
    int debugFlags,
    FaceList& sonfa,
    FaceList& sonfb)
{
    int i;
    int inda;
    int indb;
    _PolyhedralBoundedSolidFace* f;

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("- 4. ------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        printf("setOpFinish\n");
    }

    if ( (debugFlags & DEBUG_06_FINISH) != 0x00 ) {
        printf("TESTING FINISH: %d\n", (int)sonfa.size());
    }
    tracePipelineSummary(("finish start op=" + str(op) +
        " sonfa=" + str((int)sonfa.size()) +
        " sonfb=" + str((int)sonfb.size())).c_str());

    int oldsize = sanitizePairedFaces(sonfa, sonfb);
    // Section 11 fail-fast precondition failed: MANT1988_15_1 B-A fires the
    // legacy fallback (count>0) while still producing a valid result. Track
    // via getLastLegacyFallbackCount().
    inda = (op == INTERSECTION) ? (int)sonfa.size() : 0;
    indb = (op == UNION) ? 0 : (int)sonfb.size();

    for ( i = 0; i < oldsize; i++ ) {
        f = PolyhedralBoundedSolidEulerOperators::lmfkrh(inSolidA,
            sonfa[i]->boundariesList.get(1), inSolidA->getMaxFaceId()+1);
        sonfa.add(f);

        f = PolyhedralBoundedSolidEulerOperators::lmfkrh(inSolidB,
            sonfb[i]->boundariesList.get(1), inSolidB->getMaxFaceId()+1);
        sonfb.add(f);
    }

    if ( op == SUBTRACT ) {
        inSolidB->revert();
    }

    for ( i = 0; i < oldsize; i++ ) {
        movefac(sonfa[i+inda], outRes);
        movefac(sonfb[i+indb], outRes);
    }

    cleanup(outRes);

    for ( i = 0; i < oldsize; i++ ) {
        PolyhedralBoundedSolidEulerOperators::lkfmrh(outRes, sonfa[i+inda],
            sonfb[i+indb]);
        PolyhedralBoundedSolidTopologyEditing::loopGlue(outRes, sonfa[i+inda]);
    }
    cleanup(outRes);
    triangulateNonPlanarFaces(outRes);
    PolyhedralBoundedSolidTopologyEditing::compactIds(outRes);
    tracePipelineSummary(("finish end outRes faces=" +
        str((int)outRes->getPolygonsList().size()) +
        " edges=" + str((int)outRes->getEdgesList().size()) +
        " vertices=" + str((int)outRes->getVerticesList().size())).c_str());
}
