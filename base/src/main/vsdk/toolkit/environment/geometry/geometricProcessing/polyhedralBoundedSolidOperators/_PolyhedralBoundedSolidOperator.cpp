#include "java/lang/String.h"
#include <cfloat>
#include <cmath>
#include <cstdio>

#include "java/lang/Boolean.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
    _PolyhedralBoundedSolidOperator::numericContext =
    PolyhedralBoundedSolidNumericPolicy::defaultContext();

_PolyhedralBoundedSolidIdNamespace* _PolyhedralBoundedSolidOperator::idNamespace =
    nullptr;

namespace {

/// Name of the system property that enables the pipeline traces
const char* const TRACE_PIPELINE_SUMMARY = "vsdk.setop.tracePipelineSummary";

bool searchForEdge(java::ArrayList<_PolyhedralBoundedSolidEdge*>& l,
                   _PolyhedralBoundedSolidEdge* e)
{
    long i;

    for ( i = 0; i < l.size(); i++ ) {
        if ( l.get(i) == e ) return true;
    }
    return false;
}

bool searchForVertex(java::ArrayList<_PolyhedralBoundedSolidVertex*>& l,
                     _PolyhedralBoundedSolidVertex* v)
{
    long i;

    for ( i = 0; i < l.size(); i++ ) {
        if ( l.get(i) == v ) return true;
    }
    return false;
}

const char* booleanName(bool value)
{
    return value ? "true" : "false";
}

}

void _PolyhedralBoundedSolidOperator::setNumericContext(
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext* context)
{
    if ( context == nullptr ) {
        numericContext = PolyhedralBoundedSolidNumericPolicy::defaultContext();
    }
    else {
        numericContext = *context;
    }
}

void _PolyhedralBoundedSolidOperator::setIdNamespace(
    _PolyhedralBoundedSolidIdNamespace* ns)
{
    idNamespace = ns;
}

bool _PolyhedralBoundedSolidOperator::neighbor(
    _PolyhedralBoundedSolidHalfEdge* h1, _PolyhedralBoundedSolidHalfEdge* h2)
{
    return (h1->parentLoop->parentFace == h2->parentLoop->parentFace) &&
        ( (
          h1 == h1->parentEdge->rightHalf && h2 == h2->parentEdge->leftHalf
          ) ||
          (
          h1 == h1->parentEdge->leftHalf && h2 == h2->parentEdge->rightHalf
          ) );
}

void _PolyhedralBoundedSolidOperator::cleanup(PolyhedralBoundedSolid* s)
{
    long i;
    long j;
    _PolyhedralBoundedSolidFace* f;
    _PolyhedralBoundedSolidLoop* l;
    _PolyhedralBoundedSolidHalfEdge* he;

    for ( i = 0; i < s->getPolygonsList().size(); i++ ) {
        f = s->getPolygonsList().get(i);
        for ( j = 0; j < f->boundariesList.size(); j++ ) {
            l = f->boundariesList.get(j);
            he = l->boundaryStartHalfEdge;
            do {
                //
                if ( !searchForEdge(s->getEdgesList(), he->parentEdge) ) {
                    s->getEdgesList().add(he->parentEdge);
                }
                if ( !searchForVertex(s->getVerticesList(), he->startingVertex) ) {
                    s->getVerticesList().add(he->startingVertex);
                    he->startingVertex->emanatingHalfEdge = he;
                }
                //
                he = he->next();
            } while( he != l->boundaryStartHalfEdge );
        }
    }
}

void _PolyhedralBoundedSolidOperator::movefac(_PolyhedralBoundedSolidFace* f,
                                              PolyhedralBoundedSolid* s)
{
    _PolyhedralBoundedSolidLoop* l;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidFace* f2;
    long i;

    if ( !canMoveFace(f) ) {
        return;
    }

    f->parentSolid->getPolygonsList().remove(f);
    s->getPolygonsList().add(f);
    f->parentSolid = s;

    for ( i = 0; i < f->boundariesList.size(); i++ ) {
        l = f->boundariesList.get(i);
        he = l->boundaryStartHalfEdge;
        do {
            _PolyhedralBoundedSolidHalfEdge* mirror = he->mirrorHalfEdge();
            if ( mirror == nullptr || mirror->parentLoop == nullptr ||
                 mirror->parentLoop->parentFace == nullptr ) {
                he = he->next();
                continue;
            }
            f2 = mirror->parentLoop->parentFace;
            if ( f2->parentSolid != s && canMoveFace(f2) ) {
                movefac(f2, s);
            }
            he = he->next();
        } while( he != l->boundaryStartHalfEdge );
    }
}

bool _PolyhedralBoundedSolidOperator::canMoveFace(_PolyhedralBoundedSolidFace* f)
{
    long i;

    if ( f == nullptr ) {
        return false;
    }
    for ( i = 0; i < f->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* l = f->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* start;
        _PolyhedralBoundedSolidHalfEdge* he;
        long guard;

        if ( l == nullptr || l->boundaryStartHalfEdge == nullptr ) {
            return false;
        }
        start = l->boundaryStartHalfEdge;
        he = start;
        guard = 0;
        do {
            if ( he == nullptr ||
                 he->parentEdge == nullptr ||
                 he->parentLoop == nullptr ||
                 he->startingVertex == nullptr ||
                 he->mirrorHalfEdge() == nullptr ||
                 he->next() == nullptr ) {
                return false;
            }
            he = he->next();
            guard++;
        } while ( he != start && guard <= l->halfEdgesList.size() + 1 );
        if ( he != start ) {
            return false;
        }
    }
    return true;
}

Vector3Dd _PolyhedralBoundedSolidOperator::bisector(
    _PolyhedralBoundedSolidHalfEdge* he)
{
    Vector3Dd middle;
    Vector3Dd a;
    Vector3Dd b;

    a = (he->next())->startingVertex->position.subtract(he->startingVertex->position);
    b = (he->previous())->startingVertex->position.subtract(he->startingVertex->position);
    a = a.normalized();
    b = b.normalized();

    middle = he->startingVertex->position.add((a.add(b)).multiply(0.5));

    return middle;
}

/**
Moves those rings of `f1` that do not lie within its outer loop to
`f2`.
This procedure is used on the splitter and set operator algorithms to
ensure that after a face has been divided by a MEF, all loops will end up
in the correct halves.
This is an answer to problem [MANT1988].13.5. Its use in the context of
the splitter algorithm is briefly described on section [MANT1988].14.7.2.
*/
void _PolyhedralBoundedSolidOperator::laringmv(_PolyhedralBoundedSolidFace* f1,
                                               _PolyhedralBoundedSolidFace* f2)
{
    _PolyhedralBoundedSolidLoop* l;
    long i;

    // It is supposed to move all (internal) rings from `f1` to `f2`
    // using PolyhedralBoundedSolid.lringmv
    // Legacy rule: move every ring. A geometric two-face containment
    // rule was tried here (mythosPlan Phase 3) and regressed reference
    // flows: mid-connect loops are not simple regions (bridge edges,
    // spikes, half-built chains), so parity containment is unreliable
    // at this point of the pipeline. The shadow trace below records
    // what the geometric rule would have decided, for diagnosis only.
    for ( i = 1; i < f1->boundariesList.size(); i++ ) {
        l = f1->boundariesList.get(i);
        if ( java::Boolean::getBoolean(TRACE_PIPELINE_SUMMARY) ) {
            traceRingMoveShadowDecision(f1, f2, l,
                java::String("laringmv MOVE wouldMove=") +
                booleanName(ringBelongsToOtherHalf(f1, f2, l)));
        }
        if ( PolyhedralBoundedSolidEulerOperators::lringmv(f1->parentSolid, l, f2, false) ) {
            i--;
        }
    }
}

/**
Decides on which side of a face division a pending ring belongs
([MANT1988].13.5, completed): after a `lmef` divides a face into
`f1` (keeping the old outer loop side) and `f2`, a ring belongs to the
half whose region geometrically contains its representative point.
Containment is evaluated by 2D parity against each half's outer loop on
the dominant projection plane. The three observed regimes (mythosPlan
Phase 3 shadow study):
- Chord division (regions disjoint): exactly one parity test is true
  — the ring follows it. This keeps cusp-region strut rings with
  their junction partners instead of stranding them.
- Interior closed curve (one region nested in the other): both tests
  are true for rings in the nested region; the nested half wins
  (decided by testing one vertex of f2's outer loop against f1's).
- Ring on the dividing boundary (zero clearance: the common
  single-chord pending strut): both tests are unreliable/false —
  the legacy behavior (move to `f2`) is preserved.
@param f1 face that kept the original outer loop side
@param f2 face created by the division
@param l pending ring currently parented by `f1`
@return true when the ring must move to `f2`
*/
bool _PolyhedralBoundedSolidOperator::ringBelongsToOtherHalf(
    _PolyhedralBoundedSolidFace* f1,
    _PolyhedralBoundedSolidFace* f2,
    _PolyhedralBoundedSolidLoop* l)
{
    if ( f1 == nullptr || f2 == nullptr || l == nullptr ||
         l->boundaryStartHalfEdge == nullptr ||
         l->boundaryStartHalfEdge->startingVertex == nullptr ) {
        return true;
    }
    Vector3Dd point = l->boundaryStartHalfEdge->startingVertex->position;
    int insideF1 = outerLoopParityContains(f1, point);
    int insideF2 = outerLoopParityContains(f2, point);
    if ( insideF1 < 0 || insideF2 < 0 ) {
        return true;
    }
    if ( insideF1 != insideF2 ) {
        return insideF2 == 1;
    }
    if ( insideF1 == 1 ) {
        // Nested halves: the ring belongs to the inner region. Decide
        // which outer loop is nested by testing a representative vertex
        // of f2's outer loop against f1's outer loop.
        _PolyhedralBoundedSolidLoop* f2Outer = f2->boundariesList.size() > 0
            ? f2->boundariesList.get(0) : nullptr;
        if ( f2Outer == nullptr || f2Outer->boundaryStartHalfEdge == nullptr ||
             f2Outer->boundaryStartHalfEdge->startingVertex == nullptr ) {
            return true;
        }
        int f2NestedInF1 = outerLoopParityContains(f1,
            f2Outer->boundaryStartHalfEdge->startingVertex->position);
        if ( f2NestedInF1 < 0 ) {
            return true;
        }
        return f2NestedInF1 == 1;
    }
    // Outside both (on the dividing boundary or degenerate): legacy.
    return true;
}

/**
2D parity (ray crossing) containment of a point against the outer loop
of a face, on the dominant projection plane of the face normal.
@param f face whose outer loop is tested
@param point point to classify
@return 1 / 0 parity result, or -1 when not computable (Java returns a
nullable `Boolean`)
*/
int _PolyhedralBoundedSolidOperator::outerLoopParityContains(
    _PolyhedralBoundedSolidFace* f,
    const Vector3Dd& point)
{
    if ( f == nullptr || f->boundariesList.size() == 0 ||
         f->getContainingPlane() == nullptr ) {
        return -1;
    }
    _PolyhedralBoundedSolidLoop* outerLoop = f->boundariesList.get(0);
    if ( outerLoop == nullptr || outerLoop->boundaryStartHalfEdge == nullptr ) {
        return -1;
    }

    Vector3Dd normal = f->getContainingPlane()->getNormal();
    double ax = std::abs(normal.x());
    double ay = std::abs(normal.y());
    double az = std::abs(normal.z());
    int dropAxis;
    if ( ax >= ay && ax >= az ) {
        dropAxis = 0;
    }
    else if ( ay >= ax && ay >= az ) {
        dropAxis = 1;
    }
    else {
        dropAxis = 2;
    }

    double pu = shadowProjectedU(point, dropAxis);
    double pv = shadowProjectedV(point, dropAxis);
    bool inside = false;
    int guard = 0;
    _PolyhedralBoundedSolidHalfEdge* start = outerLoop->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* he = start;
    do {
        _PolyhedralBoundedSolidHalfEdge* next = he->next();
        if ( next == nullptr || he->startingVertex == nullptr ||
             next->startingVertex == nullptr ) {
            return -1;
        }
        double au = shadowProjectedU(he->startingVertex->position, dropAxis);
        double av = shadowProjectedV(he->startingVertex->position, dropAxis);
        double bu = shadowProjectedU(next->startingVertex->position, dropAxis);
        double bv = shadowProjectedV(next->startingVertex->position, dropAxis);
        if ( (av > pv) != (bv > pv) ) {
            double crossingU = (bu - au) * (pv - av) / (bv - av) + au;
            if ( pu < crossingU ) {
                inside = !inside;
            }
        }
        he = next;
        guard++;
    } while ( he != start && guard < 100000 );
    return inside ? 1 : 0;
}

/**
Diagnostic shadow for the ring redistribution decision (mythosPlan
Phase 3): computes — without changing behavior — whether the ring's
representative point lies inside `f1`'s outer loop (2D parity on the
dominant projection plane) and its clearance from the outer-loop edges,
then logs one line when the pipeline trace property is set. The legacy
behavior (move every ring to `f2`) is preserved by the caller; this
trace exists to map where that behavior is load-bearing versus where it
mis-parents pending null-edge strut rings (crescent-cusp faces).
@param f1 face whose rings are being redistributed
@param f2 destination face of the legacy unconditional move
@param l ring about to be moved
*/
void _PolyhedralBoundedSolidOperator::traceRingMoveShadowDecision(
    _PolyhedralBoundedSolidFace* f1,
    _PolyhedralBoundedSolidFace* f2,
    _PolyhedralBoundedSolidLoop* l,
    const java::String& site)
{
    if ( !java::Boolean::getBoolean(TRACE_PIPELINE_SUMMARY) ) {
        return;
    }
    if ( f1 == nullptr || f2 == nullptr || l == nullptr ||
         f1->boundariesList.size() == 0 ||
         l->boundaryStartHalfEdge == nullptr ||
         l->boundaryStartHalfEdge->startingVertex == nullptr ||
         f1->getContainingPlane() == nullptr ) {
        printf("[LARINGMV] f1=%s f2=%s ring=untestable site=%s\n",
            f1 == nullptr ? "?" : java::String::valueOf(f1->id).c_str(),
            f2 == nullptr ? "?" : java::String::valueOf(f2->id).c_str(),
            site.c_str());
        return;
    }
    _PolyhedralBoundedSolidLoop* outerLoop = f1->boundariesList.get(0);
    if ( outerLoop == l || outerLoop->boundaryStartHalfEdge == nullptr ) {
        printf("[LARINGMV] f1=%d f2=%d ring=outer? site=%s\n", f1->id, f2->id,
            site.c_str());
        return;
    }

    Vector3Dd normal = f1->getContainingPlane()->getNormal();
    double ax = std::abs(normal.x());
    double ay = std::abs(normal.y());
    double az = std::abs(normal.z());
    int dropAxis;
    if ( ax >= ay && ax >= az ) {
        dropAxis = 0;
    }
    else if ( ay >= ax && ay >= az ) {
        dropAxis = 1;
    }
    else {
        dropAxis = 2;
    }

    Vector3Dd point = l->boundaryStartHalfEdge->startingVertex->position;
    double pu = shadowProjectedU(point, dropAxis);
    double pv = shadowProjectedV(point, dropAxis);

    bool inside = false;
    double minClearance = DBL_MAX;
    int outerSize = 0;
    _PolyhedralBoundedSolidHalfEdge* start = outerLoop->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* he = start;
    bool walkable = true;
    do {
        _PolyhedralBoundedSolidHalfEdge* next = he->next();
        if ( next == nullptr || he->startingVertex == nullptr ||
             next->startingVertex == nullptr ) {
            walkable = false;
            break;
        }
        double au = shadowProjectedU(he->startingVertex->position, dropAxis);
        double av = shadowProjectedV(he->startingVertex->position, dropAxis);
        double bu = shadowProjectedU(next->startingVertex->position, dropAxis);
        double bv = shadowProjectedV(next->startingVertex->position, dropAxis);

        double eu = bu - au;
        double ev = bv - av;
        double lenSq = eu * eu + ev * ev;
        double t = 0.0;
        if ( lenSq > 0.0 ) {
            t = ((pu - au) * eu + (pv - av) * ev) / lenSq;
            if ( t < 0.0 ) {
                t = 0.0;
            }
            else if ( t > 1.0 ) {
                t = 1.0;
            }
        }
        double du = pu - (au + eu * t);
        double dv = pv - (av + ev * t);
        double clearance = std::sqrt(du * du + dv * dv);
        if ( clearance < minClearance ) {
            minClearance = clearance;
        }

        if ( (av > pv) != (bv > pv) ) {
            double crossingU = (bu - au) * (pv - av) / (bv - av) + au;
            if ( pu < crossingU ) {
                inside = !inside;
            }
        }
        outerSize++;
        he = next;
    } while ( he != start && outerSize < 100000 );

    long ringSize = l->halfEdgesList.size();
    java::String* position = point.toString();
    printf("[LARINGMV] f1=%d f2=%d ringV=%d ringSize=%ld outerSize=%d "
        "walkable=%s inside=%s clearance=%.6f p=%s site=%s\n",
        f1->id, f2->id, l->boundaryStartHalfEdge->startingVertex->id,
        ringSize, outerSize, booleanName(walkable), booleanName(inside),
        minClearance, position->c_str(), site.c_str());
    delete position;
}

double _PolyhedralBoundedSolidOperator::shadowProjectedU(const Vector3Dd& p,
                                                         int dropAxis)
{
    if ( dropAxis == 0 ) {
        return p.y();
    }
    return p.x();
}

double _PolyhedralBoundedSolidOperator::shadowProjectedV(const Vector3Dd& p,
                                                         int dropAxis)
{
    if ( dropAxis == 1 || dropAxis == 0 ) {
        return p.z();
    }
    return p.y();
}

void _PolyhedralBoundedSolidOperator::join(_PolyhedralBoundedSolidHalfEdge* h1,
                                           _PolyhedralBoundedSolidHalfEdge* h2,
                                           bool withDebug)
{
    join(h1, h2, withDebug, true);
}

void _PolyhedralBoundedSolidOperator::join(_PolyhedralBoundedSolidHalfEdge* h1,
                                           _PolyhedralBoundedSolidHalfEdge* h2,
                                           bool withDebug,
                                           bool allowRingMove)
{
    PolyhedralBoundedSolidStatistics::recordJoinCall();
    _PolyhedralBoundedSolidFace* oldf;
    _PolyhedralBoundedSolidFace* newf;
    PolyhedralBoundedSolid* s;

    if ( withDebug ) {
        printf("       -> JOIN:\n");
        printf("          . H1: %s\n", h1->toString().c_str());
        printf("          . H2: %s\n", h2->toString().c_str());
    }

    oldf = h1->parentLoop->parentFace;
    newf = nullptr;
    s = oldf->parentSolid;
    if ( h1->parentLoop == h2->parentLoop ) {
        if ( h1->previous()->previous() != h2 ) {
            int fid1 = (idNamespace != nullptr)
                ? idNamespace->nextFaceId(s)
                : s->getMaxFaceId() + 1;
            newf = PolyhedralBoundedSolidEulerOperators::lmef(s, h1, h2->next(), fid1);
        }
    }
    else {
        PolyhedralBoundedSolidEulerOperators::lmekr(s, h1, h2->next());
    }

    if ( h1->next()->next() != h2 ) {
        int fid2 = (idNamespace != nullptr)
            ? idNamespace->nextFaceId(s)
            : s->getMaxFaceId() + 1;
        // Shadow diagnostics (mythosPlan Phase 3): capture which face
        // the second division splits and the face it creates, so the
        // rings that are never redistributed across this division stay
        // observable. Applying laringmv here was tried and regressed
        // MOON_BLOCK reference cases — behavior stays legacy.
        _PolyhedralBoundedSolidFace* splitFace2 =
            h2->parentLoop->parentFace;
        _PolyhedralBoundedSolidFace* newFace2 =
            PolyhedralBoundedSolidEulerOperators::lmef(s, h2, h1->next(), fid2);
        if ( java::Boolean::getBoolean(TRACE_PIPELINE_SUMMARY) &&
             splitFace2 != nullptr &&
             splitFace2->boundariesList.size() >= 2 ) {
            long shadowI;
            for ( shadowI = 1;
                  shadowI < splitFace2->boundariesList.size();
                  shadowI++ ) {
                traceRingMoveShadowDecision(splitFace2, newFace2,
                    splitFace2->boundariesList.get(shadowI),
                    java::String("lmef2 KEEP wouldMove=") +
                    booleanName(ringBelongsToOtherHalf(splitFace2, newFace2,
                        splitFace2->boundariesList.get(shadowI))));
            }
        }
        if ( newf != nullptr && oldf->boundariesList.size() >= 2 ) {
            if ( allowRingMove ) {
                laringmv(oldf, newf);
            }
        }
    }
}

bool _PolyhedralBoundedSolidOperator::checkWideness(
    _PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == nullptr || he->parentLoop == nullptr ||
         he->parentLoop->parentFace == nullptr ||
         he->parentLoop->parentFace->getContainingPlane() == nullptr ||
         he->previous() == nullptr || he->next() == nullptr ) {
        return true;
    }

    Vector3Dd ref1;
    Vector3Dd ref2;
    Vector3Dd ref12;

    ref1 = he->previous()->startingVertex->position.subtract(
        he->startingVertex->position);
    ref2 = he->next()->startingVertex->position.subtract(
        he->startingVertex->position);
    ref12 = ref1.crossProduct(ref2);
    if ( ref12.length() < VSDK::EPSILON ) {
        return true;
    }
    return ref12.dotProduct(
        he->parentLoop->parentFace->getContainingPlane()->getNormal()) <= 0.0;
}
