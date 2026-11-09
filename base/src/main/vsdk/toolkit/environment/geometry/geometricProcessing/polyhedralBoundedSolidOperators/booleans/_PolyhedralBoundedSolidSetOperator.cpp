//= References:                                                             =
//= [MANT1986] Mantyla Martti. "Boolean Operations of 2-Manifolds through   =
//=     Vertex Neighborhood Classification". ACM Transactions on Graphics,  =
//=     Vol. 5, No. 1, January 1986, pp. 1-29.                              =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

#include "java/lang/Exception.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include "java/util/TreeSet.txx"
#include <cstdio>

#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_PolyhedralBoundedSolidSetOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationContext.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationTrace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetGeometricPredicateProcessor.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetFinisher.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetNullEdgesConnector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidOrthogonalProfileFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidProfileDifferenceFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidAxisAlignedCellFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidFallbackGeometry.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidOffsetCylinderFallback.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetOperator SetOperator;

int SetOperator::debugFlags = 0;
SetOperator::DebugSolidExporter SetOperator::debugSolidExporter = 0;
_PolyhedralBoundedSolidIdNamespace* SetOperator::ownedNamespace = 0;

namespace {

const char* const CLASS_NAME = "_PolyhedralBoundedSolidSetOperator";

class HalfEdgePair {
public:
    _PolyhedralBoundedSolidHalfEdge* from;
    _PolyhedralBoundedSolidHalfEdge* to;

    HalfEdgePair() : from(nullptr), to(nullptr) {}

    HalfEdgePair(_PolyhedralBoundedSolidHalfEdge* from,
                 _PolyhedralBoundedSolidHalfEdge* to) : from(from), to(to) {}

    bool operator<(const HalfEdgePair& other) const
    {
        return java::naturalOrderLess(from, other.from) ||
            (from == other.from && java::naturalOrderLess(to, other.to));
    }

    bool operator==(const HalfEdgePair& other) const
    {
        return from == other.from && to == other.to;
    }
};

java::String str(long long value)
{
    return java::String::valueOf(value);
}

void tracePipelineSummary(const java::String& message)
{
    _SetOperationTrace::tracePipelineSummary(message.c_str());
}

bool isPipelineSummaryTraceEnabled()
{
    return _SetOperationTrace::isPipelineSummaryTraceEnabled();
}

/**
Emulates Java `Arrays.toString(double[])` for the six bounds values.
*/
java::String boundsText(const double* bounds)
{
    java::String text = "[";
    for ( int i = 0; i < 6; i++ ) {
        if ( i > 0 ) {
            text += ", ";
        }
        text += java::Double::toString(bounds[i]).c_str();
    }
    return text + "]";
}

template <typename T>
void unlistShared(java::ArrayList<T*>& from, const java::ArrayList<T*>& reference)
{
    java::TreeSet<T*> referenced;
    long int i;

    for ( i = 0; i < reference.size(); i++ ) {
        referenced.add(reference.get(i));
    }
    i = 0;
    while ( i < from.size() ) {
        if ( referenced.contains(from.get(i)) ) {
            from.remove(i);
        }
        else {
            i++;
        }
    }
}

}

//= Configuration and helpers =======================================

void SetOperator::setDebugSolidExporter(DebugSolidExporter exporter)
{
    debugSolidExporter = exporter;
}

void SetOperator::updmaxnames(PolyhedralBoundedSolid* solidToUpdate,
    PolyhedralBoundedSolid* referenceSolid)
{
    _PolyhedralBoundedSolidIdNamespace::updmaxnames(solidToUpdate, referenceSolid);
}

int SetOperator::nextVertexId(PolyhedralBoundedSolid* current,
    PolyhedralBoundedSolid* other)
{
    return _PolyhedralBoundedSolidIdNamespace::nextVertexId(current, other,
        idNamespace);
}

/**
Initial vertex intersection detector for the set operations algorithm
(big phase 0). Following program [MANT1988].15.2. After generation,
coincident intersection vertices are welded in both solids and stale
entries are pruned from sonva/sonvb.
*/
void SetOperator::setOpGenerate(_SetOperationContext& ctx,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    _PolyhedralBoundedSolidSetIntersector::GenerationResult generation =
        _PolyhedralBoundedSolidSetIntersector::setOpGenerate(inSolidA, inSolidB);
    ctx.sonvv = generation.sonvv();
    ctx.sonva = generation.sonva();
    ctx.sonvb = generation.sonvb();

    weldIntersectionVertices(ctx, inSolidA, inSolidB);
}

/**
Post-Generate weld pass: collapses spatially coincident vertices introduced
during setOpGenerate in each solid, then removes from sonva/sonvb any entry
whose vertex was merged away by lkev.
*/
void SetOperator::weldIntersectionVertices(_SetOperationContext& ctx,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    int weldedA = PolyhedralBoundedSolidTopologyEditing::weldCoincidentVertices(
        inSolidA, numericContext);
    int weldedB = PolyhedralBoundedSolidTopologyEditing::weldCoincidentVertices(
        inSolidB, numericContext);

    if ( weldedA > 0 ) {
        Logger::reportMessage(java::String(), Logger::DEBUG,
            "weldIntersectionVertices",
            ("setOpGenerate weld: " + str(weldedA) +
             " vertex pair(s) collapsed in solidA").c_str());
        pruneStaleVertexFaceEntries(ctx.sonva, inSolidA);
    }
    if ( weldedB > 0 ) {
        Logger::reportMessage(java::String(), Logger::DEBUG,
            "weldIntersectionVertices",
            ("setOpGenerate weld: " + str(weldedB) +
             " vertex pair(s) collapsed in solidB").c_str());
        pruneStaleVertexFaceEntries(ctx.sonvb, inSolidB);
    }
}

/**
Removes entries from `list` whose vertex is no longer present in `solid`
(after lkev merges two vertices, the removed vertex is detached from the
solid's vertices list).
*/
void SetOperator::pruneStaleVertexFaceEntries(
    java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexFace>& list,
    PolyhedralBoundedSolid* solid)
{
    java::TreeSet<_PolyhedralBoundedSolidVertex*> present;
    long int k;
    long int i;

    for ( k = 0; k < solid->getVerticesList().size(); k++ ) {
        present.add(solid->getVerticesList().get(k));
    }
    i = 0;
    while ( i < list.size() ) {
        if ( !present.contains(list[i].v) ) {
            list.remove(i);
        }
        else {
            i++;
        }
    }
}

Vector3Dd SetOperator::inside(_PolyhedralBoundedSolidHalfEdge* he)
{
    Vector3Dd middle;
    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd n;

    a = (he->next())->startingVertex->position.subtract(he->startingVertex->position);
    b = (he->previous())->startingVertex->position.subtract(he->startingVertex->position);
    a = a.normalized();
    b = b.normalized();

    InfinitePlane* plane = he->parentLoop->parentFace->getContainingPlane();
    n = plane->getNormal();
    delete plane;

    middle = n.crossProduct(a);
    middle = middle.normalized();

    return middle;
}

/**
Checks if two coplanar sectors overlaps, following section [MANT1988].15.6.2.
*/
bool SetOperator::sectoroverlap(
    const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex& na,
    const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex& nb)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor
        ::sectoroverlap(na, nb, (debugFlags & DEBUG_04_VERTEXVERTEXCLASIFFIER) != 0);
}

bool SetOperator::colinearVectorsWithDirection(const Vector3Dd& a,
    const Vector3Dd& b)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor
        ::colinearVectorsWithDirection(a, b);
}

/**
Normalizes one endpoint for `separateEdgeSequence` when a previous null
strut edge was already inserted on the same vertex neighborhood.
*/
_PolyhedralBoundedSolidHalfEdge* SetOperator::recoverEdgeSequenceEndpointFromStrut(
    _PolyhedralBoundedSolidHalfEdge* endpoint, bool isFromEndpoint)
{
    _PolyhedralBoundedSolidHalfEdge* prev;

    if ( endpoint == 0 ) {
        return 0;
    }

    prev = endpoint->previous();
    if ( prev == 0 || prev->parentEdge == 0 ) {
        return endpoint;
    }

    if ( !nulledge(prev) || !strutnulledge(prev) ) {
        return endpoint;
    }

    if ( isFromEndpoint ) {
        if ( prev == prev->parentEdge->leftHalf ) {
            return prev->previous();
        }
    }
    else {
        if ( prev == prev->parentEdge->rightHalf ) {
            return prev->previous();
        }
    }
    return endpoint;
}

SetOperator::SeparateEdgeSequenceResult SetOperator::separateEdgeSequence(
    _PolyhedralBoundedSolidHalfEdge* from,
    _PolyhedralBoundedSolidHalfEdge* to,
    int type,
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB)
{
    bool trace = (debugFlags & DEBUG_04_VERTEXVERTEXCLASIFFIER) != 0x00;

    //-----------------------------------------------------------------
    if ( trace ) {
        printf("      SEPARATEEDGESEQUENCE %d\n", type);
        printf("        From: %s\n", from != 0 ? from->toString().c_str() : "null");
        printf("        To: %s\n", to != 0 ? to->toString().c_str() : "null");
    }

    if ( from == 0 || to == 0 ) {
        Logger::reportMessage(java::String(), Logger::WARNING,
            "separateEdgeSequence",
            "Unexpected case: null halfedges; skipping LMEV.");
        return FAILED_NULL_INPUT;
    }

    PolyhedralBoundedSolid* s = from->parentLoop->parentFace->parentSolid;

    if ( s != to->parentLoop->parentFace->parentSolid ) {
        Logger::reportMessage(java::String(), Logger::WARNING,
            "separateEdgeSequence",
            "Unexpected case: halfedges on different solids; skipping LMEV.");
        return FAILED_DIFFERENT_SOLIDS;
    }

    //-----------------------------------------------------------------
    // Recover from null edges already inserted. Cases A/B follow null-edge
    // struts inserted previously; cases C/D/E step backwards in the loop
    // until the two starts coincide. Each iteration must produce an unseen
    // (from, to) pair - repeating a pair proves divergence.
    java::TreeSet<HalfEdgePair> visitedConfigurations;
    bool changed;
    do {
        if ( !visitedConfigurations.add(HalfEdgePair(from, to)) ) {
            Logger::reportMessage(java::String(), Logger::WARNING,
                "separateEdgeSequence",
                "Cycle detected in endpoint recovery (cases A-E "
                "did not converge); skipping LMEV to keep B-rep valid.");
            return FAILED_CYCLE_DETECTED;
        }

        changed = false;

        _PolyhedralBoundedSolidHalfEdge* recoveredFrom =
            recoverEdgeSequenceEndpointFromStrut(from, true);
        if ( recoveredFrom != from ) {
            from = recoveredFrom;
            changed = true;
            if ( trace ) {
                printf("        Recovered edge sequence case A\n");
            }
        }

        _PolyhedralBoundedSolidHalfEdge* recoveredTo =
            recoverEdgeSequenceEndpointFromStrut(to, false);
        if ( recoveredTo != to ) {
            to = recoveredTo;
            changed = true;
            if ( trace ) {
                printf("        Recovered edge sequence case B\n");
            }
        }

        if ( from->startingVertex != to->startingVertex ) {
            _PolyhedralBoundedSolidHalfEdge* fromPrev = from->previous();
            _PolyhedralBoundedSolidHalfEdge* toPrev = to->previous();

            if ( fromPrev != 0 && toPrev != 0 &&
                 fromPrev->parentEdge != 0 && toPrev->parentEdge != 0 &&
                 fromPrev == toPrev->mirrorHalfEdge() ) {
                from = fromPrev;
                changed = true;
                if ( trace ) {
                    printf("        Recovered edge sequence case C\n");
                }
            }
            else if ( fromPrev != 0 &&
                      fromPrev->startingVertex == to->startingVertex ) {
                from = fromPrev;
                changed = true;
                if ( trace ) {
                    printf("        Recovered edge sequence case D\n");
                }
            }
            else if ( toPrev != 0 &&
                      toPrev->startingVertex == from->startingVertex ) {
                to = toPrev;
                changed = true;
                if ( trace ) {
                    printf("        Recovered edge sequence case E\n");
                }
            }
        }
    } while ( changed );

    if ( from->startingVertex != to->startingVertex ) {
        Logger::reportMessage(java::String(), Logger::WARNING,
            "separateEdgeSequence",
            "Unable to recover endpoint pairing after A-E normalization; "
            "skipping LMEV.");
        return FAILED_NO_PAIRING_REACHED;
    }

    //-----------------------------------------------------------------
    if ( trace && (debugFlags & DEBUG_99_SHOWOPERATIONS) != 0x00 ) {
        printf("       -> LMEV (Separate edge sequence):\n");
        printf("          . H1: %s\n", to->toString().c_str());
        printf("          . H2: %s\n", from->toString().c_str());
    }

    int id = nextVertexId(inSolidA, inSolidB);

    PolyhedralBoundedSolidEulerOperators::lmev(s, to, from, id,
        to->startingVertex->position);

    if ( trace && (debugFlags & DEBUG_99_SHOWOPERATIONS) != 0x00 ) {
        printf("          . New vertex: %d\n", id);
    }

    // The live pipeline records the created null edge from within
    // _PolyhedralBoundedSolidSetClassifier; this overload is retained only to
    // validate the cycle-detection failure modes (as in Java).
    return OK;
}

bool SetOperator::nulledge(_PolyhedralBoundedSolidHalfEdge* he)
{
    return PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
        he->startingVertex->position, he->next()->startingVertex->position,
        numericContext);
}

/**
Borrowed from [.wMANT2008].
*/
bool SetOperator::strutnulledge(_PolyhedralBoundedSolidHalfEdge* he)
{
    return he == he->mirrorHalfEdge()->next() ||
           he == he->mirrorHalfEdge()->previous();
}

/**
Main control algorithm for the big phases 1 and 2, following section
[MANT1988].16.6.1. and program [MANT1988].15.5.
*/
void SetOperator::setOpClassify(_SetOperationContext& ctx, int op,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    _PolyhedralBoundedSolidSetClassifier::runSetOpClassify(
        op, inSolidA, inSolidB, debugFlags, ctx);
}

/**
Diagnostic: scans a solid for boundary loops whose vertices are
coincident-but-distinct (self-touching / figure-8). Gated behind the
pipeline-summary trace property.
*/
void SetOperator::traceSelfTouchingLoops(PolyhedralBoundedSolid* solid,
    const char* label)
{
    if ( !isPipelineSummaryTraceEnabled() || solid == 0 ) {
        return;
    }
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext tol =
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
    long int fi;
    for ( fi = 0; fi < solid->getPolygonsList().size(); fi++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(fi);
        long int li;
        for ( li = 0; li < face->boundariesList.size(); li++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(li);
            long int sz = loop->halfEdgesList.size();
            long int a;
            for ( a = 0; a < sz; a++ ) {
                long int b;
                for ( b = a + 1; b < sz; b++ ) {
                    _PolyhedralBoundedSolidHalfEdge* ha = loop->halfEdgesList.get(a);
                    _PolyhedralBoundedSolidHalfEdge* hb = loop->halfEdgesList.get(b);
                    if ( ha->startingVertex->id != hb->startingVertex->id &&
                         PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                             ha->startingVertex->position,
                             hb->startingVertex->position, tol) ) {
                        long int gap = b - a;
                        bool adjacent = (gap == 1) || (gap == sz - 1);
                        printf("[SelfTouch] %s face=%d loop=%d size=%d "
                            "idx[%d]v%d==idx[%d]v%d%s at (%.4f,%.4f,%.4f)\n",
                            label, face->id, (int)li, (int)sz,
                            (int)a, ha->startingVertex->id,
                            (int)b, hb->startingVertex->id,
                            adjacent ? " ADJACENT(zero-len-edge)" :
                                " NON-ADJACENT(pinch)",
                            ha->startingVertex->position.x(),
                            ha->startingVertex->position.y(),
                            ha->startingVertex->position.z());
                    }
                }
            }
        }
    }
}

/**
Splits every boundary loop that is self-touching (pinched) into simple loops
via `lmef`. Run on each operand right after setOpGenerate and before
setOpClassify; size-2 strut loops are excluded.
*/
void SetOperator::splitSelfTouchingLoops(PolyhedralBoundedSolid* solid)
{
    if ( solid == 0 ) {
        return;
    }
    bool tracing = isPipelineSummaryTraceEnabled();
    int splitsFired = 0;
    long int fi = 0;
    while ( fi < solid->getPolygonsList().size() ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(fi);
        bool splitDone = false;
        long int li = 0;
        while ( li < face->boundariesList.size() && !splitDone ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(li);
            long int sz = loop->halfEdgesList.size();
            if ( sz <= 2 ) {
                li++;
                continue;
            }
            long int a;
            for ( a = 0; a < sz && !splitDone; a++ ) {
                long int b;
                for ( b = a + 2; b < sz; b++ ) {
                    if ( a == 0 && b == sz - 1 ) {
                        continue; // adjacent via wrap-around
                    }
                    _PolyhedralBoundedSolidHalfEdge* ha = loop->halfEdgesList.get(a);
                    _PolyhedralBoundedSolidHalfEdge* hb = loop->halfEdgesList.get(b);
                    if ( ha->startingVertex->id != hb->startingVertex->id &&
                         PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                             ha->startingVertex->position,
                             hb->startingVertex->position,
                             numericContext) ) {
                        int newId = (idNamespace != 0) ?
                            idNamespace->nextFaceId(solid) :
                            solid->getMaxFaceId() + 1;
                        _PolyhedralBoundedSolidFace* newFace =
                            PolyhedralBoundedSolidEulerOperators::lmef(
                                solid, ha, hb, newId);
                        if ( newFace != 0 ) {
                            splitsFired++;
                            if ( tracing ) {
                                tracePipelineSummary(
                                    "splitSelfTouchingLoops #" + str(splitsFired) +
                                    " face=" + str(face->id) + " loop=" + str(li) +
                                    " sz=" + str(sz) +
                                    " idx[" + str(a) + "]v" + str(ha->startingVertex->id) +
                                    "==idx[" + str(b) + "]v" + str(hb->startingVertex->id) +
                                    " -> newFace=" + str(newFace->id));
                            }
                            splitDone = true;
                        }
                        break;
                    }
                }
            }
            if ( !splitDone ) {
                li++;
            }
        }
        if ( !splitDone ) {
            fi++;
        }
        // If splitDone: stay at the same fi so the modified face is
        // re-examined for any remaining pinches.
    }
    if ( tracing && splitsFired > 0 ) {
        tracePipelineSummary("splitSelfTouchingLoops total=" + str(splitsFired) +
            " faces-after=" + str(solid->getPolygonsList().size()));
    }
}

void SetOperator::setOpConnect(_SetOperationContext& ctx, int op)
{
    _PolyhedralBoundedSolidSetNullEdgesConnector connector;
    _PolyhedralBoundedSolidSetNullEdgesConnector::ConnectResult result =
        connector.connect(op, debugFlags, ctx.sonea, ctx.soneb);
    ctx.sonfa = result.sonfa();
    ctx.sonfb = result.sonfb();
}

/**
Answer integrator for the set operations algorithm (big phase 4).
Following program [MANT1988].15.15.
*/
void SetOperator::setOpFinish(_SetOperationContext& ctx,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB,
    PolyhedralBoundedSolid* outRes, int op)
{
    _PolyhedralBoundedSolidSetFinisher::finish(
        inSolidA, inSolidB, outRes, op, debugFlags, ctx.sonfa, ctx.sonfb);
}

void SetOperator::debugSolid(PolyhedralBoundedSolid* solid, const char* pattern)
{
    printf("**** DEBUGGING SOLID INFORMATION WRITEN TO FILES %s ****\n", pattern);
    java::String fileName = java::String(pattern) + ".txt";
    FILE* fd = fopen(fileName.c_str(), "w");
    if ( fd == 0 ) {
        return;
    }
    if ( debugSolidExporter != 0 ) {
        debugSolidExporter(solid, pattern);
    }
    fprintf(fd, "%s\n", solid->toString().c_str());
    fclose(fd);
}

/**
Following program [MANT1988].15.1.
*/
void SetOperator::postProcessResult(PolyhedralBoundedSolid* res,
    bool maximizeResultFaces)
{
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(res);
    PolyhedralBoundedSolidTopologyEditing::compactIds(res);
    if ( maximizeResultFaces ) {
        PolyhedralBoundedSolidTopologyEditing::maximizeFaces(res);
        _PolyhedralBoundedSolidSetFinisher::triangulateNonPlanarFaces(res);
        PolyhedralBoundedSolidTopologyEditing::compactIds(res);
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(res);
}

PolyhedralBoundedSolid* SetOperator::completeSetOpResult(
    PolyhedralBoundedSolid* result,
    const java::String& operandADiagnostics,
    const java::String& operandBDiagnostics,
    int op,
    const char* resultPath,
    bool maximizeResultFaces,
    bool doStrictValidation)
{
    if ( result == 0 ) {
        throw java::IllegalStateException(java::String("Boolean operation returned null: op=") +
            operationName(op).c_str() + ", path=" + resultPath);
    }
    if ( result->getPolygonsList().size() > 0 ) {
        postProcessResult(result, maximizeResultFaces);
    }
    if ( doStrictValidation ) {
        java::String strictMessage;
        if ( !PolyhedralBoundedSolidValidationEngine::validateStrict(
                 result, &strictMessage) ) {
            PolyhedralBoundedSolidTopologySummary topology =
                PolyhedralBoundedSolidTopologySummary::from(result);
            throw java::IllegalStateException(
                java::String("Strict boolean result validation failed: op=") +
                operationName(op).c_str() + ", path=" + resultPath +
                ", operandA=" + operandADiagnostics.c_str() +
                ", operandB=" + operandBDiagnostics.c_str() +
                ", result=" + cardinalitiesAndBounds(result).c_str() +
                ", " + topology.toString().c_str() + "\n" +
                strictMessage.c_str());
        }
    }
    return result;
}

java::String SetOperator::operationName(int op)
{
    if ( op == UNION ) {
        return "UNION";
    }
    if ( op == INTERSECTION ) {
        return "INTERSECTION";
    }
    if ( op == SUBTRACT ) {
        return "SUBTRACT";
    }
    return ("UNKNOWN(" + str(op) + ")").c_str();
}

java::String SetOperator::cardinalitiesAndBounds(PolyhedralBoundedSolid* solid)
{
    if ( solid == 0 ) {
        return "null";
    }
    double* bounds = solid->getMinMax();
    java::String text = "{faces=" + str(solid->getPolygonsList().size()) +
        ", edges=" + str(solid->getEdgesList().size()) +
        ", vertices=" + str(solid->getVerticesList().size()) +
        ", bounds=" + boundsText(bounds) + "}";
    delete[] bounds;
    return text.c_str();
}

/**
Deep copy of a solid, preserving ids, list orders and the half-edge
structure (Java uses object serialization for this).
@return a new solid owned by the caller, or null
*/
PolyhedralBoundedSolid* SetOperator::deepCloneSolid(PolyhedralBoundedSolid* solid,
    const char* solidLabel)
{
    java::TreeMap<_PolyhedralBoundedSolidVertex*, _PolyhedralBoundedSolidVertex*> vertexMap;
    java::TreeMap<_PolyhedralBoundedSolidEdge*, _PolyhedralBoundedSolidEdge*> edgeMap;
    java::TreeMap<_PolyhedralBoundedSolidHalfEdge*, _PolyhedralBoundedSolidHalfEdge*> halfEdgeMap;
    long int i;
    long int j;
    long int k;

    (void)solidLabel;
    if ( solid == 0 ) {
        return 0;
    }

    PolyhedralBoundedSolid* copy = new PolyhedralBoundedSolid();
    copy->setMaxVertexId(solid->getMaxVertexId());
    copy->setMaxFaceId(solid->getMaxFaceId());
    copy->setValidationState(solid->isValid());

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        _PolyhedralBoundedSolidVertex* nv =
            new _PolyhedralBoundedSolidVertex(v->position, v->id);
        nv->debugColor = v->debugColor;
        copy->getVerticesList().add(nv);
        vertexMap.put(v, nv);
    }
    for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
        _PolyhedralBoundedSolidEdge* e = solid->getEdgesList().get(i);
        _PolyhedralBoundedSolidEdge* ne = new _PolyhedralBoundedSolidEdge();
        ne->id = e->id;
        ne->debugColor = e->debugColor;
        copy->getEdgesList().add(ne);
        edgeMap.put(e, ne);
    }
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* f = solid->getPolygonsList().get(i);
        _PolyhedralBoundedSolidFace* nf =
            new _PolyhedralBoundedSolidFace(copy, f->id);
        copy->getPolygonsList().add(nf);
        for ( j = 0; j < f->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* l = f->boundariesList.get(j);
            _PolyhedralBoundedSolidLoop* nl = new _PolyhedralBoundedSolidLoop(nf);
            for ( k = 0; k < l->halfEdgesList.size(); k++ ) {
                _PolyhedralBoundedSolidHalfEdge* he = l->halfEdgesList.get(k);
                _PolyhedralBoundedSolidHalfEdge* nhe =
                    new _PolyhedralBoundedSolidHalfEdge(
                        he->startingVertex != 0 ? vertexMap.getOrDefault(he->startingVertex, nullptr) : 0,
                        nl);
                nhe->id = he->id;
                if ( he->parentEdge != 0 ) {
                    _PolyhedralBoundedSolidEdge** found =
                        edgeMap.get(he->parentEdge);
                    if ( found == nullptr ) {
                        // Edge not listed by the solid: copy it on demand
                        _PolyhedralBoundedSolidEdge* ne = new _PolyhedralBoundedSolidEdge();
                        ne->id = he->parentEdge->id;
                        ne->debugColor = he->parentEdge->debugColor;
                        edgeMap.put(he->parentEdge, ne);
                        nhe->parentEdge = ne;
                    }
                    else {
                        nhe->parentEdge = *found;
                    }
                }
                nl->halfEdgesList.add(nhe);
                halfEdgeMap.put(he, nhe);
            }
            nl->boundaryStartHalfEdge = l->boundaryStartHalfEdge != 0 ?
                halfEdgeMap.getOrDefault(l->boundaryStartHalfEdge, nullptr) : 0;
        }
    }

    // Edge halves and vertex emanating references, now that half-edges exist
    for ( long int it = 0; it < edgeMap.size(); it++ ) {
        _PolyhedralBoundedSolidEdge* e = edgeMap.getKey(it);
        _PolyhedralBoundedSolidEdge* ne = edgeMap.getValue(it);
        ne->rightHalf = (e->rightHalf != 0 && halfEdgeMap.containsKey(e->rightHalf)) ?
            *halfEdgeMap.get(e->rightHalf) : 0;
        ne->leftHalf = (e->leftHalf != 0 && halfEdgeMap.containsKey(e->leftHalf)) ?
            *halfEdgeMap.get(e->leftHalf) : 0;
    }
    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        _PolyhedralBoundedSolidVertex* nv = vertexMap.getOrDefault(v, nullptr);
        nv->emanatingHalfEdge = (v->emanatingHalfEdge != 0 &&
            halfEdgeMap.containsKey(v->emanatingHalfEdge)) ?
            *halfEdgeMap.get(v->emanatingHalfEdge) : 0;
    }
    return copy;
}

bool SetOperator::hasDegenerateFace(PolyhedralBoundedSolid* solid)
{
    long int i;

    if ( solid == 0 ) {
        return true;
    }
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        if ( face->boundariesList.size() < 1 ||
             face->boundariesList.get(0)->halfEdgesList.size() < 3 ) {
            return true;
        }
        InfinitePlane* plane = face->getContainingPlane();
        if ( plane == 0 ) {
            return true;
        }
        delete plane;
    }
    return false;
}

bool SetOperator::shouldUseAxisAlignedCellBooleanFallback(
    PolyhedralBoundedSolid* fallback, PolyhedralBoundedSolid* result)
{
    if ( fallback == 0 || fallback->getPolygonsList().size() <= 0 ) {
        return false;
    }
    if ( result == 0 || result->getPolygonsList().size() <= 0 ) {
        return true;
    }
    if ( hasDegenerateFace(result) ) {
        return true;
    }
    return false;
}

bool SetOperator::hasIncompleteConnectState()
{
    return _PolyhedralBoundedSolidSetNullEdgesConnector::getLastLooseACount() > 0 ||
        _PolyhedralBoundedSolidSetNullEdgesConnector::getLastLooseBCount() > 0;
}

bool SetOperator::isStructurallyUsableSetOpResult(PolyhedralBoundedSolid* result)
{
    if ( result == 0 ||
         result->getPolygonsList().size() <= 0 ||
         hasDegenerateFace(result) ) {
        return false;
    }
    return PolyhedralBoundedSolidValidationEngine::validateIntermediate(result);
}

bool SetOperator::hasBasicSetOpShapeData(PolyhedralBoundedSolid* result)
{
    return result != 0 &&
        result->getPolygonsList().size() > 0 &&
        result->getEdgesList().size() > 0 &&
        result->getVerticesList().size() > 0;
}

bool SetOperator::hasSameShapeData(PolyhedralBoundedSolid* first,
    PolyhedralBoundedSolid* second)
{
    if ( !hasBasicSetOpShapeData(first) || !hasBasicSetOpShapeData(second) ||
         first->getPolygonsList().size() != second->getPolygonsList().size() ||
         first->getEdgesList().size() != second->getEdgesList().size() ||
         first->getVerticesList().size() != second->getVerticesList().size() ) {
        return false;
    }
    double* a = first->getMinMax();
    double* b = second->getMinMax();
    bool same = _PolyhedralBoundedSolidFallbackGeometry::boundsMatch(a, b);
    delete[] a;
    delete[] b;
    return same;
}

void SetOperator::unlistSharedNodes(PolyhedralBoundedSolid* from,
    PolyhedralBoundedSolid* reference)
{
    if ( from == 0 || reference == 0 || from == reference ) {
        return;
    }
    unlistShared(from->getPolygonsList(), reference->getPolygonsList());
    unlistShared(from->getEdgesList(), reference->getEdgesList());
    unlistShared(from->getVerticesList(), reference->getVerticesList());
}

void SetOperator::discardIntermediateResult(PolyhedralBoundedSolid* result,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    if ( result == 0 ) {
        return;
    }
    unlistSharedNodes(result, inSolidA);
    unlistSharedNodes(result, inSolidB);
    delete result;
}

//= Main algorithm ==================================================

/**
Runs a boolean operation following program [MANT1988].15.1, with
configurable strict B-Rep validation as a final result postcondition.
*/
PolyhedralBoundedSolid* SetOperator::setOp(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    int op,
    bool withDebug,
    bool maximizeResultFaces,
    bool doStrictValidation)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolids(inSolidA, inSolidB);
    setNumericContext(&context);
    _PolyhedralBoundedSolidSetOperatorNullEdge::setNumericContext(&numericContext);

    if ( withDebug ) {
        debugFlags = 0
          | DEBUG_01_STRUCTURE
          | DEBUG_02_GENERATOR
          | DEBUG_03_VERTEXFACECLASIFFIER
          | DEBUG_04_VERTEXVERTEXCLASIFFIER
          | DEBUG_05_CONNECT
          | DEBUG_06_FINISH
          | DEBUG_99_SHOWOPERATIONS
          ;
    }
    else {
        debugFlags = 0;
    }

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("= [START OF SETOP REPORT] =================================================================================================================================\n");
        printf("Dumping debug log for _PolyhedralBoundedSolidSetOperator.setOp.\n");
        printf("The algorithm structure is:\n");
        printf("  0. Calculate vertex/face and vertex/vertex crossings.\n");
        printf("  1. Classify and split for vertex/face cases.\n");
        printf("  2. Classify and split for vertex/vertex cases.\n");
        printf("  3. Connect.\n");
        printf("  4. Finish.\n");
    }

    //-----------------------------------------------------------------
    PolyhedralBoundedSolid* res = new PolyhedralBoundedSolid();
    _PolyhedralBoundedSolidProfileDifferenceFallbackSpec* profileDifferenceFallback;
    _PolyhedralBoundedSolidOffsetCylinderFallback::OffsetCylinderDifferenceFallbackSpec*
        offsetCylinderDifferenceFallbackSpec;
    PolyhedralBoundedSolid* offsetCylinderDifferenceFallback;
    PolyhedralBoundedSolid* axisAlignedCellBooleanFallback;
    PolyhedralBoundedSolid* orthogonalProfileBooleanFallback;
    bool fallbackProvidedResult;
    java::String resultPath;
    java::String operandADiagnostics;
    java::String operandBDiagnostics;

    _SetOperationContext ctx;
    offsetCylinderDifferenceFallback = 0;
    fallbackProvidedResult = false;
    resultPath = "normal-pipeline";
    operandADiagnostics = cardinalitiesAndBounds(inSolidA);
    operandBDiagnostics = cardinalitiesAndBounds(inSolidB);

    //-----------------------------------------------------------------
    if ( withDebug ) {
        debugSolid(inSolidA, "outputA_stage00");
        debugSolid(inSolidB, "outputB_stage00");
    }

    PolyhedralBoundedSolidTopologyEditing::compactIds(inSolidA);
    PolyhedralBoundedSolidTopologyEditing::compactIds(inSolidB);
    PolyhedralBoundedSolidTopologyEditing::maximizeFaces(inSolidA);
    PolyhedralBoundedSolidTopologyEditing::maximizeFaces(inSolidB);
    PolyhedralBoundedSolidTopologyEditing::compactIds(inSolidA);
    PolyhedralBoundedSolidTopologyEditing::compactIds(inSolidB);
    java::String booleanInputMsg;
    if ( !PolyhedralBoundedSolidValidationEngine::validateBooleanInputs(
             inSolidA, inSolidB, &booleanInputMsg) ) {
        Logger::reportMessage(java::String(), Logger::WARNING, "setOp",
            java::String("Boolean input validation failed:\n") + booleanInputMsg);
    }
    else if ( booleanInputMsg.length() > 0 ) {
        Logger::reportMessage(java::String(), Logger::DEBUG, "setOp",
            java::String("Boolean input pre-processing:\n") + booleanInputMsg);
    }
    PolyhedralBoundedSolidTopologyEditing::compactIds(inSolidA);
    PolyhedralBoundedSolidTopologyEditing::compactIds(inSolidB);
    updmaxnames(inSolidB, inSolidA);
    // As Java, the namespace stays active after this operation (until the
    // next setOp replaces it); the operator owns it
    _PolyhedralBoundedSolidIdNamespace* previousNamespace = ownedNamespace;
    ownedNamespace = new _PolyhedralBoundedSolidIdNamespace(inSolidA, inSolidB);
    setIdNamespace(ownedNamespace);
    delete previousNamespace;
    context = PolyhedralBoundedSolidNumericPolicy::forSolids(inSolidA, inSolidB);
    setNumericContext(&context);
    _PolyhedralBoundedSolidSetOperatorNullEdge::setNumericContext(&numericContext);
    profileDifferenceFallback = _PolyhedralBoundedSolidProfileDifferenceFallback
        ::prepareProfileDifferenceFallbackSpec(inSolidA, inSolidB, op);
    offsetCylinderDifferenceFallbackSpec =
        _PolyhedralBoundedSolidOffsetCylinderFallback
            ::prepareOffsetCylinderDifferenceFallbackSpec(inSolidA, inSolidB, op);
    axisAlignedCellBooleanFallback =
        _PolyhedralBoundedSolidAxisAlignedCellFallback
            ::buildAxisAlignedCellBooleanFallback(inSolidA, inSolidB, op);
    orthogonalProfileBooleanFallback =
        _PolyhedralBoundedSolidOrthogonalProfileFallback
            ::buildOrthogonalProfileBooleanFallback(inSolidA, inSolidB, op);

    if ( withDebug ) {
        debugSolid(inSolidA, "outputA_stage01");
        debugSolid(inSolidB, "outputB_stage01");
    }

    // Per-setOp memo shared across all "no real intersection" preflights:
    // operands are not mutated until setOpGenerate runs (below), so the
    // heavy classification/overlap/edge-face predicates are computed once
    // and reused.
    PreflightCache preflightCache =
        _PolyhedralBoundedSolidSetNonIntersectingClassifier
            ::newPreflightCache(inSolidA, inSolidB);
    bool preflightResolved = false;

    PolyhedralBoundedSolid* coplanarAreaContactResult =
        _PolyhedralBoundedSolidSetNonIntersectingClassifier
            ::runPartialCoplanarFaceAreaCase(inSolidA, inSolidB, res, op,
                preflightCache);
    if ( coplanarAreaContactResult != 0 ) {
        res = coplanarAreaContactResult;
        resultPath = "partial-coplanar-area-preflight";
        preflightResolved = true;
    }

    if ( !preflightResolved &&
         _PolyhedralBoundedSolidSetNonIntersectingClassifier
             ::runTouchingOnlyPreflightCase(inSolidA, inSolidB, preflightCache) ) {
        res = _PolyhedralBoundedSolidSetNonIntersectingClassifier
            ::runSetOpNoIntersectionCase(inSolidA, inSolidB, res, op,
                preflightCache);
        resultPath = "touching-only-preflight";
        preflightResolved = true;
    }

    // Section 7.3.1.A - Degenerate identity preflight: when A and B are
    // geometrically identical, the classifier marks every face of both as
    // "inside the other" and the regular pipeline collapses to empty,
    // breaking A U A and A ^ A. Dispatch directly per set-theoretic
    // identity. MUST run before the containment preflight.
    if ( !preflightResolved &&
         PolyhedralBoundedSolidValidationEngine::areGeometricallyIdentical(
             inSolidA, inSolidB, numericContext.bigEpsilon()) ) {
        tracePipelineSummary("setOp identity-preflight A=B op=" + str(op));
        if ( op == UNION || op == INTERSECTION ) {
            PolyhedralBoundedSolid* clone =
                deepCloneSolid(inSolidA, "identity-preflight clone");
            if ( clone != 0 ) {
                delete res;
                res = clone;
            }
        }
        // SUBTRACT case: A - A = empty; res remains the empty solid
        // already initialised at function entry.
        resultPath = "geometric-identity-preflight";
        preflightResolved = true;
    }

    // Section 7.3.1.D - Containment-only preflight: when one solid is
    // contained inside the other without real edge/face intersections,
    // the regular pipeline produces empty. Dispatch to a dedicated
    // containment table per [MANT1988] chapter 15.1.
    if ( !preflightResolved &&
         _PolyhedralBoundedSolidSetNonIntersectingClassifier
             ::runContainmentOnlyPreflightCase(inSolidA, inSolidB,
                 preflightCache) ) {
        res = _PolyhedralBoundedSolidSetNonIntersectingClassifier
            ::runSetOpNoIntersectionCase(inSolidA, inSolidB, res, op,
                preflightCache);
        resultPath = "containment-only-preflight";
        preflightResolved = true;
    }

    if ( !preflightResolved ) {
        setOpGenerate(ctx, inSolidA, inSolidB);

        if ( withDebug ) {
            debugSolid(inSolidA, "outputA_stage02");
            debugSolid(inSolidB, "outputB_stage02");
        }
        traceSelfTouchingLoops(inSolidA, "A-after-generate");
        traceSelfTouchingLoops(inSolidB, "B-after-generate");
        splitSelfTouchingLoops(inSolidA);
        splitSelfTouchingLoops(inSolidB);

        setOpClassify(ctx, op, inSolidA, inSolidB);

        if ( withDebug ) {
            debugSolid(inSolidA, "outputA_stage03");
            debugSolid(inSolidB, "outputB_stage03");
        }
        // NOTE: after Classify the algorithm has (by design) inserted
        // null-edge struts (size-2 loops with coincident endpoints);
        // genuine self-touch is only meaningful after Generate.

        if ( ctx.sonea.isEmpty() && ctx.sonvv.isEmpty() ) {
            // No intersections found
            res = _PolyhedralBoundedSolidSetNonIntersectingClassifier
                ::runSetOpNoIntersectionCase(inSolidA, inSolidB, res, op);
            resultPath = "no-intersection-after-classify";
            preflightResolved = true;
        }
    }

    if ( !preflightResolved ) {
        if ( withDebug ) {
            debugSolid(inSolidA, "outputA_stage04");
            debugSolid(inSolidB, "outputB_stage04");
        }

        setOpConnect(ctx, op);

        if ( withDebug ) {
            debugSolid(inSolidA, "outputA_stage05");
            debugSolid(inSolidB, "outputB_stage05");
        }

        if ( hasIncompleteConnectState() &&
             offsetCylinderDifferenceFallbackSpec != 0 ) {
            offsetCylinderDifferenceFallback =
                _PolyhedralBoundedSolidOffsetCylinderFallback
                    ::buildOffsetCylinderDifferenceFallback(
                        offsetCylinderDifferenceFallbackSpec);
            if ( offsetCylinderDifferenceFallback != 0 ) {
                tracePipelineSummary(
                    "offset cylinder fallback replacing incomplete connect");
                delete res;
                res = offsetCylinderDifferenceFallback;
                offsetCylinderDifferenceFallback = 0;
                fallbackProvidedResult = true;
                resultPath = "offset-cylinder-fallback-incomplete-connect";
            }
        }

        if ( !fallbackProvidedResult &&
             axisAlignedCellBooleanFallback != 0 &&
             hasIncompleteConnectState() ) {
            tracePipelineSummary(
                "axis-aligned cell fallback replacing incomplete connect");
            delete res;
            res = axisAlignedCellBooleanFallback;
            axisAlignedCellBooleanFallback = 0;
            fallbackProvidedResult = true;
            resultPath = "axis-aligned-cell-fallback-incomplete-connect";
        }
        else if ( !fallbackProvidedResult &&
                  orthogonalProfileBooleanFallback != 0 &&
                  hasIncompleteConnectState() ) {
            tracePipelineSummary(
                "orthogonal profile fallback replacing incomplete connect");
            delete res;
            res = orthogonalProfileBooleanFallback;
            orthogonalProfileBooleanFallback = 0;
            fallbackProvidedResult = true;
            resultPath = "orthogonal-profile-fallback-incomplete-connect";
        }
        if ( !fallbackProvidedResult ) {
            try {
                setOpFinish(ctx, inSolidA, inSolidB, res, op);
            }
            catch ( const java::Exception& e ) {
                PolyhedralBoundedSolid* offsetCylinderExceptionFallback =
                    _PolyhedralBoundedSolidOffsetCylinderFallback
                        ::buildOffsetCylinderDifferenceFallback(
                            offsetCylinderDifferenceFallbackSpec);
                if ( isStructurallyUsableSetOpResult(
                         offsetCylinderExceptionFallback) ) {
                    tracePipelineSummary(
                        java::String("offset cylinder fallback replacing finish exception: ") +
                        e.getMessage().c_str());
                    discardIntermediateResult(res, inSolidA, inSolidB);
                    res = offsetCylinderExceptionFallback;
                    delete axisAlignedCellBooleanFallback;
                    axisAlignedCellBooleanFallback = 0;
                    delete orthogonalProfileBooleanFallback;
                    orthogonalProfileBooleanFallback = 0;
                    resultPath = "offset-cylinder-fallback-finish-exception";
                }
                else {
                    delete offsetCylinderExceptionFallback;
                    if ( axisAlignedCellBooleanFallback == 0 &&
                         orthogonalProfileBooleanFallback == 0 ) {
                        delete profileDifferenceFallback;
                        delete offsetCylinderDifferenceFallbackSpec;
                        throw;
                    }
                    else if ( axisAlignedCellBooleanFallback != 0 ) {
                        tracePipelineSummary(
                            java::String("axis-aligned cell fallback replacing finish exception: ") +
                            e.getMessage().c_str());
                        discardIntermediateResult(res, inSolidA, inSolidB);
                        res = axisAlignedCellBooleanFallback;
                        axisAlignedCellBooleanFallback = 0;
                        resultPath = "axis-aligned-cell-fallback-finish-exception";
                    }
                    else {
                        tracePipelineSummary(
                            java::String("orthogonal profile fallback replacing finish exception: ") +
                            e.getMessage().c_str());
                        discardIntermediateResult(res, inSolidA, inSolidB);
                        res = orthogonalProfileBooleanFallback;
                        orthogonalProfileBooleanFallback = 0;
                        resultPath = "orthogonal-profile-fallback-finish-exception";
                    }
                }
            }
        }

        if ( withDebug ) {
            debugSolid(inSolidA, "outputA_stage06");
            debugSolid(inSolidB, "outputB_stage06");
            debugSolid(res, "outputR_stage06");
        }

        if ( shouldUseAxisAlignedCellBooleanFallback(
                 axisAlignedCellBooleanFallback, res) ) {
            tracePipelineSummary(
                "axis-aligned cell fallback replacing incomplete result");
            discardIntermediateResult(res, inSolidA, inSolidB);
            res = axisAlignedCellBooleanFallback;
            axisAlignedCellBooleanFallback = 0;
            resultPath = "axis-aligned-cell-fallback-incomplete-result";
        }
        if ( shouldUseAxisAlignedCellBooleanFallback(
                 orthogonalProfileBooleanFallback, res) ) {
            tracePipelineSummary(
                "orthogonal profile fallback replacing incomplete result");
            discardIntermediateResult(res, inSolidA, inSolidB);
            res = orthogonalProfileBooleanFallback;
            orthogonalProfileBooleanFallback = 0;
            resultPath = "orthogonal-profile-fallback-incomplete-result";
        }

        PolyhedralBoundedSolid* resultBeforeProfileFallback = res;
        res = _PolyhedralBoundedSolidProfileDifferenceFallback
            ::applyProfileDifferenceFallbackIfNeeded(profileDifferenceFallback, res);
        if ( res != resultBeforeProfileFallback ) {
            discardIntermediateResult(resultBeforeProfileFallback, inSolidA,
                inSolidB);
            resultPath = "profile-difference-fallback";
        }
    }

    // C++ ownership: release the fallbacks that were not used and leave the
    // result as the only owner of its nodes
    delete axisAlignedCellBooleanFallback;
    delete orthogonalProfileBooleanFallback;
    delete offsetCylinderDifferenceFallback;
    delete profileDifferenceFallback;
    delete offsetCylinderDifferenceFallbackSpec;
    unlistSharedNodes(inSolidA, res);
    unlistSharedNodes(inSolidB, res);

    res = completeSetOpResult(res, operandADiagnostics, operandBDiagnostics,
        op, resultPath.c_str(), maximizeResultFaces, doStrictValidation);

    if ( withDebug ) {
        debugSolid(res, "outputR_stage07");
    }

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("= [END OF SETOP REPORT] ===================================================================================================================================\n");
    }

    return res;
}
