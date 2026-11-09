//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "java/lang/Math.h"
#include "java/util/Collections.h"
#include <climits>
#include <cmath>

#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetNonIntersectingClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetGeometricPredicateProcessor.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

typedef _PolyhedralBoundedSolidSetNonIntersectingClassifier Classifier;

//= Preflight cache =================================================

const int Classifier::_PreflightCache::UNSET = INT_MIN;

Classifier::_PreflightCache::_PreflightCache(
    PolyhedralBoundedSolid* inA, PolyhedralBoundedSolid* inB)
    : a(inA), b(inB), aInBValue(UNSET), bInAValue(UNSET),
      interiorOverlap(-1), edgeFaceAB(-1), edgeFaceBA(-1)
{
}

int Classifier::_PreflightCache::aInB()
{
    if ( aInBValue == UNSET ) {
        aInBValue = classifySolidAgainstSolid(a, b);
    }
    return aInBValue;
}

int Classifier::_PreflightCache::bInA()
{
    if ( bInAValue == UNSET ) {
        bInAValue = classifySolidAgainstSolid(b, a);
    }
    return bInAValue;
}

bool Classifier::_PreflightCache::hasInteriorOverlap()
{
    if ( interiorOverlap < 0 ) {
        interiorOverlap = (signed char)(hasConfirmedInteriorOverlap(a, b) ? 1 : 0);
    }
    return interiorOverlap == 1;
}

bool Classifier::_PreflightCache::hasEdgeFaceIntersectionAB()
{
    if ( edgeFaceAB < 0 ) {
        edgeFaceAB = (signed char)(hasProperEdgeFaceIntersection(a, b) ? 1 : 0);
    }
    return edgeFaceAB == 1;
}

bool Classifier::_PreflightCache::hasEdgeFaceIntersectionBA()
{
    if ( edgeFaceBA < 0 ) {
        edgeFaceBA = (signed char)(hasProperEdgeFaceIntersection(b, a) ? 1 : 0);
    }
    return edgeFaceBA == 1;
}

//= Face planes holder ==============================================

/**
Precomputes the containing plane of every face of `solid`, indexed by
face position. Point classification tests many points against the same
unmutated solid, so computing each plane once (instead of once per point per
face) removes the dominant Newell/tolerance-context recompute from the hot
path.
*/
Classifier::FacePlanes::FacePlanes(PolyhedralBoundedSolid* solid)
{
    long int i;

    if ( solid == 0 ) {
        return;
    }
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        planes.add(solid->getPolygonsList().get(i)->getContainingPlane());
    }
}

Classifier::FacePlanes::~FacePlanes()
{
    for ( size_t i = 0; i < planes.size(); i++ ) {
        delete planes[i];
    }
}

InfinitePlane* Classifier::FacePlanes::get(long int i) const
{
    return planes[(size_t)i];
}

//= Public preflight API ============================================

Classifier::_PolyhedralBoundedSolidSetNonIntersectingClassifier()
{
}

Classifier::_PreflightCache Classifier::newPreflightCache(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB)
{
    return _PreflightCache(inSolidA, inSolidB);
}

bool Classifier::runContainmentOnlyPreflightCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB)
{
    _PreflightCache cache = newPreflightCache(inSolidA, inSolidB);
    return runContainmentOnlyPreflightCase(inSolidA, inSolidB, cache);
}

bool Classifier::runContainmentOnlyPreflightCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    _PreflightCache& cache)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolids(inSolidA, inSolidB);
    setNumericContext(&context);

    int relation = classifyNoIntersectionRelation(cache.aInB(), cache.bInA());

    // Restricted to strict containment only. An earlier attempt to
    // extend this preflight to "tangent containment" (one solid
    // sitting inside the other with all its boundary vertices on
    // the other's surface) regressed legitimate cases - those need
    // the regular pipeline because their result expects the inner
    // surface preserved as a hole/cut, not a plain merge. The two
    // absorption-step-2 fixtures (MANT1988_15_2_LIMIT,
    // MANT1988_6_13) that motivate this preflight are tracked in
    // plan section 7.3.1.D-cont as remaining drift.
    if ( relation != NO_INT_RELATION_A_IN_B &&
         relation != NO_INT_RELATION_B_IN_A ) {
        return false;
    }

    if ( cache.hasEdgeFaceIntersectionAB() ) {
        return false;
    }
    if ( cache.hasEdgeFaceIntersectionBA() ) {
        return false;
    }

    return true;
}

bool Classifier::runTouchingOnlyPreflightCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB)
{
    _PreflightCache cache = newPreflightCache(inSolidA, inSolidB);
    return runTouchingOnlyPreflightCase(inSolidA, inSolidB, cache);
}

bool Classifier::runTouchingOnlyPreflightCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    _PreflightCache& cache)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolids(inSolidA, inSolidB);
    setNumericContext(&context);

    int relation = classifyNoIntersectionRelation(cache.aInB(), cache.bInA());

    if ( relation != NO_INT_RELATION_TOUCHING ) {
        return false;
    }

    if ( cache.hasInteriorOverlap() ) {
        return false;
    }

    if ( cache.hasEdgeFaceIntersectionAB() ) {
        return false;
    }
    if ( cache.hasEdgeFaceIntersectionBA() ) {
        return false;
    }

    if ( hasPartialCoplanarFaceAreaOverlap(inSolidA, inSolidB) ) {
        return false;
    }

    return true;
}

PolyhedralBoundedSolid* Classifier::runSetOpNoIntersectionCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    PolyhedralBoundedSolid* outRes,
    int op)
{
    _PreflightCache cache = newPreflightCache(inSolidA, inSolidB);
    return runSetOpNoIntersectionCase(inSolidA, inSolidB, outRes, op, cache);
}

PolyhedralBoundedSolid* Classifier::runSetOpNoIntersectionCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    PolyhedralBoundedSolid* outRes,
    int op,
    _PreflightCache& cache)
{
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolids(inSolidA, inSolidB);
    setNumericContext(&context);

    int relation = classifyNoIntersectionRelation(cache.aInB(), cache.bInA());

    if ( op == INTERSECTION ) {
        if ( relation == NO_INT_RELATION_A_IN_B ) {
            outRes->merge(inSolidA);
        }
        else if ( relation == NO_INT_RELATION_B_IN_A ) {
            outRes->merge(inSolidB);
        }
        return outRes;
    }

    if ( op == UNION ) {
        if ( relation == NO_INT_RELATION_A_IN_B ) {
            outRes->merge(inSolidB);
        }
        else if ( relation == NO_INT_RELATION_B_IN_A ) {
            outRes->merge(inSolidA);
        }
        else {
            outRes->merge(inSolidA);
            outRes->merge(inSolidB);
        }
        return outRes;
    }

    if ( relation == NO_INT_RELATION_A_IN_B ) {
        return outRes;
    }
    if ( relation == NO_INT_RELATION_B_IN_A ) {
        outRes->merge(inSolidA);
        inSolidB->revert();
        outRes->merge(inSolidB);
        PolyhedralBoundedSolidTopologyEditing::compactIds(outRes);
        return outRes;
    }
    outRes->merge(inSolidA);
    return outRes;
}

PolyhedralBoundedSolid* Classifier::runPartialCoplanarFaceAreaCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    PolyhedralBoundedSolid* outRes,
    int op)
{
    _PreflightCache cache = newPreflightCache(inSolidA, inSolidB);
    return runPartialCoplanarFaceAreaCase(inSolidA, inSolidB, outRes, op, cache);
}

PolyhedralBoundedSolid* Classifier::runPartialCoplanarFaceAreaCase(
    PolyhedralBoundedSolid* inSolidA,
    PolyhedralBoundedSolid* inSolidB,
    PolyhedralBoundedSolid* outRes,
    int op,
    _PreflightCache& cache)
{
    size_t i;

    if ( op == UNION ) {
        return 0;
    }

    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolids(inSolidA, inSolidB);
    setNumericContext(&context);

    if ( cache.hasInteriorOverlap() ||
         cache.hasEdgeFaceIntersectionAB() ||
         cache.hasEdgeFaceIntersectionBA() ) {
        return 0;
    }

    java::ArrayList<Polygon> contactPolygons =
        partialCoplanarFaceAreaOverlapPolygons(inSolidA, inSolidB);
    if ( contactPolygons.isEmpty() ) {
        return 0;
    }

    if ( op == SUBTRACT ) {
        outRes->merge(inSolidA);
        return outRes;
    }

    for ( i = 0; i < contactPolygons.size(); i++ ) {
        PolyhedralBoundedSolid* lamina =
            createLaminaFromPolygon(contactPolygons[i]);
        if ( lamina->getPolygonsList().size() > 0 ) {
            outRes->merge(lamina);
        }
        // After merge the lamina is an empty shell
        delete lamina;
    }

    return outRes;
}

//= Point and solid classification ==================================

int Classifier::compareToZero(double value)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor
        ::compareToZero(value);
}

int Classifier::pointInFace(_PolyhedralBoundedSolidFace* face,
    const Vector3Dd& point)
{
    return _PolyhedralBoundedSolidSetGeometricPredicateProcessor
        ::pointInFace(face, point);
}

int Classifier::classifyPointAgainstSolid(PolyhedralBoundedSolid* solid,
    const FacePlanes& facePlanes,
    const Vector3Dd& point)
{
    long int i;
    int j;
    _PolyhedralBoundedSolidFace* face;
    double eps = numericContext.bigEpsilon();
    int insideVotes = 0;
    int outsideVotes = 0;

    if ( solid == 0 || solid->getPolygonsList().size() < 1 ) {
        return Geometry::OUTSIDE;
    }

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        face = solid->getPolygonsList().get(i);
        InfinitePlane* facePlane = facePlanes.get(i);
        if ( facePlane == 0 ) {
            continue;
        }
        if ( std::fabs(facePlane->pointDistance(point)) <= eps ) {
            if ( face->testPointInside(point, eps, facePlane) !=
                 Geometry::OUTSIDE ) {
                return Geometry::LIMIT;
            }
        }
    }

    Vector3Dd dirs[3] = {
        Vector3Dd(1.0, 0.371, 0.137),
        Vector3Dd(0.193, 1.0, 0.417),
        Vector3Dd(0.217, 0.173, 1.0)
    };

    for ( j = 0; j < 3; j++ ) {
        int hits = 0;
        bool ambiguous = false;
        java::ArrayList<double> distances;
        Ray ray(point, dirs[j]);

        for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
            face = solid->getPolygonsList().get(i);
            InfinitePlane* facePlane = facePlanes.get(i);
            if ( facePlane == 0 ) {
                ambiguous = true;
                break;
            }
            Ray rayHit(ray);
            Ray* hit = facePlane->doIntersectionFirstHit(rayHit);
            if ( hit == 0 ) {
                continue;
            }
            double t = hit->getT();
            Vector3Dd pi = hit->getOrigin().add(hit->getDirection().multiply(t));
            delete hit;
            if ( t <= eps ) {
                continue;
            }

            int status = face->testPointInside(pi, eps, facePlane);
            if ( status == Geometry::LIMIT ) {
                ambiguous = true;
                break;
            }
            if ( status == Geometry::INSIDE ) {
                bool duplicated = false;
                size_t k;
                for ( k = 0; k < distances.size(); k++ ) {
                    if ( std::fabs(distances[k] - t) <= eps ) {
                        duplicated = true;
                        break;
                    }
                }
                if ( !duplicated ) {
                    distances.add(t);
                    hits++;
                }
            }
        }

        if ( !ambiguous ) {
            if ( (hits % 2) == 1 ) {
                insideVotes++;
            }
            else {
                outsideVotes++;
            }
        }
    }

    if ( insideVotes > outsideVotes ) {
        return Geometry::INSIDE;
    }
    if ( outsideVotes > insideVotes ) {
        return Geometry::OUTSIDE;
    }
    return Geometry::LIMIT;
}

java::ArrayList<double> Classifier::overlappingBounds(
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB)
{
    double* a = solidA->getMinMax();
    double* b = solidB->getMinMax();
    java::ArrayList<double> bounds = java::Collections::nCopies<double>(6, 0.0);

    bounds[0] = java::Math::max(a[0], b[0]);
    bounds[1] = java::Math::max(a[1], b[1]);
    bounds[2] = java::Math::max(a[2], b[2]);
    bounds[3] = java::Math::min(a[3], b[3]);
    bounds[4] = java::Math::min(a[4], b[4]);
    bounds[5] = java::Math::min(a[5], b[5]);
    delete[] a;
    delete[] b;
    return bounds;
}

bool Classifier::hasPositiveOverlapVolume(const java::ArrayList<double>& bounds)
{
    double eps = numericContext.bigEpsilon();

    return bounds[3] - bounds[0] > eps &&
        bounds[4] - bounds[1] > eps &&
        bounds[5] - bounds[2] > eps;
}

/**
Cheap, constant-size existence probe for a point interior to both solids.
Tests the 27 quarter/center/three-quarter combinations of the overlap AABB
`bounds`. Returns true only on a genuine INSIDE/INSIDE witness, so a positive
result is always exact; a negative result is inconclusive and the caller
must still run the exhaustive grid.
*/
bool Classifier::hasInteriorOverlapWitnessInAabb(
    const java::ArrayList<double>& bounds,
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB,
    const FacePlanes& planesA,
    const FacePlanes& planesB)
{
    java::ArrayList<double> sx = axisProbeCoordinates(bounds[0], bounds[3]);
    java::ArrayList<double> sy = axisProbeCoordinates(bounds[1], bounds[4]);
    java::ArrayList<double> sz = axisProbeCoordinates(bounds[2], bounds[5]);
    size_t i;
    size_t j;
    size_t k;

    for ( i = 0; i < sx.size(); i++ ) {
        for ( j = 0; j < sy.size(); j++ ) {
            for ( k = 0; k < sz.size(); k++ ) {
                Vector3Dd sample(sx[i], sy[j], sz[k]);
                if ( classifyPointAgainstSolid(solidA, planesA, sample) ==
                     Geometry::INSIDE &&
                     classifyPointAgainstSolid(solidB, planesB, sample) ==
                     Geometry::INSIDE ) {
                    return true;
                }
            }
        }
    }
    return false;
}

java::ArrayList<double> Classifier::axisProbeCoordinates(double min, double max)
{
    double span = max - min;
    java::ArrayList<double> coordinates = java::Collections::nCopies<double>(3, 0.0);

    coordinates[0] = min + span / 4.0;
    coordinates[1] = min + span / 2.0;
    coordinates[2] = max - span / 4.0;
    return coordinates;
}

double Classifier::vertexCoordinate(_PolyhedralBoundedSolidVertex* vertex,
    int axis)
{
    if ( axis == 0 ) {
        return vertex->position.x();
    }
    if ( axis == 1 ) {
        return vertex->position.y();
    }
    return vertex->position.z();
}

void Classifier::appendInteriorVertexCoordinates(
    java::ArrayList<double>& coords,
    PolyhedralBoundedSolid* solid,
    int axis,
    double min,
    double max,
    double eps)
{
    long int i;

    if ( solid == 0 ) {
        return;
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        double c = vertexCoordinate(solid->getVerticesList().get(i), axis);
        if ( c > min + eps && c < max - eps ) {
            coords.add(c);
        }
    }
}

void Classifier::appendUniqueInteriorSample(
    java::ArrayList<double>& samples,
    double value,
    double min,
    double max,
    double eps)
{
    size_t i;

    if ( value <= min + eps || value >= max - eps ) {
        return;
    }

    for ( i = 0; i < samples.size(); i++ ) {
        if ( std::fabs(samples[i] - value) <= eps ) {
            return;
        }
    }
    samples.add(value);
}

namespace {

/**
Emulates `Collections.sort` over a list of `Double`: stable natural order
of `Double.compareTo`.
*/
void sortAsJavaDoubles(java::ArrayList<double>& values)
{
    values.sort([](double x, double y) { return java::Double::compare(x, y) < 0; });
}

}

java::ArrayList<double> Classifier::sampleCoordinates(double min, double max,
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB,
    int axis)
{
    java::ArrayList<double> coords;
    java::ArrayList<double> samples;
    java::ArrayList<double> uniqueCoords;
    double eps = numericContext.bigEpsilon();
    double center = (min + max) / 2.0;
    double quarter = min + (max - min) / 4.0;
    double threeQuarters = max - (max - min) / 4.0;
    size_t i;

    coords.add(min);
    coords.add(max);
    appendInteriorVertexCoordinates(coords, solidA, axis, min, max, eps);
    appendInteriorVertexCoordinates(coords, solidB, axis, min, max, eps);
    sortAsJavaDoubles(coords);

    for ( i = 0; i < coords.size(); i++ ) {
        double c = coords[i];
        if ( uniqueCoords.isEmpty() ||
             std::fabs(uniqueCoords[uniqueCoords.size()-1] - c) > eps ) {
            uniqueCoords.add(c);
        }
    }

    appendUniqueInteriorSample(samples, quarter, min, max, eps);
    appendUniqueInteriorSample(samples, center, min, max, eps);
    appendUniqueInteriorSample(samples, threeQuarters, min, max, eps);

    for ( i = 0; i + 1 < uniqueCoords.size(); i++ ) {
        double left = uniqueCoords[i];
        double right = uniqueCoords[i+1];
        if ( right - left > eps ) {
            appendUniqueInteriorSample(samples, (left + right) / 2.0,
                min, max, eps);
        }
    }

    sortAsJavaDoubles(samples);
    if ( samples.isEmpty() ) {
        samples.add(center);
    }
    return samples;
}

bool Classifier::hasConfirmedInteriorOverlap(
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB)
{
    size_t i;
    size_t j;
    size_t k;

    if ( solidA == 0 || solidB == 0 ) {
        return false;
    }

    java::ArrayList<double> bounds = overlappingBounds(solidA, solidB);
    if ( !hasPositiveOverlapVolume(bounds) ) {
        return false;
    }

    // Precompute each solid's face planes once; every probe/grid point is
    // classified against the same unmutated solids, so this removes the
    // per-point Newell/tolerance-context recompute (P2 scoped cache).
    FacePlanes planesA(solidA);
    FacePlanes planesB(solidB);

    // P1.2 - cheap early-positive pass: any INSIDE/INSIDE witness gives the
    // same answer as the exhaustive vertex-derived grid below, which remains
    // the exact fallback for the negative case.
    if ( hasInteriorOverlapWitnessInAabb(bounds, solidA, solidB,
             planesA, planesB) ) {
        return true;
    }

    java::ArrayList<double> xs = sampleCoordinates(bounds[0], bounds[3], solidA, solidB, 0);
    java::ArrayList<double> ys = sampleCoordinates(bounds[1], bounds[4], solidA, solidB, 1);
    java::ArrayList<double> zs = sampleCoordinates(bounds[2], bounds[5], solidA, solidB, 2);

    for ( i = 0; i < xs.size(); i++ ) {
        for ( j = 0; j < ys.size(); j++ ) {
            for ( k = 0; k < zs.size(); k++ ) {
                Vector3Dd sample(xs[i], ys[j], zs[k]);
                if ( classifyPointAgainstSolid(solidA, planesA, sample) ==
                     Geometry::INSIDE &&
                     classifyPointAgainstSolid(solidB, planesB, sample) ==
                     Geometry::INSIDE ) {
                    return true;
                }
            }
        }
    }

    return false;
}

int Classifier::classifySolidAgainstSolid(
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB)
{
    long int i;
    bool sawLimit = false;
    bool sawOutside = false;

    if ( solidA == 0 || solidA->getVerticesList().size() < 1 ) {
        return Geometry::OUTSIDE;
    }

    FacePlanes planesB(solidB);
    for ( i = 0; i < solidA->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solidA->getVerticesList().get(i);
        int status = classifyPointAgainstSolid(solidB, planesB, v->position);
        if ( status == Geometry::INSIDE ) {
            return Geometry::INSIDE;
        }
        if ( status == Geometry::LIMIT ) {
            sawLimit = true;
        }
        else {
            sawOutside = true;
        }
    }

    if ( sawLimit ) {
        return Geometry::LIMIT;
    }
    if ( sawOutside ) {
        return Geometry::OUTSIDE;
    }
    return Geometry::OUTSIDE;
}

int Classifier::classifyNoIntersectionRelation(int aInB, int bInA)
{
    if ( aInB == Geometry::INSIDE ) {
        return NO_INT_RELATION_A_IN_B;
    }
    if ( bInA == Geometry::INSIDE ) {
        return NO_INT_RELATION_B_IN_A;
    }
    if ( aInB == Geometry::LIMIT || bInA == Geometry::LIMIT ) {
        return NO_INT_RELATION_TOUCHING;
    }
    return NO_INT_RELATION_DISJOINT;
}

bool Classifier::hasProperEdgeFaceIntersection(
    PolyhedralBoundedSolid* current,
    PolyhedralBoundedSolid* other)
{
    long int i;
    long int j;
    _PolyhedralBoundedSolidEdge* edge;
    _PolyhedralBoundedSolidFace* face;
    _PolyhedralBoundedSolidVertex* v1;
    _PolyhedralBoundedSolidVertex* v2;
    double d1;
    double d2;
    double d3;
    double t;
    int s1;
    int s2;
    Vector3Dd p;

    if ( current == 0 || other == 0 ) {
        return false;
    }

    for ( i = 0; i < current->getEdgesList().size(); i++ ) {
        edge = current->getEdgesList().get(i);
        if ( edge == 0 || edge->rightHalf == 0 || edge->leftHalf == 0 ) {
            continue;
        }
        v1 = edge->rightHalf->startingVertex;
        v2 = edge->leftHalf->startingVertex;
        if ( v1 == 0 || v2 == 0 ) {
            continue;
        }

        for ( j = 0; j < other->getPolygonsList().size(); j++ ) {
            face = other->getPolygonsList().get(j);
            if ( face == 0 ) {
                continue;
            }
            InfinitePlane* facePlane = face->getContainingPlane();
            if ( facePlane == 0 ) {
                continue;
            }

            d1 = facePlane->pointDistance(v1->position);
            d2 = facePlane->pointDistance(v2->position);
            s1 = compareToZero(d1);
            s2 = compareToZero(d2);

            if ( !((s1 == -1 && s2 == 1) || (s1 == 1 && s2 == -1)) ) {
                delete facePlane;
                continue;
            }

            t = d1 / (d1 - d2);
            p = v1->position.add(
                v2->position.subtract(v1->position).multiply(t));
            d3 = facePlane->pointDistance(p);
            delete facePlane;
            if ( compareToZero(d3) != 0 ) {
                continue;
            }

            if ( pointInFace(face, p) == Geometry::INSIDE ) {
                return true;
            }
        }
    }
    return false;
}

//= Coplanar faces overlap ==========================================

bool Classifier::hasPartialCoplanarFaceAreaOverlap(
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB)
{
    long int i;
    long int j;

    if ( solidA == 0 || solidB == 0 ) {
        return false;
    }

    for ( i = 0; i < solidA->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* faceA = solidA->getPolygonsList().get(i);
        for ( j = 0; j < solidB->getPolygonsList().size(); j++ ) {
            _PolyhedralBoundedSolidFace* faceB = solidB->getPolygonsList().get(j);
            if ( coplanarFaces(faceA, faceB) &&
                 partialCoplanarFaceAreaOverlap(faceA, faceB) ) {
                return true;
            }
        }
    }

    return false;
}

java::ArrayList<Classifier::Polygon> Classifier::partialCoplanarFaceAreaOverlapPolygons(
    PolyhedralBoundedSolid* solidA,
    PolyhedralBoundedSolid* solidB)
{
    java::ArrayList<Polygon> polygons;
    long int i;
    long int j;

    if ( solidA == 0 || solidB == 0 ) {
        return polygons;
    }

    for ( i = 0; i < solidA->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* faceA = solidA->getPolygonsList().get(i);
        for ( j = 0; j < solidB->getPolygonsList().size(); j++ ) {
            _PolyhedralBoundedSolidFace* faceB = solidB->getPolygonsList().get(j);
            if ( coplanarFaces(faceA, faceB) &&
                 partialCoplanarFaceAreaOverlap(faceA, faceB) ) {
                Polygon polygon = coplanarFaceIntersectionPolygon(faceA, faceB);
                if ( polygon.size() >= 3 ) {
                    polygons.add(polygon);
                }
            }
        }
    }

    return polygons;
}

bool Classifier::coplanarFaces(_PolyhedralBoundedSolidFace* faceA,
    _PolyhedralBoundedSolidFace* faceB)
{
    if ( faceA == 0 || faceB == 0 ) {
        return false;
    }
    InfinitePlane* planeA = faceA->getContainingPlane();
    InfinitePlane* planeB = faceB->getContainingPlane();
    bool coplanar = false;

    if ( planeA != 0 && planeB != 0 &&
         PolyhedralBoundedSolidNumericPolicy::unitVectorsParallel(
             planeA->getNormal(), planeB->getNormal(), numericContext) &&
         faceB->boundariesList.size() >= 1 &&
         faceB->boundariesList.get(0)->boundaryStartHalfEdge != 0 ) {
        coplanar = std::fabs(planeA->pointDistance(
            faceB->boundariesList.get(0)->boundaryStartHalfEdge
                ->startingVertex->position)) <= numericContext.bigEpsilon();
    }
    delete planeA;
    delete planeB;
    return coplanar;
}

bool Classifier::partialCoplanarFaceAreaOverlap(
    _PolyhedralBoundedSolidFace* faceA,
    _PolyhedralBoundedSolidFace* faceB)
{
    if ( faceHasInteriorVertexOrEdgeMidpoint(faceA, faceB) ||
         faceHasInteriorVertexOrEdgeMidpoint(faceB, faceA) ) {
        return true;
    }

    return faceBoundariesCrossProperly(faceA, faceB);
}

Classifier::Polygon Classifier::coplanarFaceIntersectionPolygon(
    _PolyhedralBoundedSolidFace* faceA,
    _PolyhedralBoundedSolidFace* faceB)
{
    Polygon points;

    appendFaceVerticesInsideOther(points, faceA, faceB);
    appendFaceVerticesInsideOther(points, faceB, faceA);
    appendBoundaryIntersections(points, faceA, faceB);
    InfinitePlane* planeA = faceA->getContainingPlane();
    Vector3Dd planeNormalA = planeA->getNormal();
    delete planeA;
    sortCoplanarPolygon(points, planeNormalA);

    if ( coplanarPolygonAreaMagnitude(points, planeNormalA) <=
         numericContext.bigEpsilon() * numericContext.bigEpsilon() ) {
        points.clear();
    }

    return points;
}

void Classifier::appendFaceVerticesInsideOther(
    Polygon& points,
    _PolyhedralBoundedSolidFace* source,
    _PolyhedralBoundedSolidFace* target)
{
    long int i;

    for ( i = 0; i < source->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = source->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* start;
        _PolyhedralBoundedSolidHalfEdge* he;

        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
            continue;
        }
        start = loop->boundaryStartHalfEdge;
        he = start;
        do {
            if ( target->testPointInside(he->startingVertex->position,
                     numericContext.bigEpsilon()) != Geometry::OUTSIDE ) {
                appendUniquePoint(points, he->startingVertex->position);
            }
            he = he->next();
        } while ( he != 0 && he != start );
    }
}

void Classifier::appendBoundaryIntersections(
    Polygon& points,
    _PolyhedralBoundedSolidFace* faceA,
    _PolyhedralBoundedSolidFace* faceB)
{
    long int i;
    long int j;
    int dominantCoordinate = dominantCoordinateForFace(faceA);

    for ( i = 0; i < faceA->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loopA = faceA->boundariesList.get(i);
        if ( loopA == 0 ) {
            continue;
        }
        for ( j = 0; j < faceB->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loopB = faceB->boundariesList.get(j);
            if ( loopB != 0 ) {
                appendLoopIntersections(points, loopA, loopB,
                    dominantCoordinate);
            }
        }
    }
}

void Classifier::appendLoopIntersections(
    Polygon& points,
    _PolyhedralBoundedSolidLoop* loopA,
    _PolyhedralBoundedSolidLoop* loopB,
    int dominantCoordinate)
{
    long int i;
    long int j;

    for ( i = 0; i < loopA->halfEdgesList.size(); i++ ) {
        _PolyhedralBoundedSolidHalfEdge* heA = loopA->halfEdgesList.get(i);
        if ( heA == 0 || heA->next() == 0 ) {
            continue;
        }
        for ( j = 0; j < loopB->halfEdgesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* heB = loopB->halfEdgesList.get(j);
            if ( heB == 0 || heB->next() == 0 ) {
                continue;
            }
            appendSegmentIntersection(points, heA, heB, dominantCoordinate);
        }
    }
}

void Classifier::appendSegmentIntersection(
    Polygon& points,
    _PolyhedralBoundedSolidHalfEdge* heA,
    _PolyhedralBoundedSolidHalfEdge* heB,
    int dominantCoordinate)
{
    Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position,
        dominantCoordinate);
    Vector2Dd a2 = projectPointTo2D(heA->next()->startingVertex->position,
        dominantCoordinate);
    Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position,
        dominantCoordinate);
    Vector2Dd b2 = projectPointTo2D(heB->next()->startingVertex->position,
        dominantCoordinate);
    double den;
    double t;

    if ( !segmentsCrossProperly2D(a1, a2, b1, b2) ) {
        return;
    }

    Vector2Dd da(a2.x - a1.x, a2.y - a1.y);
    Vector2Dd db(b2.x - b1.x, b2.y - b1.y);
    Vector2Dd ba(b1.x - a1.x, b1.y - a1.y);
    den = cross2D(da, db);
    if ( std::fabs(den) <= numericContext.bigEpsilon() ) {
        return;
    }

    t = cross2D(ba, db) / den;
    appendUniquePoint(points, heA->startingVertex->position.add(
        heA->next()->startingVertex->position
            .subtract(heA->startingVertex->position).multiply(t)));
}

double Classifier::cross2D(const Vector2Dd& a, const Vector2Dd& b)
{
    return a.x*b.y - a.y*b.x;
}

void Classifier::appendUniquePoint(Polygon& points, const Vector3Dd& point)
{
    size_t i;

    for ( i = 0; i < points.size(); i++ ) {
        if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                points[i], point, numericContext) ) {
            return;
        }
    }
    points.add(point);
}

void Classifier::sortCoplanarPolygon(Polygon& points, const Vector3Dd& normal)
{
    Vector3Dd center;
    Vector3Dd u;
    Vector3Dd v;
    Vector3Dd n;
    size_t i;

    if ( points.size() < 3 ) {
        return;
    }

    for ( i = 0; i < points.size(); i++ ) {
        center = center.add(points[i]);
    }
    center = center.multiply(1.0 / (double)points.size());

    n = normal.normalized();
    u = points[0].subtract(center);
    if ( u.length() <= numericContext.bigEpsilon() ) {
        return;
    }
    u = u.normalized();
    v = n.crossProduct(u).normalized();

    // As Java `Collections.sort`, a stable sort by angle around the center
    points.sort([&center, &u, &v](const Vector3Dd& p1, const Vector3Dd& p2) {
            Vector3Dd d1 = p1.subtract(center);
            Vector3Dd d2 = p2.subtract(center);
            double a1 = std::atan2(d1.dotProduct(v), d1.dotProduct(u));
            double a2 = std::atan2(d2.dotProduct(v), d2.dotProduct(u));
            return java::Double::compare(a1, a2) < 0;
        });
}

double Classifier::coplanarPolygonAreaMagnitude(const Polygon& points,
    const Vector3Dd& normal)
{
    Vector3Dd accumulator;
    size_t i;

    if ( points.size() < 3 ) {
        return 0.0;
    }

    for ( i = 0; i < points.size(); i++ ) {
        const Vector3Dd& p = points[i];
        const Vector3Dd& q = points[(i+1)%points.size()];
        accumulator = accumulator.add(p.crossProduct(q));
    }

    return std::fabs(accumulator.dotProduct(normal.normalized())) * 0.5;
}

PolyhedralBoundedSolid* Classifier::createLaminaFromPolygon(
    const Polygon& points)
{
    PolyhedralBoundedSolid* solid;
    size_t i;

    solid = new PolyhedralBoundedSolid();
    if ( points.size() < 3 ) {
        return solid;
    }

    PolyhedralBoundedSolidEulerOperators::mvfs(solid, points[0], 1, 1);
    for ( i = 1; i < points.size(); i++ ) {
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, (int)i,
            (int)i + 1, points[i]);
    }
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, (int)points.size(),
        1, 2);
    return solid;
}

bool Classifier::faceHasInteriorVertexOrEdgeMidpoint(
    _PolyhedralBoundedSolidFace* source,
    _PolyhedralBoundedSolidFace* target)
{
    long int i;

    for ( i = 0; i < source->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = source->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* start;
        _PolyhedralBoundedSolidHalfEdge* he;

        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
            continue;
        }
        start = loop->boundaryStartHalfEdge;
        he = start;
        do {
            if ( target->testPointInside(he->startingVertex->position,
                     numericContext.bigEpsilon()) == Geometry::INSIDE ) {
                return true;
            }
            if ( he->next() != 0 ) {
                Vector3Dd midpoint = he->startingVertex->position.add(
                    he->next()->startingVertex->position
                        .subtract(he->startingVertex->position)
                        .multiply(0.5));
                if ( target->testPointInside(midpoint,
                         numericContext.bigEpsilon()) == Geometry::INSIDE ) {
                    return true;
                }
            }
            he = he->next();
        } while ( he != 0 && he != start );
    }

    return false;
}

bool Classifier::faceBoundariesCrossProperly(
    _PolyhedralBoundedSolidFace* faceA,
    _PolyhedralBoundedSolidFace* faceB)
{
    long int i;
    long int j;
    int dominantCoordinate = dominantCoordinateForFace(faceA);

    for ( i = 0; i < faceA->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loopA = faceA->boundariesList.get(i);
        if ( loopA == 0 ) {
            continue;
        }
        for ( j = 0; j < faceB->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loopB = faceB->boundariesList.get(j);
            if ( loopB != 0 &&
                 loopsCrossProperly(loopA, loopB, dominantCoordinate) ) {
                return true;
            }
        }
    }

    return false;
}

bool Classifier::loopsCrossProperly(_PolyhedralBoundedSolidLoop* loopA,
    _PolyhedralBoundedSolidLoop* loopB,
    int dominantCoordinate)
{
    long int i;
    long int j;

    for ( i = 0; i < loopA->halfEdgesList.size(); i++ ) {
        _PolyhedralBoundedSolidHalfEdge* heA = loopA->halfEdgesList.get(i);
        if ( heA == 0 || heA->next() == 0 ) {
            continue;
        }
        Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position,
            dominantCoordinate);
        Vector2Dd a2 = projectPointTo2D(heA->next()->startingVertex->position,
            dominantCoordinate);

        for ( j = 0; j < loopB->halfEdgesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* heB = loopB->halfEdgesList.get(j);
            if ( heB == 0 || heB->next() == 0 ) {
                continue;
            }
            Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position,
                dominantCoordinate);
            Vector2Dd b2 = projectPointTo2D(
                heB->next()->startingVertex->position, dominantCoordinate);
            if ( segmentsCrossProperly2D(a1, a2, b1, b2) ) {
                return true;
            }
        }
    }

    return false;
}

int Classifier::dominantCoordinateForFace(_PolyhedralBoundedSolidFace* face)
{
    InfinitePlane* plane = face->getContainingPlane();
    Vector3Dd n = plane->getNormal();
    delete plane;

    if ( std::fabs(n.x()) >= std::fabs(n.y()) &&
         std::fabs(n.x()) >= std::fabs(n.z()) ) {
        return 1;
    }
    if ( std::fabs(n.y()) >= std::fabs(n.x()) &&
         std::fabs(n.y()) >= std::fabs(n.z()) ) {
        return 2;
    }
    return 3;
}

Vector2Dd Classifier::projectPointTo2D(const Vector3Dd& in,
    int dominantCoordinate)
{
    if ( dominantCoordinate == 1 ) {
        return Vector2Dd(in.y(), in.z());
    }
    if ( dominantCoordinate == 2 ) {
        return Vector2Dd(in.x(), in.z());
    }
    return Vector2Dd(in.x(), in.y());
}

double Classifier::orientation2D(const Vector2Dd& a, const Vector2Dd& b,
    const Vector2Dd& c)
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

bool Classifier::segmentsCrossProperly2D(const Vector2Dd& a1,
    const Vector2Dd& a2, const Vector2Dd& b1, const Vector2Dd& b2)
{
    double o1 = orientation2D(a1, a2, b1);
    double o2 = orientation2D(a1, a2, b2);
    double o3 = orientation2D(b1, b2, a1);
    double o4 = orientation2D(b1, b2, a2);
    double tolerance = PolyhedralBoundedSolidNumericPolicy
        ::orientationTolerance2D(a1, a2, b1, numericContext);

    tolerance = java::Math::max(tolerance, PolyhedralBoundedSolidNumericPolicy
        ::orientationTolerance2D(a1, a2, b2, numericContext));
    tolerance = java::Math::max(tolerance, PolyhedralBoundedSolidNumericPolicy
        ::orientationTolerance2D(b1, b2, a1, numericContext));
    tolerance = java::Math::max(tolerance, PolyhedralBoundedSolidNumericPolicy
        ::orientationTolerance2D(b1, b2, a2, numericContext));

    return ((o1 > tolerance && o2 < -tolerance) ||
            (o1 < -tolerance && o2 > tolerance)) &&
           ((o3 > tolerance && o4 < -tolerance) ||
            (o3 < -tolerance && o4 > tolerance));
}
