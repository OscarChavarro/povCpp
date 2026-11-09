#include "java/lang/String.h"
#include <cstdio>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationContext.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationTrace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetVertexFaceClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetGeometricPredicateProcessor.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetClassifier Classifier;
typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector OnSector;

int Classifier::debugFlags = 0;

namespace {

const int DEBUG_01_STRUCTURE = 0x01;
const int DEBUG_04_VERTEX_VERTEX_CLASSIFIER = 0x08;
const int DEBUG_99_SHOW_OPERATIONS = 0x40;

void tracePipelineSummary(const java::String& message)
{
    _SetOperationTrace::tracePipelineSummary(java::String(message.c_str()));
}

void traceCoplanarTangential(const java::String& message)
{
    _SetOperationTrace::traceCoplanarTangential(java::String(message.c_str()));
}

const char* booleanName(bool value)
{
    return value ? "true" : "false";
}

java::String halfEdgeText(_PolyhedralBoundedSolidHalfEdge* he)
{
    return he == nullptr ? java::String("null") : java::String(he->toString().c_str());
}

}

void Classifier::runSetOpClassify(int op,
                                  PolyhedralBoundedSolid* inSolidA,
                                  PolyhedralBoundedSolid* inSolidB,
                                  int flags,
                                  _SetOperationContext& ctx)
{
    debugFlags = flags;
    setOpClassify(ctx, op, inSolidA, inSolidB);
    tracePipelineSummary(
        "classify done op=" + java::String::valueOf(op) +
        " sonva=" + java::String::valueOf(ctx.sonva.size()) +
        " sonvb=" + java::String::valueOf(ctx.sonvb.size()) +
        " sonvv=" + java::String::valueOf(ctx.sonvv.size()) +
        " sonea=" + java::String::valueOf(ctx.sonea.size()) +
        " soneb=" + java::String::valueOf(ctx.soneb.size()));
}

int Classifier::compareToZero(double value)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor::compareToZero(value);
}

java::String Classifier::summarizeHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == nullptr ) {
        return "null";
    }

    return "he(v=" + java::String::valueOf(he->startingVertex->id) +
        ",f=" + java::String::valueOf(he->parentLoop->parentFace->id) + ")";
}

java::String Classifier::summarizeNullEdge(
    const _PolyhedralBoundedSolidSetOperatorNullEdge* edge)
{
    if ( edge == nullptr || edge->e == nullptr ) {
        return "null";
    }
    return summarizeHalfEdge(edge->e->rightHalf) + " | " +
        summarizeHalfEdge(edge->e->leftHalf);
}

java::String Classifier::formatVertexVertexTraceContext(int op, int cursor,
    const java::String& caseName, const java::String& action, int type)
{
    return "op=" + java::String::valueOf(op) +
        " cursor=" + java::String::valueOf(cursor) +
        " case=" + caseName +
        " action=" + action +
        " target=" + (type == 0 ? "A" : "B");
}

java::String Classifier::labelSectorState(int state)
{
    switch ( state ) {
        case OnSector::ON:
            return "ON";
        case OnSector::OUT:
            return "OUT";
        case OnSector::IN:
            return "IN";
        default:
            return "?" + java::String::valueOf(state);
    }
}

java::String Classifier::summarizeSector(const VertexVertexClassificationData& data,
                                        int index)
{
    if ( index < 0 || index >= (int)data.sectors.size() ) {
        return "sector[idx=" + java::String::valueOf(index) + "]=<out>";
    }
    const OnSector& sector = data.sectors[(size_t)index];
    return "sector[idx=" + java::String::valueOf(index) +
        ",intersect=" + booleanName(sector.intersect) +
        ",sectA=" + java::String::valueOf(sector.secta) +
        ",sectB=" + java::String::valueOf(sector.sectb) +
        ",s1a=" + labelSectorState(sector.s1a) +
        ",s2a=" + labelSectorState(sector.s2a) +
        ",s1b=" + labelSectorState(sector.s1b) +
        ",s2b=" + labelSectorState(sector.s2b) +
        ",hea=" + summarizeHalfEdge(data.nba[(size_t)sector.secta].he) +
        ",heb=" + summarizeHalfEdge(data.nbb[(size_t)sector.sectb].he) +
        "]";
}

void Classifier::traceVertexVertexDecisionStep(
    const VertexVertexClassificationData& data, int op,
    const java::String& phase, int index,
    _PolyhedralBoundedSolidHalfEdge* ha1, _PolyhedralBoundedSolidHalfEdge* ha2,
    _PolyhedralBoundedSolidHalfEdge* hb1, _PolyhedralBoundedSolidHalfEdge* hb2)
{
    tracePipelineSummary(
        "vv-step op=" + java::String::valueOf(op) +
        " phase=" + phase +
        " " + summarizeSector(data, index) +
        " => ha1=" + summarizeHalfEdge(ha1) +
        " ha2=" + summarizeHalfEdge(ha2) +
        " hb1=" + summarizeHalfEdge(hb1) +
        " hb2=" + summarizeHalfEdge(hb2));
}

void Classifier::traceVertexVertexDecisionReason(int op, int cursor,
    const java::String& caseName, const java::String& reason,
    _PolyhedralBoundedSolidHalfEdge* ha1, _PolyhedralBoundedSolidHalfEdge* ha2,
    _PolyhedralBoundedSolidHalfEdge* hb1, _PolyhedralBoundedSolidHalfEdge* hb2)
{
    tracePipelineSummary(
        "vv-decision op=" + java::String::valueOf(op) +
        " cursor=" + java::String::valueOf(cursor) +
        " case=" + caseName +
        " reason=" + reason +
        " ha1=" + summarizeHalfEdge(ha1) +
        " ha2=" + summarizeHalfEdge(ha2) +
        " hb1=" + summarizeHalfEdge(hb1) +
        " hb2=" + summarizeHalfEdge(hb2));
}

_PolyhedralBoundedSolidHalfEdge* Classifier::selectAlternativeBHalfEdge(
    const VertexVertexClassificationData& data, int sectb,
    _PolyhedralBoundedSolidHalfEdge* avoid)
{
    _PolyhedralBoundedSolidHalfEdge* candidate;
    int nextsectb;
    int prevsectb;
    int nbbSize = (int)data.nbb.size();

    if ( data.nbb.isEmpty() ) {
        return nullptr;
    }
    if ( sectb < 0 || sectb >= nbbSize ) {
        return nullptr;
    }

    nextsectb = (sectb == nbbSize - 1) ? 0 : sectb + 1;
    prevsectb = (sectb == 0) ? nbbSize - 1 : sectb - 1;

    candidate = data.nbb[(size_t)nextsectb].he;
    if ( candidate != nullptr && candidate != avoid ) {
        return candidate;
    }

    candidate = data.nbb[(size_t)prevsectb].he;
    if ( candidate != nullptr && candidate != avoid ) {
        return candidate;
    }

    return nullptr;
}

_PolyhedralBoundedSolidHalfEdge* Classifier::resolveBEndpointCandidate(
    const VertexVertexClassificationData& data, int sectorIndex,
    bool selectHb1, _PolyhedralBoundedSolidHalfEdge* hb1,
    _PolyhedralBoundedSolidHalfEdge* hb2)
{
    _PolyhedralBoundedSolidHalfEdge* candidate;
    _PolyhedralBoundedSolidHalfEdge* avoid;
    _PolyhedralBoundedSolidHalfEdge* alternative;

    const OnSector& sector = data.sectors[(size_t)sectorIndex];
    candidate = data.nbb[(size_t)sector.sectb].he;
    avoid = selectHb1 ? hb2 : hb1;

    if ( candidate != nullptr &&
         candidate == avoid &&
         sector.s2b == OnSector::ON ) {
        alternative = selectAlternativeBHalfEdge(data, sector.sectb, avoid);
        if ( alternative != nullptr ) {
            tracePipelineSummary(
                "vv-adjust B endpoint sectorIdx=" + java::String::valueOf(sectorIndex) +
                " sectB=" + java::String::valueOf(sector.sectb) +
                " select=" + (selectHb1 ? "hb1" : "hb2") +
                " repeated=" + summarizeHalfEdge(candidate) +
                " alternative=" + summarizeHalfEdge(alternative) +
                " because s2b=ON");
            return alternative;
        }
    }

    return candidate;
}

void Classifier::traceIntersectingSectorsSnapshot(
    const VertexVertexClassificationData& data, int cursor)
{
    size_t idx;

    if ( !_SetOperationTrace::isCoplanarTangentialTraceEnabled() ) {
        return;
    }

    for ( idx = 0; idx < data.sectors.size(); idx++ ) {
        if ( !data.sectors[idx].intersect ) {
            continue;
        }
        traceCoplanarTangential(
            "  sector idx=" + java::String::valueOf(idx) +
            " cursor=" + java::String::valueOf(cursor) +
            " sectA=" + java::String::valueOf(data.sectors[idx].secta) +
            " sectB=" + java::String::valueOf(data.sectors[idx].sectb) +
            " s1a=" + java::String::valueOf(data.sectors[idx].s1a) +
            " s2a=" + java::String::valueOf(data.sectors[idx].s2a) +
            " s1b=" + java::String::valueOf(data.sectors[idx].s1b) +
            " s2b=" + java::String::valueOf(data.sectors[idx].s2b));
    }
}

int Classifier::pointInFace(_PolyhedralBoundedSolidFace* face,
                            const Vector3Dd& point)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor::pointInFace(face, point);
}

int Classifier::resolveCoplanarVertexVertexClass(int op, bool sameOrientation,
                                                 bool sideA)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor
        ::resolveCoplanarVertexVertexClass(op, sameOrientation, sideA);
}

int Classifier::nextVertexId(PolyhedralBoundedSolid* current,
                             PolyhedralBoundedSolid* other)
{
    int a;
    int b;
    int m;

    a = current->getMaxVertexId();
    b = other->getMaxVertexId();
    m = a;
    if ( b > a ) {
        m = b;
    }

    return m+1;
}

Vector3Dd Classifier::inside(_PolyhedralBoundedSolidHalfEdge* he)
{
    Vector3Dd middle;
    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd n;

    a = (he->next())->startingVertex->position.subtract(
        he->startingVertex->position);
    b = (he->previous())->startingVertex->position.subtract(
        he->startingVertex->position);
    a = a.normalized();
    b = b.normalized();

    n = he->parentLoop->parentFace->getContainingPlane()->getNormal();

    middle = n.crossProduct(a);
    middle = middle.normalized();

    return middle;
}

bool Classifier::sctrwitthin(const Vector3Dd& dir, const Vector3Dd& ref1,
                             const Vector3Dd& ref2, const Vector3Dd& ref12)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor::sctrwitthin(
        dir, ref1, ref2, ref12);
}

/**
Normalizes one endpoint for `separateEdgeSequence` when a previous null
strut edge was already inserted on the same vertex neighborhood.
*/
_PolyhedralBoundedSolidHalfEdge* Classifier::recoverEdgeSequenceEndpointFromStrut(
    _PolyhedralBoundedSolidHalfEdge* endpoint, bool isFromEndpoint)
{
    _PolyhedralBoundedSolidHalfEdge* prev;

    if ( endpoint == nullptr ) {
        return nullptr;
    }

    prev = endpoint->previous();
    if ( prev == nullptr || prev->parentEdge == nullptr ) {
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

/**
Following program [MANT1988].15.12.
Taking in to account the updated version modifications from
[.wMANT2008].
*/
void Classifier::separateEdgeSequence(_SetOperationContext& ctx,
    _PolyhedralBoundedSolidHalfEdge* from,
    _PolyhedralBoundedSolidHalfEdge* to,
    int type, const java::String& traceContext,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    //-----------------------------------------------------------------
    if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
        printf("      SEPARATEEDGESEQUENCE %d\n", type);
        printf("        From: %s\n", halfEdgeText(from).c_str());
        printf("        To: %s\n", halfEdgeText(to).c_str());
    }

    if ( from == nullptr || to == nullptr ) {
        Logger::reportMessage("null", Logger::FATAL_ERROR, "separateEdgeSequence",
            "Unexpected case: null halfedges!");
    }

    PolyhedralBoundedSolid* s;
    s = from->parentLoop->parentFace->parentSolid;

    if ( s != to->parentLoop->parentFace->parentSolid ) {
        Logger::reportMessage("null", Logger::FATAL_ERROR, "separateEdgeSequence",
            "Unexpected case: halfedges on different solids!");
    }

    //-----------------------------------------------------------------
    // Recover from null edges already inserted.
    // This block fully resolves the old A-E unsupported branches by
    // canonicalizing endpoint selection until both halfedges share origin.
    int recoveryGuard = 0;
    bool changed;
    do {
        changed = false;

        _PolyhedralBoundedSolidHalfEdge* recoveredFrom;
        _PolyhedralBoundedSolidHalfEdge* recoveredTo;

        recoveredFrom = recoverEdgeSequenceEndpointFromStrut(from, true);
        if ( recoveredFrom != from ) {
            from = recoveredFrom;
            changed = true;
            if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                printf("        Recovered edge sequence case A\n");
            }
        }

        recoveredTo = recoverEdgeSequenceEndpointFromStrut(to, false);
        if ( recoveredTo != to ) {
            to = recoveredTo;
            changed = true;
            if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                printf("        Recovered edge sequence case B\n");
            }
        }

        if ( from->startingVertex != to->startingVertex ) {
            _PolyhedralBoundedSolidHalfEdge* fromPrev = from->previous();
            _PolyhedralBoundedSolidHalfEdge* toPrev = to->previous();

            if ( fromPrev != nullptr && toPrev != nullptr &&
                 fromPrev->parentEdge != nullptr && toPrev->parentEdge != nullptr &&
                 fromPrev == toPrev->mirrorHalfEdge() ) {
                from = fromPrev;
                changed = true;
                if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                    printf("        Recovered edge sequence case C\n");
                }
            }
            else if ( fromPrev != nullptr &&
                      fromPrev->startingVertex == to->startingVertex ) {
                from = fromPrev;
                changed = true;
                if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                    printf("        Recovered edge sequence case D\n");
                }
            }
            else if ( toPrev != nullptr &&
                      toPrev->startingVertex == from->startingVertex ) {
                to = toPrev;
                changed = true;
                if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                    printf("        Recovered edge sequence case E\n");
                }
            }
        }

        recoveryGuard++;
        if ( recoveryGuard > 16 ) {
            break;
        }
    } while ( changed );

    if ( from->startingVertex != to->startingVertex ) {
        Logger::reportMessage("null", Logger::FATAL_ERROR, "separateEdgeSequence",
            "Unable to recover endpoint pairing after A-E normalization.");
        return;
    }

    //-----------------------------------------------------------------
    if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 &&
         (debugFlags & DEBUG_99_SHOW_OPERATIONS) != 0x00 ) {
        printf("       -> LMEV (Separate edge sequence):\n");
        printf("          . H1: %s\n", halfEdgeText(to).c_str());
        printf("          . H2: %s\n", halfEdgeText(from).c_str());
    }

    int id = nextVertexId(inSolidA, inSolidB);

    PolyhedralBoundedSolidEulerOperators::lmev(s, to, from, id, to->startingVertex->position);

    if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 &&
         (debugFlags & DEBUG_99_SHOW_OPERATIONS) != 0x00 ) {
        printf("          . New vertex: %d\n", id);
    }

    _PolyhedralBoundedSolidSetOperatorNullEdge edge(from->previous()->parentEdge);
    if ( type == 0 ) {
        ctx.sonea.add(edge);
    }
    else {
        ctx.soneb.add(edge);
    }
    tracePipelineSummary(
        "emit " + traceContext +
        " separateEdgeSequence sourceFrom=" + summarizeHalfEdge(from) +
        " sourceTo=" + summarizeHalfEdge(to) +
        " inserted=" + summarizeNullEdge(&edge) +
        " soneaSize=" + java::String::valueOf(ctx.sonea.size()) +
        " sonebSize=" + java::String::valueOf(ctx.soneb.size()));
}

/**
Inserts a null-edge strut for the coplanar V/V case (Program [MANT1988].15.12)
and, when `orient` is `false`, swaps rightHalf/leftHalf so the
null-edge points toward the open (OUT) side of the boundary per table 15.3.
The orientation flip is required when the half-edge approaches the shared
edge from the side that would otherwise produce an inward-facing null-edge.
@param he half-edge whose starting vertex receives the new strut vertex
@param type 0 = emitting to sonea (solidA side), 1 = soneb (solidB side)
@param orient true when the null-edge natural direction is already correct
@param traceContext caller label for pipeline-summary diagnostics
*/
void Classifier::flipNullEdgeOrientationForOpenSide(_SetOperationContext& ctx,
    _PolyhedralBoundedSolidHalfEdge* he, int type, bool orient,
    const java::String& traceContext,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
        printf("      SEPARATEINTERIOR %d\n", type);
        printf("        From/To: %s\n", halfEdgeText(he).c_str());
    }

    _PolyhedralBoundedSolidHalfEdge* tmp;

    int id = nextVertexId(inSolidA, inSolidB);
    PolyhedralBoundedSolidEulerOperators::lmev(he->parentLoop->parentFace->parentSolid,
        he, he, id, he->startingVertex->position);

    if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 &&
         (debugFlags & DEBUG_99_SHOW_OPERATIONS) != 0x00 ) {
        printf("          . New vertex: %d\n", id);
    }

    // A piece of Black Art: reverse orientation of the null edge
    if ( !orient ) {
        tmp = he->previous()->parentEdge->rightHalf;
        he->previous()->parentEdge->rightHalf = he->previous()->parentEdge->leftHalf;
        he->previous()->parentEdge->leftHalf = tmp;
    }

    _PolyhedralBoundedSolidSetOperatorNullEdge edge(he->previous()->parentEdge);
    if ( type == 0 ) {
        ctx.sonea.add(edge);
    }
    else {
        ctx.soneb.add(edge);
    }
    tracePipelineSummary(
        "emit " + traceContext +
        " flipNullEdgeOrientationForOpenSide orient=" + booleanName(orient) +
        " source=" + summarizeHalfEdge(he) +
        " inserted=" + summarizeNullEdge(&edge) +
        " soneaSize=" + java::String::valueOf(ctx.sonea.size()) +
        " sonebSize=" + java::String::valueOf(ctx.soneb.size()));
}

/**
Borrowed from [.wMANT2008].
*/
bool Classifier::nulledge(_PolyhedralBoundedSolidHalfEdge* he)
{
    return PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
        he->startingVertex->position, he->next()->startingVertex->position,
        numericContext);
}

/**
Borrowed from [.wMANT2008].
*/
bool Classifier::strutnulledge(_PolyhedralBoundedSolidHalfEdge* he)
{
    if( he == he->mirrorHalfEdge()->next() ||
        he == he->mirrorHalfEdge()->previous() ) {
        return true;
    }
    return false;
}

/**
Borrowed from [.wMANT2008].
*/
bool Classifier::convexedg(_PolyhedralBoundedSolidHalfEdge* he)
{
    _PolyhedralBoundedSolidHalfEdge* h2;
    Vector3Dd dir;
    Vector3Dd cr;

    h2 = he->next();
    if ( nulledge(he) ) {
        h2 = h2->next();
    }
    dir = h2->startingVertex->position.subtract(he->startingVertex->position);
    cr = he->parentLoop->parentFace->getContainingPlane()->getNormal().crossProduct(
        he->mirrorHalfEdge()->parentLoop->parentFace->getContainingPlane()->getNormal());
    if ( cr.length() < numericContext.unitVectorTolerance() ) {
        return true;
    }
    return (dir.dotProduct(cr) < 0.0);
}

/**
Borrowed from [.wMANT2008].
*/
bool Classifier::sectorwide(_PolyhedralBoundedSolidHalfEdge* he, int /*ind*/)
{
    return checkWideness(he);
}

/**
Borrowed from [.wMANT2008].
*/
bool Classifier::getOrientation(_PolyhedralBoundedSolidHalfEdge* ref,
                                _PolyhedralBoundedSolidHalfEdge* he1,
                                _PolyhedralBoundedSolidHalfEdge* he2)
{
    _PolyhedralBoundedSolidHalfEdge* mhe1;
    _PolyhedralBoundedSolidHalfEdge* mhe2;
    bool retcode = false;

    mhe1 = he1->mirrorHalfEdge()->next();
    mhe2 = he2->mirrorHalfEdge()->next();
    if ( mhe1 != he2 && mhe2 == he1 ) {
        retcode = convexedg(he2);
    }
    else {
        retcode = convexedg(he1);
    }
    if( sectorwide(mhe1, 0) && sectorwide(ref, 0) ) {
        retcode = !retcode;
    }

    return !retcode;
}

bool Classifier::sectorContainsAState(const OnSector& sector, int wantedState)
{
    return sector.s1a == wantedState || sector.s2a == wantedState;
}

bool Classifier::sectorContainsBState(const OnSector& sector, int wantedState)
{
    return sector.s1b == wantedState || sector.s2b == wantedState;
}

_PolyhedralBoundedSolidHalfEdge* Classifier::selectMissingEndpoint(
    const VertexVertexClassificationData& data, bool fromSolidA,
    int wantedState, _PolyhedralBoundedSolidHalfEdge* avoid)
{
    _PolyhedralBoundedSolidHalfEdge* fallback;
    size_t i;

    fallback = nullptr;
    for ( i = 0; i < data.sectors.size(); i++ ) {
        _PolyhedralBoundedSolidHalfEdge* candidate;
        bool containsState;

        const OnSector& sector = data.sectors[i];
        if ( !sector.intersect ) {
            continue;
        }

        if ( fromSolidA ) {
            containsState = sectorContainsAState(sector, wantedState);
            candidate = data.nba[(size_t)sector.secta].he;
        }
        else {
            containsState = sectorContainsBState(sector, wantedState);
            candidate = data.nbb[(size_t)sector.sectb].he;
        }

        if ( !containsState ) {
            continue;
        }

        if ( fallback == nullptr ) {
            fallback = candidate;
        }

        if ( avoid != nullptr && candidate != avoid ) {
            return candidate;
        }

        if ( avoid == nullptr ) {
            return candidate;
        }
    }

    return fallback;
}

void Classifier::recoverMissingCoplanarEndpoints(
    const VertexVertexClassificationData& data,
    _PolyhedralBoundedSolidHalfEdge*& ha1, _PolyhedralBoundedSolidHalfEdge*& ha2,
    _PolyhedralBoundedSolidHalfEdge*& hb1, _PolyhedralBoundedSolidHalfEdge*& hb2)
{
    traceCoplanarTangential(
        "recover endpoints before ha1=" + summarizeHalfEdge(ha1) +
        " ha2=" + summarizeHalfEdge(ha2) +
        " hb1=" + summarizeHalfEdge(hb1) +
        " hb2=" + summarizeHalfEdge(hb2));
    if ( ha1 == nullptr ) {
        ha1 = selectMissingEndpoint(data, true, OnSector::OUT, ha2);
    }
    if ( ha2 == nullptr ) {
        ha2 = selectMissingEndpoint(data, true, OnSector::IN, ha1);
    }
    if ( hb1 == nullptr ) {
        hb1 = selectMissingEndpoint(data, false, OnSector::IN, hb2);
    }
    if ( hb2 == nullptr ) {
        hb2 = selectMissingEndpoint(data, false, OnSector::OUT, hb1);
    }

    traceCoplanarTangential(
        "recover endpoints after ha1=" + summarizeHalfEdge(ha1) +
        " ha2=" + summarizeHalfEdge(ha2) +
        " hb1=" + summarizeHalfEdge(hb1) +
        " hb2=" + summarizeHalfEdge(hb2));
}

/**
Following section [MANT1988].15.6.2. and program [MANT1988].15.11.
*/
void Classifier::vertexVertexInsertNullEdges(_SetOperationContext& ctx,
    VertexVertexClassificationData& data, int op,
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    _PolyhedralBoundedSolidHalfEdge* ha1 = nullptr;
    _PolyhedralBoundedSolidHalfEdge* ha2 = nullptr;
    _PolyhedralBoundedSolidHalfEdge* hb1 = nullptr;
    _PolyhedralBoundedSolidHalfEdge* hb2 = nullptr;
    size_t i;
    size_t n = data.sectors.size();

    if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
        printf("   - Null edges insertion:\n");
    }

    int count = 0;

    for ( i = 0; i < n; i++ ) {
        if ( data.sectors[i].intersect ) count++;
    }

    if ( count == 0 && n > 0 ) {
        ha1 = data.nba[(size_t)data.sectors[0].secta].he;
        hb1 = data.nbb[(size_t)data.sectors[0].sectb].he;
    }

    traceCoplanarTangential(
        "vv insert null edges intersectCount=" + java::String::valueOf(count) +
        " totalSectors=" + java::String::valueOf(n));

    i = 0;
    while ( true ) {
        java::String traceContext;
        //-------------------------------------------------------------
        if ( i >= n ) {
            return;
        }
        while ( !data.sectors[i].intersect ) {
            i++;
            if ( i == n ) {
                return;
            }
        }
        if ( data.sectors[i].s1a == OnSector::OUT ) {
            ha1 = data.nba[(size_t)data.sectors[i].secta].he;
        }
        else {
            ha2 = data.nba[(size_t)data.sectors[i].secta].he;
        }
        if ( data.sectors[i].s1b == OnSector::IN ) {
            hb1 = data.nbb[(size_t)data.sectors[i].sectb].he;
            i++;
        }
        else {
            hb2 = data.nbb[(size_t)data.sectors[i].sectb].he;
            i++;
        }

        //-------------------------------------------------------------
        if ( i >= n ) {
            return;
        }
        while ( !data.sectors[i].intersect ) {
            i++;
            if ( i == n ) {
                return;
            }
        }
        if ( data.sectors[i].s1a == OnSector::OUT ) {
            ha1 = data.nba[(size_t)data.sectors[i].secta].he;
        }
        else {
            ha2 = data.nba[(size_t)data.sectors[i].secta].he;
        }
        if ( data.sectors[i].s1b == OnSector::IN ) {
            hb1 = data.nbb[(size_t)data.sectors[i].sectb].he;
            i++;
        }
        else {
            hb2 = data.nbb[(size_t)data.sectors[i].sectb].he;
            i++;
        }

        //-------------------------------------------------------------
        if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
            printf("    . Deciding case:\n");
            printf("      -> Ha1: %s\n", halfEdgeText(ha1).c_str());
            printf("      -> Ha2: %s\n", halfEdgeText(ha2).c_str());
            printf("      -> Hb1: %s\n", halfEdgeText(hb1).c_str());
            printf("      -> Hb2: %s\n", halfEdgeText(hb2).c_str());
        }

        //-------------------------------------------------------------
        if ( ha1 == nullptr || ha2 == nullptr || hb1 == nullptr || hb2 == nullptr ) {
            size_t j;

            for ( j = 0; j < n; j++ ) {
                data.sectors[j].intersect = false;
            }
            if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                printf("    . Incomplete coplanar pairing, skipping split\n");
            }
            return;
        }

        //-------------------------------------------------------------
        int cursor = (int)i;
        if ( ha1 == ha2 ) {
            traceVertexVertexDecisionReason(op, cursor, "STRUT_A",
                "ha1==ha2 after sector accumulation", ha1, ha2, hb1, hb2);
            traceContext = formatVertexVertexTraceContext(op, cursor,
                "STRUT_A", "separate", 0);
            if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                printf("    . STRUT A CASE\n");
            }
            flipNullEdgeOrientationForOpenSide(ctx, ha1, 0,
                getOrientation(ha1, hb1, hb2), traceContext, inSolidA, inSolidB);
            separateEdgeSequence(ctx, hb1, hb2, 1,
                formatVertexVertexTraceContext(op, cursor, "STRUT_A", "separate", 1),
                inSolidA, inSolidB);
        }
        else if ( hb1 == hb2 ) {
            traceVertexVertexDecisionReason(op, cursor, "STRUT_B",
                "hb1==hb2 after sector accumulation", ha1, ha2, hb1, hb2);
            traceContext = formatVertexVertexTraceContext(op, cursor,
                "STRUT_B", "separate", 1);
            if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                printf("    . STRUT B CASE\n");
            }
            flipNullEdgeOrientationForOpenSide(ctx, hb1, 1,
                getOrientation(hb1, ha2, ha1), traceContext, inSolidA, inSolidB);
            separateEdgeSequence(ctx, ha2, ha1, 0,
                formatVertexVertexTraceContext(op, cursor, "STRUT_B", "separate", 0),
                inSolidA, inSolidB);
        }
        else {
            traceVertexVertexDecisionReason(op, cursor, "PARALLEL",
                "ha1!=ha2 and hb1!=hb2", ha1, ha2, hb1, hb2);
            if ( (debugFlags & DEBUG_04_VERTEX_VERTEX_CLASSIFIER) != 0x00 ) {
                printf("    . PARALLEL CASE\n");
            }
            separateEdgeSequence(ctx, ha2, ha1, 0,
                formatVertexVertexTraceContext(op, cursor, "PARALLEL", "separate", 0),
                inSolidA, inSolidB);
            separateEdgeSequence(ctx, hb1, hb2, 1,
                formatVertexVertexTraceContext(op, cursor, "PARALLEL", "separate", 1),
                inSolidA, inSolidB);
        }
        if ( i == n ) {
            return;
        }
    }
}

/**
Vertex/Vertex classifier for the set operations algorithm (big phase 2).
Following program [MANT1988].15.6. Similar in structure to program
[MANT1988].14.3.
*/
void Classifier::vertexVertexClassify(_SetOperationContext& ctx,
    _PolyhedralBoundedSolidVertex* va, _PolyhedralBoundedSolidVertex* vb,
    int op, PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    _PolyhedralBoundedSolidSetVertexVertexClassifier classifier;
    VertexVertexClassificationData data = classifier.classify(va, vb, op,
                                                              debugFlags);
    vertexVertexInsertNullEdges(ctx, data, op, inSolidA, inSolidB);
}

/**
Main control algorithm for the big phases 1 and 2. This calls the
classifiers for vertex/face and vertex/vertex coincidences found on
`setOpGenerate`.
Following section [MANT1988].16.6.1. and program [MANT1988].15.5.
*/
void Classifier::setOpClassify(_SetOperationContext& ctx, int op,
                               PolyhedralBoundedSolid* inSolidA,
                               PolyhedralBoundedSolid* inSolidB)
{
    size_t i;

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("- 1.A. ----------------------------------------------------------------------------------------------------------------------------------------------------\n");
        printf("VERTICES OF {A} TOUCHING FACES ON {B} (sonva array of %zu matches)\n", ctx.sonva.size());
    }

    for ( i = 0; i < ctx.sonva.size(); i++ ) {
        _PolyhedralBoundedSolidSetVertexFaceClassifier classifier;
        classifier.classify(ctx.sonva[i].v, ctx.sonva[i].f, op, 0, debugFlags,
            &ctx.sonea, &ctx.soneb, inSolidA, inSolidB);
    }

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("- 1.B. ----------------------------------------------------------------------------------------------------------------------------------------------------\n");
        printf("VERTICES OF {B} TOUCHING FACES ON {A} (sonvb array of %zu matches):\n", ctx.sonvb.size());
    }

    for ( i = 0; i < ctx.sonvb.size(); i++ ) {
        _PolyhedralBoundedSolidSetVertexFaceClassifier classifier;
        classifier.classify(ctx.sonvb[i].v, ctx.sonvb[i].f, op, 1, debugFlags,
            &ctx.sonea, &ctx.soneb, inSolidA, inSolidB);
    }

    if ( (debugFlags & DEBUG_01_STRUCTURE) != 0x00 ) {
        printf("- 2. ------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        printf("VERTEX-VERTEX PAIRS (sonvv array of %zu pairs):\n", ctx.sonvv.size());
    }

    for ( i = 0; i < ctx.sonvv.size(); i++ ) {
        vertexVertexClassify(ctx, ctx.sonvv[i].va, ctx.sonvv[i].vb, op, inSolidA, inSolidB);
    }
}
