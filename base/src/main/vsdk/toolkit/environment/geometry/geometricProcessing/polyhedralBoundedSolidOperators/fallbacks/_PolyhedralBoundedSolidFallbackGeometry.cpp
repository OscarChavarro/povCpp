#include <cmath>

#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidFallbackGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidFallbackGeometry FallbackGeometry;

double FallbackGeometry::coordinate(const Vector3Dd& p, int axis)
{
    if ( axis == 0 ) {
        return p.x();
    }
    if ( axis == 1 ) {
        return p.y();
    }
    return p.z();
}

bool FallbackGeometry::sameCoordinate(double a, double b)
{
    return std::fabs(a - b) <= numericContext.bigEpsilon();
}

bool FallbackGeometry::boundsMatch(const double* a, const double* b)
{
    int i;

    if ( a == 0 || b == 0 ) {
        return false;
    }
    for ( i = 0; i < 6; i++ ) {
        if ( !sameCoordinate(a[i], b[i]) ) {
            return false;
        }
    }
    return true;
}

void FallbackGeometry::addUniqueCoordinate(java::ArrayList<double>& values,
    double value)
{
    size_t i;

    for ( i = 0; i < values.size(); i++ ) {
        if ( sameCoordinate(values[i], value) ) {
            return;
        }
    }
    values.add(value);
    // As Java `Collections.sort` over `Double`
    values.sort([](double x, double y) { return java::Double::compare(x, y) < 0; });
}

java::ArrayList<double> FallbackGeometry::uniqueVertexCoordinates(
    PolyhedralBoundedSolid* solid, int axis)
{
    java::ArrayList<double> values;
    long int i;

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        addUniqueCoordinate(values,
            coordinate(solid->getVerticesList().get(i)->position, axis));
    }
    return values;
}

double FallbackGeometry::signedAreaOnYZ(const Profile& profile)
{
    double area;
    size_t i;

    area = 0.0;
    for ( i = 0; i < profile.size(); i++ ) {
        const Vector3Dd& a = profile[i];
        const Vector3Dd& b = profile[(i + 1) % profile.size()];
        area += a.y() * b.z() - b.y() * a.z();
    }
    return area * 0.5;
}

FallbackGeometry::Profile FallbackGeometry::extractProfileAtX(
    PolyhedralBoundedSolid* solid, double x)
{
    Profile best;
    double bestArea;
    long int i;

    bestArea = 0.0;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        long int j;

        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            Profile profile;
            double area;
            long int k;
            bool onPlane;

            if ( loop->halfEdgesList.size() < 3 ) {
                continue;
            }
            onPlane = true;
            for ( k = 0; k < loop->halfEdgesList.size(); k++ ) {
                Vector3Dd p = loop->halfEdgesList.get(k)->startingVertex->position;
                if ( !sameCoordinate(p.x(), x) ) {
                    onPlane = false;
                    break;
                }
                profile.add(p);
            }
            if ( !onPlane ) {
                continue;
            }
            area = std::fabs(signedAreaOnYZ(profile));
            if ( area > bestArea ) {
                bestArea = area;
                best = profile;
            }
        }
    }
    return best;
}

bool FallbackGeometry::sameProfilePoint(const Vector3Dd& a, const Vector3Dd& b)
{
    return sameCoordinate(a.x(), b.x()) &&
           sameCoordinate(a.y(), b.y()) &&
           sameCoordinate(a.z(), b.z());
}

void FallbackGeometry::appendProfilePoint(Profile& profile, const Vector3Dd& point)
{
    if ( !profile.isEmpty() && sameProfilePoint(profile[profile.size() - 1], point) ) {
        return;
    }
    profile.add(point);
}

Vector3Dd FallbackGeometry::projectProfilePoint(const Vector3Dd& point,
    double x, double zCut)
{
    double z;

    z = point.z();
    if ( sameCoordinate(z, zCut) ) {
        z = zCut;
    }
    return Vector3Dd(x, point.y(), z);
}

Vector3Dd FallbackGeometry::intersectProfileSegmentAtZ(const Vector3Dd& a,
    const Vector3Dd& b, double x, double zCut)
{
    double t;
    double y;

    if ( sameCoordinate(a.z(), b.z()) ) {
        return Vector3Dd(x, a.y(), zCut);
    }
    t = (zCut - a.z()) / (b.z() - a.z());
    y = a.y() + (b.y() - a.y()) * t;
    return Vector3Dd(x, y, zCut);
}

FallbackGeometry::Profile FallbackGeometry::clipProfileAboveZ(
    const Profile& profile, double x, double zCut)
{
    Profile clipped;
    Vector3Dd previous;
    bool previousInside;
    size_t i;

    if ( profile.size() < 3 ) {
        return clipped;
    }

    previous = profile[profile.size() - 1];
    previousInside = previous.z() + numericContext.bigEpsilon() >= zCut;
    for ( i = 0; i < profile.size(); i++ ) {
        Vector3Dd current = profile[i];
        bool currentInside = current.z() + numericContext.bigEpsilon() >= zCut;

        if ( currentInside ) {
            if ( !previousInside ) {
                appendProfilePoint(clipped,
                    intersectProfileSegmentAtZ(previous, current, x, zCut));
            }
            appendProfilePoint(clipped, projectProfilePoint(current, x, zCut));
        }
        else if ( previousInside ) {
            appendProfilePoint(clipped,
                intersectProfileSegmentAtZ(previous, current, x, zCut));
        }

        previous = current;
        previousInside = currentInside;
    }

    if ( clipped.size() > 1 && sameProfilePoint(clipped[0], clipped[clipped.size() - 1]) ) {
        clipped.remove(clipped.size() - 1);
    }
    return clipped;
}
