#include "java/lang/Math.h"
#include "java/lang/String.h"
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

java::ArrayList<java::String> _PolyhedralBoundedSolidSetIntersector::intersectionTrace;

namespace {

java::String format(const char* pattern, ...) __attribute__((format(printf, 1, 2)));

java::String format(const char* pattern, ...)
{
    char buffer[512];
    va_list arguments;
    va_start(arguments, pattern);
    vsnprintf(buffer, sizeof(buffer), pattern, arguments);
    va_end(arguments);
    return java::String(buffer);
}

}

int _PolyhedralBoundedSolidSetIntersector::compareToZero(double value)
{
    return PolyhedralBoundedSolidNumericPolicy::compareToZero(value,
        numericContext);
}

bool _PolyhedralBoundedSolidSetIntersector::isZero(double value)
{
    return PolyhedralBoundedSolidNumericPolicy::isZero(value, numericContext);
}

bool _PolyhedralBoundedSolidSetIntersector::isZeroBig(double value)
{
    return PolyhedralBoundedSolidNumericPolicy::isZeroBig(value, numericContext);
}

_PolyhedralBoundedSolidSetIntersector::BoundaryHit
_PolyhedralBoundedSolidSetIntersector::findNearbyBoundaryHit(
    _PolyhedralBoundedSolidFace* face, const Vector3Dd& point)
{
    double tolerance;
    long i;

    if ( face == nullptr ) {
        return BoundaryHit();
    }

    tolerance = java::Math::max(numericContext.bigEpsilon() * 10.0, 1.0e-7);
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidHalfEdge* start;
        _PolyhedralBoundedSolidHalfEdge* current;
        long guard;

        if ( face->boundariesList.get(i) == nullptr ||
             face->boundariesList.get(i)->boundaryStartHalfEdge == nullptr ) {
            continue;
        }
        start = face->boundariesList.get(i)->boundaryStartHalfEdge;
        current = start;
        guard = 0;
        do {
            BoundaryHit hit = findNearbyBoundaryHit(current, point, tolerance);

            if ( hit.found ) {
                return hit;
            }
            current = current->next();
            guard++;
        } while ( current != start &&
                  guard <= face->boundariesList.get(i)->halfEdgesList.size() + 1 );
    }
    return BoundaryHit();
}

_PolyhedralBoundedSolidSetIntersector::BoundaryHit
_PolyhedralBoundedSolidSetIntersector::findNearbyBoundaryHit(
    _PolyhedralBoundedSolidHalfEdge* halfEdge, const Vector3Dd& point,
    double tolerance)
{
    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd ab;
    Vector3Dd closest;
    double lengthSquared;
    double t;

    if ( halfEdge == nullptr ||
         halfEdge->startingVertex == nullptr ||
         halfEdge->next() == nullptr ||
         halfEdge->next()->startingVertex == nullptr ) {
        return BoundaryHit();
    }

    a = halfEdge->startingVertex->position;
    b = halfEdge->next()->startingVertex->position;
    if ( point.subtract(a).length() <= tolerance ) {
        return BoundaryHit(nullptr, halfEdge->startingVertex, a);
    }
    if ( point.subtract(b).length() <= tolerance ) {
        return BoundaryHit(nullptr, halfEdge->next()->startingVertex, b);
    }

    ab = b.subtract(a);
    lengthSquared = ab.dotProduct(ab);
    if ( lengthSquared <= tolerance * tolerance ) {
        return BoundaryHit();
    }
    t = point.subtract(a).dotProduct(ab) / lengthSquared;
    if ( t <= 0.0 || t >= 1.0 ) {
        return BoundaryHit();
    }
    closest = a.add(ab.multiply(t));
    if ( point.subtract(closest).length() > tolerance ) {
        return BoundaryHit();
    }
    return BoundaryHit(halfEdge, nullptr, closest);
}

int _PolyhedralBoundedSolidSetIntersector::nextVertexId(
    PolyhedralBoundedSolid* current, PolyhedralBoundedSolid* other)
{
    if ( idNamespace != nullptr ) {
        return idNamespace->nextVertexId(current, other);
    }
    int currentMax;
    int otherMax;

    currentMax = current->getMaxVertexId();
    otherMax = other->getMaxVertexId();
    if ( otherMax > currentMax ) {
        currentMax = otherMax;
    }
    return currentMax + 1;
}

/**
Inserts a vertex/face coincidence into the set corresponding to the
`sonva`/`sonvb` variables of program [MANT1988].15.1.
*/
void _PolyhedralBoundedSolidSetIntersector::addsovf(
    _PolyhedralBoundedSolidHalfEdge* he, _PolyhedralBoundedSolidFace* f,
    int BvsA, VertexFaceList& sonva, VertexFaceList& sonvb)
{
    VertexFaceList& sonv = (BvsA == 0) ? sonva : sonvb;
    size_t i;

    for ( i = 0; i < sonv.size(); i++ ) {
        if ( sonv[i].v == he->startingVertex && sonv[i].f == f ) {
            return;
        }
    }

    _PolyhedralBoundedSolidSetOperatorVertexFace elem;
    elem.v = he->startingVertex;
    elem.f = f;
    sonv.add(elem);
}

/**
Inserts a vertex/vertex coincidence into the set corresponding to the
`sonvv` variable of program [MANT1988].15.1.
*/
void _PolyhedralBoundedSolidSetIntersector::addsovv(
    _PolyhedralBoundedSolidVertex* a, _PolyhedralBoundedSolidVertex* b,
    int BvsA, VertexVertexList& sonvv)
{
    size_t i;

    for ( i = 0; i < sonvv.size(); i++ ) {
        const _PolyhedralBoundedSolidSetOperatorVertexVertex& elem = sonvv[i];
        if ( (BvsA == 0 && elem.va == a && elem.vb == b) ||
             (BvsA != 0 && elem.va == b && elem.vb == a) ) {
            return;
        }
    }

    _PolyhedralBoundedSolidSetOperatorVertexVertex elem;
    if ( BvsA == 0 ) {
        elem.va = a;
        elem.vb = b;
    }
    else {
        elem.va = b;
        elem.vb = a;
    }
    sonvv.add(elem);
}

/**
Handles the degenerate branch of the edge/face test from section
[MANT1988].15.3 when one endpoint already lies on the reference face, using
the point-on-edge and point-on-vertex bookkeeping suggested by problem
[MANT1988].13.3.
*/
void _PolyhedralBoundedSolidSetIntersector::doVertexOnFace(
    _PolyhedralBoundedSolidVertex* v,
    _PolyhedralBoundedSolidFace* f,
    int BvsA,
    PolyhedralBoundedSolid* edgeSolid,
    PolyhedralBoundedSolid* faceSolid,
    VertexVertexList& sonvv,
    VertexFaceList& sonva,
    VertexFaceList& sonvb)
{
    int cont;
    double d;
    _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge;
    _PolyhedralBoundedSolidVertex* intersectedVertex;

    d = f->getContainingPlane()->pointDistance(v->position);
    if ( compareToZero(d) == 0 ) {
        _PolyhedralBoundedSolidFace::PointInsideResult containment =
            f->testPointInsideDetailed(v->position, numericContext.bigEpsilon());
        cont = containment.status();
        intersectedHalfedge = containment.intersectedHalfedge();
        intersectedVertex = containment.intersectedVertex();
        if ( cont == Geometry::INSIDE ) {
            BoundaryHit nearbyBoundaryHit = findNearbyBoundaryHit(f, v->position);
            if ( nearbyBoundaryHit.found ) {
                cont = Geometry::LIMIT;
                intersectedHalfedge = nearbyBoundaryHit.halfEdge;
                intersectedVertex = nearbyBoundaryHit.vertex;
            }
        }
        if ( cont == Geometry::INSIDE ) {
            addsovf(v->emanatingHalfEdge, f, BvsA, sonva, sonvb);
        }
        else if ( cont == Geometry::LIMIT && intersectedHalfedge != nullptr ) {
            int newVIdVof = nextVertexId(edgeSolid, faceSolid);
            const char* eLabelVof = (BvsA == 0) ? "A" : "B";
            const char* fLabelVof = (BvsA == 0) ? "B" : "A";
            int bv1Vof = intersectedHalfedge->startingVertex->id;
            int bv2Vof = intersectedHalfedge->next()->startingVertex->id;
            intersectionTrace.add(format(
                "Vertex %s:%d created splitting %s:<%d, %d> boundary edge (at coincidence with %s vertex %d).",
                fLabelVof, newVIdVof, fLabelVof, bv1Vof, bv2Vof, eLabelVof, v->id));
            PolyhedralBoundedSolidEulerOperators::lmev(faceSolid,
                intersectedHalfedge,
                intersectedHalfedge->mirrorHalfEdge()->next(),
                newVIdVof, v->position);
            addsovv(v, intersectedHalfedge->startingVertex, BvsA, sonvv);
        }
        else if ( cont == Geometry::LIMIT && intersectedVertex != nullptr ) {
            addsovv(v, intersectedVertex, BvsA, sonvv);
        }
    }
}

/**
Performs one edge/face intersection test for the big-phase-0 generator.
This is the edge/face crossing analysis required by section [MANT1988].15.3
as part of the initial detector of program [MANT1988].15.2.
*/
_PolyhedralBoundedSolidEdge* _PolyhedralBoundedSolidSetIntersector::doSetOpGenerate(
    _PolyhedralBoundedSolidEdge* e,
    _PolyhedralBoundedSolidFace* f,
    int BvsA,
    PolyhedralBoundedSolid* edgeSolid,
    PolyhedralBoundedSolid* faceSolid,
    VertexVertexList& sonvv,
    VertexFaceList& sonva,
    VertexFaceList& sonvb)
{
    _PolyhedralBoundedSolidVertex* v1;
    _PolyhedralBoundedSolidVertex* v2;
    double d1;
    double d2;
    double d3;
    double t;
    Vector3Dd p;
    int s1;
    int s2;
    int cont;
    _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge;
    _PolyhedralBoundedSolidVertex* intersectedVertex;

    v1 = e->rightHalf->startingVertex;
    v2 = e->leftHalf->startingVertex;
    d1 = f->getContainingPlane()->pointDistance(v1->position);
    d2 = f->getContainingPlane()->pointDistance(v2->position);

    // Snap vertices in the (epsilon, bigEpsilon] gap onto the face plane.
    // Without this, such a vertex triggers the crossing branch and produces
    // a null-edge whose original endpoint is off-plane; after Connect, that
    // endpoint ends up in the result face and fails the coplanarity check.
    //
    // The snap is only valid when the vertex actually lies over the bounded
    // face: this test runs for every (edge, face) pair, so the infinite
    // plane of a face can pass within bigEpsilon of vertices that are
    // arbitrarily far from the face itself. Snapping those would drag
    // unrelated vertices off their own faces and make remote, previously
    // planar faces non-planar (stage-6 finding: moon motifs at z=9.0 moved
    // the bowl vertex antipodal to the motif). A vertex whose projection
    // falls outside the bounded face cannot contribute an intersection
    // inside that face, so skipping the snap there is always safe.
    if ( isZeroBig(d1) && !isZero(d1) ) {
        Vector3Dd snapped1 = f->getContainingPlane()->projectPoint(v1->position);
        if ( f->testPointInsideDetailed(snapped1, numericContext.bigEpsilon())
                 .status() != Geometry::OUTSIDE ) {
            v1->position = snapped1;
            d1 = 0.0;
        }
    }
    if ( isZeroBig(d2) && !isZero(d2) ) {
        Vector3Dd snapped2 = f->getContainingPlane()->projectPoint(v2->position);
        if ( f->testPointInsideDetailed(snapped2, numericContext.bigEpsilon())
                 .status() != Geometry::OUTSIDE ) {
            v2->position = snapped2;
            d2 = 0.0;
        }
    }

    s1 = compareToZero(d1);
    s2 = compareToZero(d2);

    if ( (s1 == -1 && s2 == 1) || (s1 == 1 && s2 == -1) ) {
        t = d1 / (d1 - d2);
        p = v1->position.add((v2->position.subtract(v1->position)).multiply(t));

        d3 = f->getContainingPlane()->pointDistance(p);
        if ( compareToZero(d3) == 0 ) {
            InfinitePlane* facePlane = f->getContainingPlane();
            p = facePlane->projectPoint(p);

            // Snap p to the intersection line of f's plane and the edge's
            // face plane. Without this, intrinsically non-planar faces (e.g.
            // sphere quads) produce a vertex that lies on f's plane but not
            // on the edge face's plane, causing coplanarity failures after
            // the boolean operation.
            _PolyhedralBoundedSolidFace* edgeFace =
                e->rightHalf->parentLoop->parentFace;
            if ( edgeFace != nullptr ) {
                InfinitePlane* edgeFacePlane = edgeFace->getContainingPlane();
                double dEdge = edgeFacePlane->pointDistance(p);
                if ( !isZero(dEdge) && isZeroBig(dEdge) ) {
                    Vector3Dd n1 = edgeFacePlane->getNormal();
                    Vector3Dd n2 = facePlane->getNormal();
                    double n1DotN2 = n1.dotProduct(n2);
                    double denom = 1.0 - n1DotN2 * n1DotN2;
                    if ( denom > numericContext.epsilon() ) {
                        Vector3Dd n1Perp = n1.subtract(n2.multiply(n1DotN2));
                        p = p.subtract(n1Perp.multiply(dEdge / denom));
                    }
                }
            }

            _PolyhedralBoundedSolidFace::PointInsideResult containment =
                f->testPointInsideDetailed(p, numericContext.bigEpsilon());
            cont = containment.status();
            BoundaryHit nearbyBoundaryHit;
            if ( cont == Geometry::INSIDE ) {
                nearbyBoundaryHit = findNearbyBoundaryHit(f, p);
                if ( nearbyBoundaryHit.found ) {
                    cont = Geometry::LIMIT;
                    p = nearbyBoundaryHit.point;
                }
            }

            if ( cont != Geometry::OUTSIDE ) {
                const char* eLabel = (BvsA == 0) ? "A" : "B";
                const char* fLabel = (BvsA == 0) ? "B" : "A";
                int newVId = nextVertexId(edgeSolid, faceSolid);
                intersectionTrace.add(format(
                    "Vertex %s:%d created after intersection between %s:%d face and %s:<%d, %d> edge.",
                    eLabel, newVId, fLabel, f->id, eLabel, v1->id, v2->id));
                PolyhedralBoundedSolidEulerOperators::lmev(edgeSolid,
                    e->rightHalf, e->leftHalf->next(), newVId, p);

                if ( cont == Geometry::INSIDE ) {
                    addsovf(e->rightHalf, f, BvsA, sonva, sonvb);
                }
                else if ( cont == Geometry::LIMIT &&
                          ((nearbyBoundaryHit.found &&
                            nearbyBoundaryHit.halfEdge != nullptr) ||
                           containment.intersectedHalfedge() != nullptr) ) {
                    intersectedHalfedge = nearbyBoundaryHit.found &&
                        nearbyBoundaryHit.halfEdge != nullptr ?
                        nearbyBoundaryHit.halfEdge :
                        containment.intersectedHalfedge();
                    int newVIdBoundary = nextVertexId(edgeSolid, faceSolid);
                    int bv1 = intersectedHalfedge->startingVertex->id;
                    int bv2 = intersectedHalfedge->next()->startingVertex->id;
                    intersectionTrace.add(format(
                        "Vertex %s:%d created splitting %s:<%d, %d> boundary edge (intersection on %s:%d face boundary).",
                        fLabel, newVIdBoundary, fLabel, bv1, bv2, fLabel, f->id));
                    PolyhedralBoundedSolidEulerOperators::lmev(faceSolid,
                        intersectedHalfedge,
                        intersectedHalfedge->mirrorHalfEdge()->next(),
                        newVIdBoundary, p);
                    addsovv(e->rightHalf->startingVertex,
                        intersectedHalfedge->startingVertex, BvsA, sonvv);
                }
                else if ( cont == Geometry::LIMIT &&
                          ((nearbyBoundaryHit.found &&
                            nearbyBoundaryHit.vertex != nullptr) ||
                           containment.intersectedVertex() != nullptr) ) {
                    intersectedVertex = nearbyBoundaryHit.found &&
                        nearbyBoundaryHit.vertex != nullptr ?
                        nearbyBoundaryHit.vertex :
                        containment.intersectedVertex();
                    addsovv(e->rightHalf->startingVertex, intersectedVertex,
                        BvsA, sonvv);
                }
                return e->rightHalf->previous()->parentEdge;
            }
        }
    }
    else {
        if ( s1 == 0 ) {
            doVertexOnFace(v1, f, BvsA, edgeSolid, faceSolid, sonvv, sonva,
                sonvb);
        }
        if ( s2 == 0 ) {
            doVertexOnFace(v2, f, BvsA, edgeSolid, faceSolid, sonvv, sonva,
                sonvb);
        }
    }

    return nullptr;
}

void _PolyhedralBoundedSolidSetIntersector::processEdge(
    _PolyhedralBoundedSolidEdge* e,
    PolyhedralBoundedSolid* edgeSolid,
    PolyhedralBoundedSolid* faceSolid,
    int BvsA,
    VertexVertexList& sonvv,
    VertexFaceList& sonva,
    VertexFaceList& sonvb)
{
    _PolyhedralBoundedSolidFace* f;
    _PolyhedralBoundedSolidEdge* generatedEdge;
    long i;

    for ( i = 0; i < faceSolid->getPolygonsList().size(); i++ ) {
        f = faceSolid->getPolygonsList().get(i);
        generatedEdge = doSetOpGenerate(e, f, BvsA, edgeSolid,
            faceSolid, sonvv, sonva, sonvb);
        if ( generatedEdge != nullptr ) {
            processEdge(generatedEdge, edgeSolid, faceSolid, BvsA,
                sonvv, sonva, sonvb);
        }
    }
}

_PolyhedralBoundedSolidSetIntersector::GenerationResult
_PolyhedralBoundedSolidSetIntersector::setOpGenerate(
    PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB)
{
    _PolyhedralBoundedSolidEdge* e;
    VertexVertexList sonvv;
    VertexFaceList sonva;
    VertexFaceList sonvb;
    long i;

    intersectionTrace.clear();

    for ( i = 0; i < inSolidA->getEdgesList().size(); i++ ) {
        e = inSolidA->getEdgesList().get(i);
        processEdge(e, inSolidA, inSolidB, 0, sonvv, sonva, sonvb);
    }
    for ( i = 0; i < inSolidB->getEdgesList().size(); i++ ) {
        e = inSolidB->getEdgesList().get(i);
        processEdge(e, inSolidB, inSolidA, 1, sonvv, sonva, sonvb);
    }

    return GenerationResult(sonvv, sonva, sonvb);
}
