#include "java/lang/Math.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/validation/_PolyhedralBoundedSolidFaceValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

struct FaceSegment {
    Vector3Dd start;
    Vector3Dd end;

    // Default constructible, as the elements of `java::ArrayList`
    FaceSegment() {}
    FaceSegment(const Vector3Dd& start, const Vector3Dd& end)
        : start(start), end(end) {}

    bool sharesEndpointWith(const FaceSegment& other, double tolerance) const
    {
        return start.subtract(other.start).length() <= tolerance ||
            start.subtract(other.end).length() <= tolerance ||
            end.subtract(other.start).length() <= tolerance ||
            end.subtract(other.end).length() <= tolerance;
    }
};

double clamp(double value, double min, double max)
{
    if ( value < min ) {
        return min;
    }
    if ( value > max ) {
        return max;
    }
    return value;
}

java::ArrayList<FaceSegment> collectFaceSegments(_PolyhedralBoundedSolidFace* face)
{
    java::ArrayList<FaceSegment> segments;
    for ( long i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
        if ( he == nullptr ) {
            continue;
        }
        _PolyhedralBoundedSolidHalfEdge* start = he;
        do {
            _PolyhedralBoundedSolidHalfEdge* next = he->next();
            if ( next == nullptr ) {
                break;
            }
            segments.add(FaceSegment(he->startingVertex->position,
                next->startingVertex->position));
            he = next;
        } while ( he != start );
    }
    return segments;
}

double segmentDistance(const Vector3Dd& p1, const Vector3Dd& q1,
                       const Vector3Dd& p2, const Vector3Dd& q2)
{
    Vector3Dd d1 = q1.subtract(p1);
    Vector3Dd d2 = q2.subtract(p2);
    Vector3Dd r = p1.subtract(p2);
    double a = d1.dotProduct(d1);
    double e = d2.dotProduct(d2);
    double f = d2.dotProduct(r);
    double s;
    double t;

    if ( a <= VSDK::EPSILON && e <= VSDK::EPSILON ) {
        return p1.subtract(p2).length();
    }
    if ( a <= VSDK::EPSILON ) {
        s = 0.0;
        t = clamp(f / e, 0.0, 1.0);
    }
    else {
        double c = d1.dotProduct(r);
        if ( e <= VSDK::EPSILON ) {
            t = 0.0;
            s = clamp(-c / a, 0.0, 1.0);
        }
        else {
            double b = d1.dotProduct(d2);
            double denom = a * e - b * b;
            if ( denom != 0.0 ) {
                s = clamp((b * f - c * e) / denom, 0.0, 1.0);
            }
            else {
                s = 0.0;
            }
            t = (b * s + f) / e;
            if ( t < 0.0 ) {
                t = 0.0;
                s = clamp(-c / a, 0.0, 1.0);
            }
            else if ( t > 1.0 ) {
                t = 1.0;
                s = clamp((b - c) / a, 0.0, 1.0);
            }
        }
    }

    return p1.add(d1.multiply(s)).subtract(p2.add(d2.multiply(t))).length();
}

}

bool _PolyhedralBoundedSolidFaceValidator::isSurfaceDegenerate(
    _PolyhedralBoundedSolidFace* face)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext numericContext =
        PolyhedralBoundedSolidNumericPolicy::forFace(face);
    java::ArrayList<Vector3Dd> points;
    if ( !PolyhedralBoundedSolidGeometricValidator::extractPointsFromFace(
             face, points) ||
         !PolyhedralBoundedSolidGeometricValidator
             ::validateFacePointsAreCoplanar(points, numericContext) ) {
        return true;
    }
    if ( faceArea(face) <= numericContext.bigEpsilon() *
         numericContext.bigEpsilon() ) {
        return true;
    }
    return hasCloseNonAdjacentEdges(face, numericContext);
}

double _PolyhedralBoundedSolidFaceValidator::faceArea(
    _PolyhedralBoundedSolidFace* face)
{
    double area = 0.0;
    for ( long i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
        if ( he == nullptr ) {
            continue;
        }
        _PolyhedralBoundedSolidHalfEdge* start = he;
        Vector3Dd vectorArea;
        do {
            _PolyhedralBoundedSolidHalfEdge* next = he->next();
            if ( next == nullptr ) {
                break;
            }
            vectorArea = vectorArea.add(
                he->startingVertex->position.crossProduct(
                    next->startingVertex->position));
            he = next;
        } while ( he != start );
        area += 0.5 * vectorArea.length();
    }
    return area;
}

bool _PolyhedralBoundedSolidFaceValidator::hasCloseNonAdjacentEdges(
    _PolyhedralBoundedSolidFace* face,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    java::ArrayList<FaceSegment> segments = collectFaceSegments(face);
    double tolerance = java::Math::max(numericContext.bigEpsilon() * 10.0,
        numericContext.modelScale() * 1.0e-5);
    for ( size_t i = 0; i < segments.size(); i++ ) {
        for ( size_t j = i + 1; j < segments.size(); j++ ) {
            const FaceSegment& a = segments[i];
            const FaceSegment& b = segments[j];
            if ( a.sharesEndpointWith(b, numericContext.bigEpsilon()) ) {
                continue;
            }
            if ( segmentDistance(a.start, a.end, b.start, b.end) <=
                 tolerance ) {
                return true;
            }
        }
    }
    return false;
}
