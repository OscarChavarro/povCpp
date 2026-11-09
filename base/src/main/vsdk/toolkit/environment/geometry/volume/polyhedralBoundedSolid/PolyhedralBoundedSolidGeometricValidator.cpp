#include "java/lang/Math.h"
#include "java/lang/String.h"
#include <cmath>
#include <cstdio>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

typedef PolyhedralBoundedSolidNumericPolicy::ToleranceContext ToleranceContext;

const double INVERTED_FACE_THRESHOLD = 0.5;

void append(java::String* msg, const java::String& text)
{
    if ( msg != 0 ) {
        *msg += text.c_str();
    }
}

java::String str(int value)
{
    return java::String::valueOf(value);
}

int dominantCoordinateForPlane(const InfinitePlane* plane)
{
    Vector3Dd n = plane->getNormal();
    if ( std::fabs(n.x()) >= std::fabs(n.y()) && std::fabs(n.x()) >= std::fabs(n.z()) ) {
        return 1;
    }
    if ( std::fabs(n.y()) >= std::fabs(n.x()) && std::fabs(n.y()) >= std::fabs(n.z()) ) {
        return 2;
    }
    return 3;
}

int dominantCoordinateForFace(_PolyhedralBoundedSolidFace* face)
{
    InfinitePlane* plane = face->getContainingPlane();
    int dominantCoordinate = dominantCoordinateForPlane(plane);
    delete plane;
    return dominantCoordinate;
}

Vector2Dd projectPointTo2D(const Vector3Dd& in, int dominantCoordinate)
{
    if ( dominantCoordinate == 1 ) {
        return Vector2Dd(in.y(), in.z());
    }
    if ( dominantCoordinate == 2 ) {
        return Vector2Dd(in.x(), in.z());
    }
    return Vector2Dd(in.x(), in.y());
}

double orientation2D(const Vector2Dd& a, const Vector2Dd& b, const Vector2Dd& c)
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

bool pointOnSegment2D(const Vector2Dd& p, const Vector2Dd& a, const Vector2Dd& b,
                      double orientationTolerance,
                      double linearTolerance)
{
    if ( std::fabs(orientation2D(a, b, p)) > orientationTolerance ) {
        return false;
    }
    double minX = java::Math::min(a.x, b.x) - linearTolerance;
    double maxX = java::Math::max(a.x, b.x) + linearTolerance;
    double minY = java::Math::min(a.y, b.y) - linearTolerance;
    double maxY = java::Math::max(a.y, b.y) + linearTolerance;
    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
}

bool segmentsIntersect2D(const Vector2Dd& a1, const Vector2Dd& a2,
                         const Vector2Dd& b1, const Vector2Dd& b2,
                         const ToleranceContext& numericContext)
{
    double o1 = orientation2D(a1, a2, b1);
    double o2 = orientation2D(a1, a2, b2);
    double o3 = orientation2D(b1, b2, a1);
    double o4 = orientation2D(b1, b2, a2);

    double orientationTolerance = PolyhedralBoundedSolidNumericPolicy
        ::orientationTolerance2D(a1, a2, b1, numericContext);
    orientationTolerance = java::Math::max(orientationTolerance,
        PolyhedralBoundedSolidNumericPolicy
            ::orientationTolerance2D(a1, a2, b2, numericContext));
    orientationTolerance = java::Math::max(orientationTolerance,
        PolyhedralBoundedSolidNumericPolicy
            ::orientationTolerance2D(b1, b2, a1, numericContext));
    orientationTolerance = java::Math::max(orientationTolerance,
        PolyhedralBoundedSolidNumericPolicy
            ::orientationTolerance2D(b1, b2, a2, numericContext));
    double linearTolerance =
        PolyhedralBoundedSolidNumericPolicy::linearTolerance2D(numericContext);

    bool proper = ((o1 > orientationTolerance && o2 < -orientationTolerance) ||
                   (o1 < -orientationTolerance && o2 > orientationTolerance)) &&
                  ((o3 > orientationTolerance && o4 < -orientationTolerance) ||
                   (o3 < -orientationTolerance && o4 > orientationTolerance));
    if ( proper ) {
        return true;
    }

    return pointOnSegment2D(b1, a1, a2, orientationTolerance, linearTolerance) ||
           pointOnSegment2D(b2, a1, a2, orientationTolerance, linearTolerance) ||
           pointOnSegment2D(a1, b1, b2, orientationTolerance, linearTolerance) ||
           pointOnSegment2D(a2, b1, b2, orientationTolerance, linearTolerance);
}

bool loopHasSelfIntersection(_PolyhedralBoundedSolidFace* face,
                             _PolyhedralBoundedSolidLoop* loop,
                             const ToleranceContext& numericContext,
                             java::String* msg)
{
    long int n = loop->halfEdgesList.size();
    if ( n < 3 ) {
        append(msg, "  - Face [" + str(face->id) +
            "] has a loop with fewer than 3 edges.\n");
        return true;
    }

    int dominantCoordinate = dominantCoordinateForFace(face);
    long int i;
    long int j;
    for ( i = 0; i < n; i++ ) {
        _PolyhedralBoundedSolidHalfEdge* heA = loop->halfEdgesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* heANext = heA->next();
        if ( heANext == 0 ) {
            append(msg, "  - Face [" + str(face->id) +
                "] has a non-closed loop during strict validation.\n");
            return true;
        }
        Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position, dominantCoordinate);
        Vector2Dd a2 = projectPointTo2D(heANext->startingVertex->position, dominantCoordinate);

        for ( j = i+1; j < n; j++ ) {
            if ( j == (i+1)%n || i == (j+1)%n ) {
                continue;
            }
            _PolyhedralBoundedSolidHalfEdge* heB = loop->halfEdgesList.get(j);
            _PolyhedralBoundedSolidHalfEdge* heBNext = heB->next();
            if ( heBNext == 0 ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has a non-closed loop during strict validation.\n");
                return true;
            }
            Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position, dominantCoordinate);
            Vector2Dd b2 = projectPointTo2D(heBNext->startingVertex->position, dominantCoordinate);

            if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has a self-intersecting loop.\n");
                return true;
            }
        }
    }
    return false;
}

bool loopsIntersect(_PolyhedralBoundedSolidFace* face,
                    _PolyhedralBoundedSolidLoop* loopA,
                    _PolyhedralBoundedSolidLoop* loopB,
                    const ToleranceContext& numericContext,
                    java::String* msg)
{
    int dominantCoordinate = dominantCoordinateForFace(face);
    long int i;
    long int j;
    for ( i = 0; i < loopA->halfEdgesList.size(); i++ ) {
        _PolyhedralBoundedSolidHalfEdge* heA = loopA->halfEdgesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* heANext = heA->next();
        if ( heANext == 0 ) {
            append(msg, "  - Face [" + str(face->id) +
                "] has a non-closed loop during strict validation.\n");
            return true;
        }
        Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position, dominantCoordinate);
        Vector2Dd a2 = projectPointTo2D(heANext->startingVertex->position, dominantCoordinate);

        for ( j = 0; j < loopB->halfEdgesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* heB = loopB->halfEdgesList.get(j);
            _PolyhedralBoundedSolidHalfEdge* heBNext = heB->next();
            if ( heBNext == 0 ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has a non-closed loop during strict validation.\n");
                return true;
            }
            Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position, dominantCoordinate);
            Vector2Dd b2 = projectPointTo2D(heBNext->startingVertex->position, dominantCoordinate);
            if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has intersecting loops.\n");
                return true;
            }
        }
    }
    return false;
}

bool faceAgreesWithNeighbours(_PolyhedralBoundedSolidFace* face, java::String* msg)
{
    InfinitePlane* planeF;
    Vector3Dd nF;
    long int i;
    long int j;
    int neighbourCount;
    int anomalousCount;
    double worstDot;

    if ( face == 0 || face->boundariesList.size() == 0 ) {
        return true;
    }
    planeF = face->getContainingPlane();
    if ( planeF == 0 ) {
        return true;
    }
    nF = planeF->getNormal();
    delete planeF;
    neighbourCount = 0;
    anomalousCount = 0;
    worstDot = 1.0;
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if ( loop == 0 ) {
            continue;
        }
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* he = loop->halfEdgesList.get(j);
            if ( he == 0 || he->mirrorHalfEdge() == 0 ||
                 he->mirrorHalfEdge()->parentLoop == 0 ) {
                continue;
            }
            _PolyhedralBoundedSolidFace* neighbour =
                he->mirrorHalfEdge()->parentLoop->parentFace;
            if ( neighbour == 0 || neighbour == face ) {
                continue;
            }
            InfinitePlane* planeN = neighbour->getContainingPlane();
            if ( planeN == 0 ) {
                continue;
            }
            Vector3Dd nN = planeN->getNormal();
            delete planeN;
            neighbourCount++;
            double dot = nF.dotProduct(nN);
            if ( dot < worstDot ) {
                worstDot = dot;
            }
            if ( dot < -INVERTED_FACE_THRESHOLD ) {
                anomalousCount++;
            }
        }
    }
    // Flag only when EVERY neighbour disagrees strongly (consistent
    // inversion sign).  A single sharp dihedral is not enough.
    if ( neighbourCount >= 2 && anomalousCount == neighbourCount ) {
        char worst[64];
        snprintf(worst, sizeof(worst), "%.3f", worstDot);
        append(msg, "  - Face [" + str(face->id) + "] is opposed to all " +
            str(neighbourCount) + " neighbours (worst cos=" + worst + ")\n");
        return false;
    }
    return true;
}

bool facesAreCoplanar(_PolyhedralBoundedSolidFace* faceA,
                      _PolyhedralBoundedSolidFace* faceB,
                      const ToleranceContext& numericContext)
{
    InfinitePlane* planeA = faceA->getContainingPlane();
    InfinitePlane* planeB = faceB->getContainingPlane();
    bool coplanar = false;

    if ( planeA == 0 || planeB == 0 ) {
        delete planeA;
        delete planeB;
        return false;
    }

    Vector3Dd nA = planeA->getNormal().multiply(1.0);
    Vector3Dd nB = planeB->getNormal().multiply(1.0);
    nA = nA.normalized();
    nB = nB.normalized();
    if ( std::fabs(std::fabs(nA.dotProduct(nB)) - 1.0) <=
         numericContext.coplanarDotTolerance() ) {
        for ( long int i = 0; i < faceA->boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop* loop = faceA->boundariesList.get(i);
            if ( loop->halfEdgesList.size() > 0 ) {
                Vector3Dd p = loop->halfEdgesList.get(0)->startingVertex->position;
                coplanar = std::fabs(planeB->pointDistance(p)) <=
                    numericContext.bigEpsilon();
                break;
            }
        }
    }
    delete planeA;
    delete planeB;
    return coplanar;
}

bool segmentSharesEndpoint(_PolyhedralBoundedSolidHalfEdge* a,
                           _PolyhedralBoundedSolidHalfEdge* b)
{
    _PolyhedralBoundedSolidHalfEdge* an = a->next();
    _PolyhedralBoundedSolidHalfEdge* bn = b->next();
    if ( an == 0 || bn == 0 ) {
        return false;
    }
    _PolyhedralBoundedSolidVertex* a0 = a->startingVertex;
    _PolyhedralBoundedSolidVertex* a1 = an->startingVertex;
    _PolyhedralBoundedSolidVertex* b0 = b->startingVertex;
    _PolyhedralBoundedSolidVertex* b1 = bn->startingVertex;
    return a0 == b0 || a0 == b1 || a1 == b0 || a1 == b1;
}

bool vertexStrictlyInsideFace(_PolyhedralBoundedSolidVertex* v,
                              _PolyhedralBoundedSolidFace* face,
                              const ToleranceContext& numericContext)
{
    InfinitePlane* plane = face->getContainingPlane();
    if ( plane == 0 ) {
        return false;
    }
    int status = plane->doContainmentTest(v->position, numericContext.bigEpsilon());
    delete plane;
    if ( status != Geometry::LIMIT ) {
        return false;
    }
    return PolyhedralBoundedSolidNumericPolicy
        ::testPointInside(face, v->position, numericContext) == Geometry::INSIDE;
}

bool edgePiercesFaceInterior(_PolyhedralBoundedSolidHalfEdge* he,
                             _PolyhedralBoundedSolidFace* face,
                             const ToleranceContext& numericContext)
{
    _PolyhedralBoundedSolidHalfEdge* next = he->next();
    if ( next == 0 ) {
        return false;
    }
    InfinitePlane* plane = face->getContainingPlane();
    if ( plane == 0 ) {
        return false;
    }

    Vector3Dd p0 = he->startingVertex->position;
    Vector3Dd p1 = next->startingVertex->position;
    double d0 = plane->pointDistance(p0);
    double d1 = plane->pointDistance(p1);
    delete plane;

    if ( std::fabs(d0) <= numericContext.bigEpsilon() &&
         std::fabs(d1) <= numericContext.bigEpsilon() ) {
        return false;
    }
    if ( d0*d1 > 0 ) {
        return false;
    }

    double denom = d0 - d1;
    if ( PolyhedralBoundedSolidNumericPolicy::isZero(denom, numericContext) ) {
        return false;
    }
    double t = d0 / denom;
    if ( !PolyhedralBoundedSolidNumericPolicy
        ::unitIntervalContainsStrictly(t, numericContext) ) {
        return false;
    }

    Vector3Dd p = p0.add(p1.subtract(p0).multiply(t));
    return PolyhedralBoundedSolidNumericPolicy
        ::testPointInside(face, p, numericContext) == Geometry::INSIDE;
}

void appendImproperIntersection(java::String* msg,
    _PolyhedralBoundedSolidFace* faceA, _PolyhedralBoundedSolidFace* faceB)
{
    append(msg, "  - Faces [" + str(faceA->id) + "] and [" + str(faceB->id) +
        "] intersect improperly.\n");
}

bool facesHaveImproperIntersection(_PolyhedralBoundedSolidFace* faceA,
                                   _PolyhedralBoundedSolidFace* faceB,
                                   const ToleranceContext& numericContext,
                                   java::String* msg)
{
    long int i;
    long int j;
    long int k;
    _PolyhedralBoundedSolidHalfEdge* he;

    for ( i = 0; i < faceA->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = faceA->boundariesList.get(i);
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            he = loop->halfEdgesList.get(j);
            if ( vertexStrictlyInsideFace(he->startingVertex, faceB,
                                          numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return true;
            }
            if ( edgePiercesFaceInterior(he, faceB, numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return true;
            }
        }
    }

    for ( i = 0; i < faceB->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = faceB->boundariesList.get(i);
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            he = loop->halfEdgesList.get(j);
            if ( vertexStrictlyInsideFace(he->startingVertex, faceA,
                                          numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return true;
            }
            if ( edgePiercesFaceInterior(he, faceA, numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return true;
            }
        }
    }

    if ( facesAreCoplanar(faceA, faceB, numericContext) ) {
        int dominantCoordinate = dominantCoordinateForFace(faceA);
        for ( i = 0; i < faceA->boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop* loopA = faceA->boundariesList.get(i);
            for ( j = 0; j < loopA->halfEdgesList.size(); j++ ) {
                _PolyhedralBoundedSolidHalfEdge* heA = loopA->halfEdgesList.get(j);
                _PolyhedralBoundedSolidHalfEdge* heANext = heA->next();
                if ( heANext == 0 ) {
                    continue;
                }
                Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position, dominantCoordinate);
                Vector2Dd a2 = projectPointTo2D(heANext->startingVertex->position, dominantCoordinate);

                for ( k = 0; k < faceB->boundariesList.size(); k++ ) {
                    _PolyhedralBoundedSolidLoop* loopB = faceB->boundariesList.get(k);
                    for ( long int m = 0; m < loopB->halfEdgesList.size(); m++ ) {
                        _PolyhedralBoundedSolidHalfEdge* heB = loopB->halfEdgesList.get(m);
                        _PolyhedralBoundedSolidHalfEdge* heBNext = heB->next();
                        if ( heBNext == 0 ) {
                            continue;
                        }
                        if ( heA->parentEdge == heB->parentEdge ) {
                            continue;
                        }
                        Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position, dominantCoordinate);
                        Vector2Dd b2 = projectPointTo2D(heBNext->startingVertex->position, dominantCoordinate);
                        if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) &&
                             !segmentSharesEndpoint(heA, heB) ) {
                            appendImproperIntersection(msg, faceA, faceB);
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

} // namespace

bool PolyhedralBoundedSolidGeometricValidator::validateFacePointsAreCoplanar(
    java::ArrayList<Vector3Dd>& points)
{
    return validateFacePointsAreCoplanar(points,
        PolyhedralBoundedSolidNumericPolicy::forPoints(points));
}

/**
Implements the coplanarity precondition needed before applying the face
equation ideas of [MANT1988].13.1 to a polyhedral face from
[MANT1988].10.2.1. As in Java, the reference plane is the one defined by the
last valid point triple found.
*/
bool PolyhedralBoundedSolidGeometricValidator::validateFacePointsAreCoplanar(
    java::ArrayList<Vector3Dd>& points,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    if ( points.size() < 3 ) {
        return false;
    }

    Vector3Dd p0;
    Vector3Dd p1;
    Vector3Dd p2;
    p0 = points.get(0);
    bool foundSeparatedPair = false;

    long int i;
    for ( i = 1; i < points.size(); i++ ) {
        p1 = points.get(i);
        if ( PolyhedralBoundedSolidNumericPolicy::pointsSeparated(
                 p0, p1, numericContext) ) {
            foundSeparatedPair = true;
            break;
        }
    }
    if ( !foundSeparatedPair ) {
        return false;
    }

    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd n;
    double aDotB;
    InfinitePlane* facePlane = 0;
    long int j;
    long int k;

    for ( i = 0; i < points.size(); i++ ) {
        for ( j = 0; j < points.size(); j++ ) {
            for ( k = 0; k < points.size(); k++ ) {
                if ( i == j || i == k || j == k ) {
                    continue;
                }
                p0 = points.get(i);
                p1 = points.get(j);
                p2 = points.get(k);
                if ( PolyhedralBoundedSolidNumericPolicy::pointsSeparated(
                         p0, p2, numericContext) &&
                     PolyhedralBoundedSolidNumericPolicy::pointsSeparated(
                         p1, p2, numericContext) ) {
                    a = p2.subtract(p0);
                    b = p1.subtract(p0);
                    a = a.normalized();
                    b = b.normalized();
                    aDotB = std::fabs(a.dotProduct(b));
                    if ( aDotB < 1.0 - numericContext.unitVectorTolerance() ) {
                        n = a.crossProduct(b);
                        n = n.normalized();
                        delete facePlane;
                        facePlane = new InfinitePlane(n, p0);
                    }
                    break;
                }
            }
        }
    }

    if ( facePlane == 0 ) {
        return false;
    }

    bool coplanar = true;
    for ( i = 1; i < points.size(); i++ ) {
        p0 = points.get(i);
        if ( facePlane->doContainmentTest(p0, numericContext.epsilon()) !=
             Geometry::LIMIT ) {
            coplanar = false;
            break;
        }
    }

    delete facePlane;
    return coplanar;
}

bool PolyhedralBoundedSolidGeometricValidator::extractPointsFromFace(
    _PolyhedralBoundedSolidFace* face,
    java::ArrayList<Vector3Dd>& outPoints)
{
    long int j;

    for ( j = 0; j < face->boundariesList.size(); j++ ) {
        _PolyhedralBoundedSolidLoop* loop;
        _PolyhedralBoundedSolidHalfEdge* he;
        _PolyhedralBoundedSolidHalfEdge* heStart;

        loop = face->boundariesList.get(j);
        he = loop->boundaryStartHalfEdge;
        if ( he == 0 ) {
            return false;
        }
        heStart = he;
        do {
            he = he->next();
            if ( he == 0 ) {
                return false;
            }
            outPoints.add(he->startingVertex->position);
        } while ( he != heStart );
    }
    return true;
}

bool PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(
    _PolyhedralBoundedSolidFace* face)
{
    return validateFaceIsPlanar(face,
        PolyhedralBoundedSolidNumericPolicy::forFace(face));
}

bool PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(
    _PolyhedralBoundedSolidFace* face,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    java::ArrayList<Vector3Dd> points;
    return extractPointsFromFace(face, points) &&
        validateFacePointsAreCoplanar(points, numericContext);
}

bool PolyhedralBoundedSolidGeometricValidator::validateAllFacesPlanarityAndPlanes(
    PolyhedralBoundedSolid* solid, java::String* msg)
{
    return validateAllFacesPlanarityAndPlanes(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Applies the face-planarity and face-equation checks corresponding to
[MANT1988].10.2.1 and [MANT1988].13.1 to every face of the solid.
*/
bool PolyhedralBoundedSolidGeometricValidator::validateAllFacesPlanarityAndPlanes(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    long int i;
    bool test = true;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        if ( validateFaceIsPlanar(face, numericContext) ) {
            InfinitePlane* plane = face->getContainingPlane();
            if ( plane == 0 ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] was not able to compute containing plane\n");
                test = false;
            }
            delete plane;
        }
        else {
            append(msg, "  - Face [" + str(face->id) + "] is not coplanar\n");
            test = false;
        }
    }
    return test;
}

bool PolyhedralBoundedSolidGeometricValidator::validateConsistentFaceOrientations(
    PolyhedralBoundedSolid* solid, java::String* msg)
{
    return validateConsistentFaceOrientations(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Heuristic check for the consistent face-orientation invariant required by
the 2-manifold boundary model of [MANT1988].10.2.1: each face plane normal is
compared against the normals of its neighbours through shared edges, and a
face is flagged only when it is strongly opposed to all of them.
*/
bool PolyhedralBoundedSolidGeometricValidator::validateConsistentFaceOrientations(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    long int i;
    bool test;

    (void)numericContext;
    if ( solid == 0 || solid->getPolygonsList().size() == 0 ) {
        return true;
    }
    test = true;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        if ( !faceAgreesWithNeighbours(face, msg) ) {
            test = false;
        }
    }
    return test;
}

bool PolyhedralBoundedSolidGeometricValidator::validateLoopsStrict(
    PolyhedralBoundedSolid* solid, java::String* msg)
{
    return validateLoopsStrict(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Enforces strict geometric loop consistency for the planar polygons assumed
by [MANT1988].10.2.1 and manipulated by the geometric tools of chapter
[MANT1988].13.
*/
bool PolyhedralBoundedSolidGeometricValidator::validateLoopsStrict(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    long int i;
    long int j;
    long int k;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = face->getContainingPlane();
        if ( plane == 0 ) {
            append(msg, "  - Face [" + str(face->id) +
                "] has no containing plane for strict checks.\n");
            return false;
        }
        delete plane;

        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            if ( loopHasSelfIntersection(face, loop, numericContext, msg) ) {
                return false;
            }
        }
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            for ( k = j+1; k < face->boundariesList.size(); k++ ) {
                if ( loopsIntersect(face, face->boundariesList.get(j),
                                    face->boundariesList.get(k),
                                    numericContext, msg) ) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool PolyhedralBoundedSolidGeometricValidator::validateFaceIntersectionsStrict(
    PolyhedralBoundedSolid* solid, java::String* msg)
{
    return validateFaceIntersectionsStrict(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Applies strict face/face intersection checks to preserve the validity
condition from [MANT1988].15.2, criterion 3, using predicates from chapter
[MANT1988].13.
*/
bool PolyhedralBoundedSolidGeometricValidator::validateFaceIntersectionsStrict(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    long int i;
    long int j;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* faceA = solid->getPolygonsList().get(i);
        for ( j = i+1; j < solid->getPolygonsList().size(); j++ ) {
            _PolyhedralBoundedSolidFace* faceB = solid->getPolygonsList().get(j);
            if ( facesHaveImproperIntersection(faceA, faceB, numericContext,
                                               msg) ) {
                return false;
            }
        }
    }
    return true;
}

bool PolyhedralBoundedSolidGeometricValidator::validateNoCoincidentVertices(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context,
    java::String* msg)
{
    long int i;
    long int j;
    bool ok;
    _PolyhedralBoundedSolidVertex* vi;
    _PolyhedralBoundedSolidVertex* vj;

    ok = true;
    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        vi = solid->getVerticesList().get(i);
        if ( vi == 0 ) {
            continue;
        }
        for ( j = i + 1; j < solid->getVerticesList().size(); j++ ) {
            vj = solid->getVerticesList().get(j);
            if ( vj == 0 ) {
                continue;
            }
            if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                     vi->position, vj->position, context) ) {
                java::String* position = vi->position.toString();
                append(msg, "  coincident vertices: v" + str(vi->id) +
                    " and v" + str(vj->id) + " at " + position->c_str() + "\n");
                delete position;
                ok = false;
            }
        }
    }
    return ok;
}

bool PolyhedralBoundedSolidGeometricValidator::validateUniqueFaceAndVertexIds(
    PolyhedralBoundedSolid* solid, java::String* msg)
{
    bool ok;
    long int i;
    long int j;
    _PolyhedralBoundedSolidFace* fi;
    _PolyhedralBoundedSolidFace* fj;
    _PolyhedralBoundedSolidVertex* vi;
    _PolyhedralBoundedSolidVertex* vj;

    ok = true;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        fi = solid->getPolygonsList().get(i);
        if ( fi == 0 ) {
            continue;
        }
        if ( fi->id > solid->getMaxFaceId() ) {
            append(msg, "  face id " + str(fi->id) + " exceeds maxFaceId=" +
                str(solid->getMaxFaceId()) + "\n");
            ok = false;
        }
        for ( j = i + 1; j < solid->getPolygonsList().size(); j++ ) {
            fj = solid->getPolygonsList().get(j);
            if ( fj != 0 && fi->id == fj->id ) {
                append(msg, "  duplicate face id=" + str(fi->id) + "\n");
                ok = false;
            }
        }
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        vi = solid->getVerticesList().get(i);
        if ( vi == 0 ) {
            continue;
        }
        if ( vi->id > solid->getMaxVertexId() ) {
            append(msg, "  vertex id " + str(vi->id) + " exceeds maxVertexId=" +
                str(solid->getMaxVertexId()) + "\n");
            ok = false;
        }
        for ( j = i + 1; j < solid->getVerticesList().size(); j++ ) {
            vj = solid->getVerticesList().get(j);
            if ( vj != 0 && vi->id == vj->id ) {
                append(msg, "  duplicate vertex id=" + str(vi->id) + "\n");
                ok = false;
            }
        }
    }

    return ok;
}
