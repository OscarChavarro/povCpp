#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "RayCrossingTestCollector.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"

TEST(RayCrossings, SpherePreservesAffineParameterAndOrdersRoots) {
    Sphere sphere(1);
    Ray ray;
    ray.setOriginAndDirection(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayCrossingTestCollector hits;
    EXPECT_EQ(RayCrossingSupport::Supported,
        sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    ASSERT_EQ(2u, hits.roots.size());
    EXPECT_DOUBLE_EQ(1, hits.roots[0]);
    EXPECT_DOUBLE_EQ(2, hits.roots[1]);
    EXPECT_EQ(&sphere, hits.geometries[0]);
    EXPECT_EQ(RayCrossing::Kind::Entering, hits.kinds[0]);
    EXPECT_EQ(RayCrossing::Kind::Leaving, hits.kinds[1]);
}

TEST(RayCrossings, InteriorTangentAndClippedIntervals) {
    Sphere sphere(1);
    RayCrossingTestCollector inside, tangent, clipped, zero;
    sphere.emitRayCrossings(Ray(Vector3Dd(0,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0,10), inside);
    ASSERT_EQ(1u, inside.roots.size());
    EXPECT_DOUBLE_EQ(1, inside.roots[0]);
    sphere.emitRayCrossings(Ray(Vector3Dd(-2,1,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0,10), tangent);
    ASSERT_EQ(1u, tangent.roots.size());
    EXPECT_DOUBLE_EQ(2, tangent.roots[0]);
    EXPECT_EQ(RayCrossing::Kind::Tangent, tangent.kinds[0]);
    sphere.emitRayCrossings(Ray(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(2.5,3.5), clipped);
    EXPECT_TRUE(clipped.roots.isEmpty());
    Ray invalid;
    invalid.setDirection(Vector3Dd(0,0,0));
    sphere.emitRayCrossings(invalid, RayCrossingQuery(0,10), zero);
    EXPECT_TRUE(zero.roots.isEmpty());
}

TEST(RayCrossings, UnsupportedIsDifferentFromMiss) {
    Torus unsupported(2,1);
    Sphere sphere(1);
    Ray ray(Vector3Dd(3,0,0), Vector3Dd(1,0,0));
    RayCrossingTestCollector hits;
    EXPECT_EQ(RayCrossingSupport::Unsupported,
        unsupported.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    EXPECT_EQ(RayCrossingSupport::Supported,
        sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    EXPECT_TRUE(hits.roots.isEmpty());
}

TEST(BoxContract, FirstHitCrossingsAndClippedSolidShareAffineParameters) {
    Box box(2,2,2);
    const Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayCrossingTestCollector hits;
    ASSERT_EQ(RayCrossingSupport::Supported, box.emitRayCrossings(ray, RayCrossingQuery(0,4), hits));
    ASSERT_EQ(2, hits.roots.size());
    EXPECT_DOUBLE_EQ(1, hits.roots[0]);
    EXPECT_DOUBLE_EQ(2, hits.roots[1]);
    EXPECT_EQ(RayCrossing::Kind::Entering, hits.kinds[0]);
    EXPECT_EQ(RayCrossing::Kind::Leaving, hits.kinds[1]);
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit, box.queryFirstHit(ray, RayCrossingQuery(0,4), &hit, &boundary));
    EXPECT_DOUBLE_EQ(1, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(-1, hit.point.x());
    EXPECT_DOUBLE_EQ(-1, hit.normal.x());
    EXPECT_EQ(&box, boundary.geometry);
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, hit.getAvailableDetailMask());
    EXPECT_EQ(RayFirstHitStatus::Miss, box.queryFirstHit(ray, RayCrossingQuery(1.1,1.9), &hit));
    RaySolidClassification solid;
    ASSERT_EQ(RaySolidQueryStatus::Supported, box.classifyRay(ray, RayCrossingQuery(1.1,1.9), solid));
    ASSERT_EQ(1, solid.insideSegments.size());
    EXPECT_TRUE(solid.insideSegments[0]);
    EXPECT_TRUE(solid.boundaries.isEmpty());
    ASSERT_EQ(RaySolidQueryStatus::Supported, box.classifyRay(ray, RayCrossingQuery(1,2), solid));
    ASSERT_EQ(2, solid.boundaries.size());
    ASSERT_EQ(3, solid.insideSegments.size());
    EXPECT_FALSE(solid.insideSegments[0]);
    EXPECT_TRUE(solid.insideSegments[1]);
    EXPECT_FALSE(solid.insideSegments[2]);
}

TEST(BoxContract, TangencyOverlapAndNumericalFailuresAreExplicit) {
    Box box(2,2,2);
    RayCrossingTestCollector tangent;
    const Ray corner = Ray::fromAffine(Vector3Dd(-2,0,0), Vector3Dd(1,1,0));
    ASSERT_EQ(RayCrossingSupport::Supported, box.emitRayCrossings(corner, RayCrossingQuery(0,4), tangent));
    ASSERT_EQ(1, tangent.roots.size());
    EXPECT_DOUBLE_EQ(1, tangent.roots[0]);
    EXPECT_EQ(RayCrossing::Kind::Tangent, tangent.kinds[0]);
    RaySolidClassification solid;
    ASSERT_EQ(RaySolidQueryStatus::Supported, box.classifyRay(corner, RayCrossingQuery(0,4), solid));
    ASSERT_EQ(1, solid.insideSegments.size());
    EXPECT_FALSE(solid.insideSegments[0]);
    const Ray alongFace = Ray::fromAffine(Vector3Dd(-2,1,0), Vector3Dd(1,0,0));
    RayCrossingTestCollector overlap;
    EXPECT_EQ(RayCrossingSupport::Indeterminate, box.emitRayCrossings(alongFace, RayCrossingQuery(0,4), overlap));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, box.classifyRay(alongFace, RayCrossingQuery(0,4), solid));
    EXPECT_TRUE(solid.insideSegments.isEmpty());
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, box.queryFirstHit(
        Ray::fromAffine(Vector3Dd(), Vector3Dd()), RayCrossingQuery(0,4), nullptr));
    EXPECT_EQ(RayFirstHitStatus::Unsupported, box.queryFirstHit(
        corner, RayCrossingQuery(0,4,0.001), nullptr));
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, box.queryFirstHit(
        Ray::fromAffine(Vector3Dd(-1e300,0,0), Vector3Dd(1,0,0)),
        RayCrossingQuery(0,java::Double::POSITIVE_INFINITY), nullptr));
    RayCrossingTestCollector parallelMiss;
    EXPECT_EQ(RayCrossingSupport::Supported, box.emitRayCrossings(
        Ray::fromAffine(Vector3Dd(0,2,0), Vector3Dd(1,0,0)),
        RayCrossingQuery(0,4), parallelMiss));
    EXPECT_TRUE(parallelMiss.roots.isEmpty());
    Box invalid(2,-2,2);
    EXPECT_EQ(RayFirstHitStatus::InvalidInput,
        invalid.queryFirstHit(corner, RayCrossingQuery(0,4), nullptr));
}

TEST(BoxContract, EuclideanContainmentBoundsAndIndependentCopy) {
    Box box(2,4,6);
    EXPECT_EQ(GeometryContainment::Inside, box.classifyPoint(Vector3Dd(0,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, box.classifyPoint(Vector3Dd(1,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, box.classifyPoint(Vector3Dd(1.05,2.05,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, box.classifyPoint(Vector3Dd(1.08,2.08,0), 0.1));
    EXPECT_EQ(GeometryContainment::InvalidInput, box.classifyPoint(Vector3Dd(), -1));
    GeometryBounds bounds;
    box.getBounds(bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    EXPECT_DOUBLE_EQ(-1, bounds.getMinimum().x());
    EXPECT_DOUBLE_EQ(3, bounds.getMaximum().z());
    Geometry* copy = box.copyGeometry();
    ASSERT_NE(nullptr, copy);
    box.setSize(4,4,4);
    EXPECT_EQ(GeometryContainment::Outside, copy->classifyPoint(Vector3Dd(1.5,0,0), 0));
    delete copy;
}

TEST(BoxContract, ArbitraryBoundsAndParallelPolicy) {
    const Vector3Dd minimum(-2,-1,-3), maximum(4,3,1);
    double entry = java::Double::NEGATIVE_INFINITY, exit = java::Double::POSITIVE_INFINITY;
    bool overlap = false;
    ASSERT_EQ(RayFirstHitStatus::Hit, Box::intersectBounds(Vector3Dd(-4,0,0), Vector3Dd(1,0,0),
        minimum, maximum, 0, true, entry, exit, overlap));
    EXPECT_DOUBLE_EQ(2, entry);
    EXPECT_DOUBLE_EQ(8, exit);
    EXPECT_FALSE(overlap);
    entry = 0; exit = java::Double::POSITIVE_INFINITY;
    EXPECT_EQ(RayFirstHitStatus::Miss, Box::intersectBounds(Vector3Dd(5,0,0),
        Vector3Dd(-1e-8,1,0), minimum, maximum, 1e-7, false, entry, exit, overlap));
    entry = java::Double::NEGATIVE_INFINITY; exit = java::Double::POSITIVE_INFINITY;
    ASSERT_EQ(RayFirstHitStatus::Hit, Box::intersectBounds(Vector3Dd(-4,3,0), Vector3Dd(1,0,0),
        minimum, maximum, 0, true, entry, exit, overlap));
    EXPECT_TRUE(overlap);
}

TEST(InfinitePlaneContract, SurfaceQueryPreservesAffineParameterAndDetail) {
    InfinitePlane plane(2,0,0,-2);
    const Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayCrossingTestCollector crossings;
    ASSERT_EQ(RayCrossingSupport::Supported, plane.emitRayCrossings(ray, RayCrossingQuery(0,4), crossings));
    ASSERT_EQ(1, crossings.roots.size());
    EXPECT_DOUBLE_EQ(2, crossings.roots[0]);
    EXPECT_EQ(RayCrossing::Kind::Unclassified, crossings.kinds[0]);
    EXPECT_EQ(&plane, crossings.geometries[0]);
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit, plane.queryFirstHit(ray, RayCrossingQuery(2,2), &hit, &boundary));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_DOUBLE_EQ(1, hit.normal.x());
    EXPECT_EQ(&plane, boundary.geometry);
    EXPECT_EQ(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, hit.getAvailableDetailMask());
    EXPECT_EQ(RayFirstHitStatus::Miss, plane.queryFirstHit(ray, RayCrossingQuery(0,1), &hit));
    EXPECT_FALSE(hit.hasHitDistance());
    RaySolidClassification solid;
    EXPECT_EQ(RaySolidQueryStatus::Unsupported, plane.classifyRay(ray, RayCrossingQuery(0,4), solid));
    EXPECT_EQ(GeometryContainment::Unsupported, plane.classifyPoint(Vector3Dd(), 0));
}

TEST(InfinitePlaneContract, ParallelCoincidentDegenerateAndBoundsAreExplicit) {
    InfinitePlane plane(1,0,0,0);
    RayCrossingTestCollector events;
    const Ray parallel = Ray::fromAffine(Vector3Dd(1,0,0), Vector3Dd(0,1,0));
    const Ray coincident = Ray::fromAffine(Vector3Dd(), Vector3Dd(0,1,0));
    EXPECT_EQ(RayCrossingSupport::Supported, plane.emitRayCrossings(parallel, RayCrossingQuery(0,4), events));
    EXPECT_TRUE(events.roots.isEmpty());
    EXPECT_EQ(RayCrossingSupport::Indeterminate, plane.emitRayCrossings(coincident, RayCrossingQuery(0,4), events));
    InfinitePlane invalid(0,0,0,1);
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, invalid.queryFirstHit(parallel, RayCrossingQuery(0,4), nullptr));
    InfinitePlane tiny(1e-200,0,0,0);
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, tiny.queryFirstHit(parallel, RayCrossingQuery(0,4), nullptr));
    InfinitePlane shallow(1e-100,0,0,1);
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, shallow.queryFirstHit(
        Ray::fromAffine(Vector3Dd(), Vector3Dd(1e-250,1,0)), RayCrossingQuery(0,4), nullptr));
    GeometryBounds bounds;
    plane.getBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Unbounded, bounds.getKind());
    invalid.getBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, bounds.getKind());
    Geometry* copy = plane.copyGeometry();
    ASSERT_NE(nullptr, copy);
    plane.setD(-5);
    EXPECT_EQ(RayFirstHitStatus::Hit, copy->queryFirstHit(
        Ray::fromAffine(Vector3Dd(-2,0,0), Vector3Dd(1,0,0)), RayCrossingQuery(2,2), nullptr));
    delete copy;
    double depth = 0;
    EXPECT_EQ(RayFirstHitStatus::Miss,
        InfinitePlane::intersectionDepth(0.001, 0.0005, 0.001, 0, 10, false, depth));
    ASSERT_EQ(RayFirstHitStatus::Hit,
        InfinitePlane::intersectionDepth(0.001, 0.001, 0.001, 0, 10, false, depth));
    EXPECT_DOUBLE_EQ(1, depth);
}

TEST(PlanarHalfSpaceContract, ContainmentBoundsAndIndependentCopy) {
    PlanarHalfSpace half(2,0,0,-2);
    EXPECT_EQ(GeometryContainment::Inside, half.classifyPoint(Vector3Dd(0,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, half.classifyPoint(Vector3Dd(1,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, half.classifyPoint(Vector3Dd(1.05,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, half.classifyPoint(Vector3Dd(1.2,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::InvalidInput, half.classifyPoint(Vector3Dd(), -1));
    PlanarHalfSpace invalid(0,0,0,1);
    EXPECT_EQ(GeometryContainment::InvalidInput, invalid.classifyPoint(Vector3Dd(), 0));
    PlanarHalfSpace tiny(1e-200,0,0,0);
    EXPECT_EQ(GeometryContainment::Indeterminate, tiny.classifyPoint(Vector3Dd(), 0));
    PlanarHalfSpace shallow(1e-100,0,0,0);
    EXPECT_EQ(GeometryContainment::Indeterminate, shallow.classifyPoint(Vector3Dd(1e-250,0,0), 0));
    GeometryBounds bounds;
    half.getBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Unbounded, bounds.getKind());
    Geometry* copy = half.copyGeometry();
    ASSERT_NE(nullptr, copy);
    EXPECT_EQ(GeometryContainment::Inside, copy->classifyPoint(Vector3Dd(), 0));
    delete copy;
}

TEST(PlanarHalfSpaceContract, OrderedCrossingAndEndpointStates) {
    PlanarHalfSpace half(2,0,0,-2);
    const Ray leaving = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayCrossingTestCollector crossings;
    ASSERT_EQ(RayCrossingSupport::Supported, half.emitRayCrossings(leaving, RayCrossingQuery(0,4), crossings));
    ASSERT_EQ(1, crossings.roots.size());
    EXPECT_DOUBLE_EQ(2, crossings.roots[0]);
    EXPECT_EQ(RayCrossing::Kind::Leaving, crossings.kinds[0]);
    EXPECT_EQ(&half, crossings.geometries[0]);
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit, half.queryFirstHit(leaving, RayCrossingQuery(2,2), &hit, &boundary));
    EXPECT_EQ(&half, boundary.geometry);
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_DOUBLE_EQ(1, hit.normal.x());
    RaySolidClassification solid;
    ASSERT_EQ(RaySolidQueryStatus::Supported, half.classifyRay(leaving, RayCrossingQuery(2,4), solid));
    ASSERT_EQ(1, solid.boundaries.size());
    ASSERT_EQ(2, solid.insideSegments.size());
    EXPECT_TRUE(solid.insideSegments[0]);
    EXPECT_FALSE(solid.insideSegments[1]);
    const Ray entering = Ray::fromAffine(Vector3Dd(3,0,0), Vector3Dd(-2,0,0));
    RayCrossingTestCollector opposite;
    ASSERT_EQ(RayCrossingSupport::Supported, half.emitRayCrossings(entering, RayCrossingQuery(0,4), opposite));
    ASSERT_EQ(1, opposite.roots.size());
    EXPECT_DOUBLE_EQ(1, opposite.roots[0]);
    EXPECT_EQ(RayCrossing::Kind::Entering, opposite.kinds[0]);
    ASSERT_EQ(RaySolidQueryStatus::Supported, half.classifyRay(entering, RayCrossingQuery(1,4), solid));
    ASSERT_EQ(2, solid.insideSegments.size());
    EXPECT_FALSE(solid.insideSegments[0]);
    EXPECT_TRUE(solid.insideSegments[1]);
}

TEST(PlanarHalfSpaceContract, ParallelAndClippedIntervalsRemainDistinct) {
    PlanarHalfSpace half(1,0,0,0);
    RaySolidClassification solid;
    ASSERT_EQ(RaySolidQueryStatus::Supported, half.classifyRay(
        Ray::fromAffine(Vector3Dd(-1,0,0), Vector3Dd(0,1,0)), RayCrossingQuery(0,4), solid));
    ASSERT_EQ(1, solid.insideSegments.size());
    EXPECT_TRUE(solid.insideSegments[0]);
    ASSERT_EQ(RaySolidQueryStatus::Supported, half.classifyRay(
        Ray::fromAffine(Vector3Dd(1,0,0), Vector3Dd(0,1,0)), RayCrossingQuery(0,4), solid));
    ASSERT_EQ(1, solid.insideSegments.size());
    EXPECT_FALSE(solid.insideSegments[0]);
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, half.classifyRay(
        Ray::fromAffine(Vector3Dd(), Vector3Dd(0,1,0)), RayCrossingQuery(0,4), solid));
    EXPECT_TRUE(solid.insideSegments.isEmpty());
    ASSERT_EQ(RaySolidQueryStatus::Supported, half.classifyRay(
        Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(1,0,0)), RayCrossingQuery(0,2), solid));
    ASSERT_EQ(1, solid.insideSegments.size());
    EXPECT_TRUE(solid.insideSegments[0]);
    EXPECT_TRUE(solid.boundaries.isEmpty());
}

#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "java/lang/Double.h"

TEST(RaySolidClassification, NonunitRayAndClippedInterior) {
    Sphere sphere(1);
    Ray ray;
    ray.setOriginAndDirection(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RaySolidClassification result;
    EXPECT_EQ(RaySolidQueryStatus::Supported,
        sphere.classifyRay(ray, RayCrossingQuery(0,4), result));
    ASSERT_EQ(2u, result.boundaries.size());
    EXPECT_DOUBLE_EQ(1, result.boundaries[0].t);
    EXPECT_DOUBLE_EQ(2, result.boundaries[1].t);
    EXPECT_EQ(&sphere, result.boundaries[0].geometry);
    EXPECT_FALSE(result.boundaries[0].normalReversed);
    ASSERT_EQ(3L, result.insideSegments.size());
    EXPECT_EQ(false, result.insideSegments.get(0));
    EXPECT_EQ(true, result.insideSegments.get(1));
    EXPECT_EQ(false, result.insideSegments.get(2));
    EXPECT_EQ(RaySolidQueryStatus::Supported,
        sphere.classifyRay(ray, RayCrossingQuery(1.25,1.75), result));
    EXPECT_TRUE(result.boundaries.isEmpty());
    ASSERT_EQ(1L, result.insideSegments.size());
    EXPECT_EQ(true, result.insideSegments.get(0));
}

TEST(RaySolidClassification, InteriorStartAndEndpointLimitingStates) {
    Sphere sphere(1);
    Ray ray;
    ray.setOriginAndDirection(Vector3Dd(0,0,0), Vector3Dd(2,0,0));
    RaySolidClassification result;
    sphere.classifyRay(ray, RayCrossingQuery(0,2), result);
    ASSERT_EQ(1u, result.boundaries.size());
    EXPECT_DOUBLE_EQ(0.5, result.boundaries[0].t);
    ASSERT_EQ(2L, result.insideSegments.size());
    EXPECT_EQ(true, result.insideSegments.get(0));
    EXPECT_EQ(false, result.insideSegments.get(1));
    ray.setOrigin(Vector3Dd(-3,0,0));
    sphere.classifyRay(ray, RayCrossingQuery(1,2), result);
    EXPECT_EQ(2u, result.boundaries.size());
    ASSERT_EQ(3L, result.insideSegments.size());
    EXPECT_EQ(false, result.insideSegments.get(0));
    EXPECT_EQ(true, result.insideSegments.get(1));
    EXPECT_EQ(false, result.insideSegments.get(2));
    sphere.classifyRay(ray, RayCrossingQuery(2,3), result);
    ASSERT_EQ(1u, result.boundaries.size());
    EXPECT_DOUBLE_EQ(2, result.boundaries[0].t);
    ASSERT_EQ(2L, result.insideSegments.size());
    EXPECT_EQ(true, result.insideSegments.get(0));
    EXPECT_EQ(false, result.insideSegments.get(1));
}

TEST(RaySolidClassification, TangencyIsASurfaceEventButNotAnInterior) {
    Sphere sphere(1);
    Ray ray(Vector3Dd(-2,1,0), Vector3Dd(1,0,0));
    RayCrossingTestCollector crossings;
    sphere.emitRayCrossings(ray, RayCrossingQuery(0,4), crossings);
    ASSERT_EQ(1u, crossings.roots.size());
    EXPECT_EQ(RayCrossingKind::Tangent, crossings.kinds[0]);
    RaySolidClassification result;
    EXPECT_EQ(RaySolidQueryStatus::Supported,
        sphere.classifyRay(ray, RayCrossingQuery(0,4), result));
    EXPECT_TRUE(result.boundaries.isEmpty());
    ASSERT_EQ(1L, result.insideSegments.size());
    EXPECT_EQ(false, result.insideSegments.get(0));
}

TEST(RaySolidClassification, MissAndUnboundedInterval) {
    Sphere sphere(1);
    RaySolidClassification result;
    Ray ray(Vector3Dd(-3,2,0), Vector3Dd(1,0,0));
    sphere.classifyRay(ray, RayCrossingQuery(0,4), result);
    EXPECT_TRUE(result.boundaries.isEmpty());
    ASSERT_EQ(1L, result.insideSegments.size());
    EXPECT_EQ(false, result.insideSegments.get(0));
    ray.setOrigin(Vector3Dd(-3,0,0));
    const double inf = java::Double::POSITIVE_INFINITY;
    sphere.classifyRay(ray, RayCrossingQuery(-inf,inf), result);
    ASSERT_EQ(2u, result.boundaries.size());
    EXPECT_DOUBLE_EQ(2, result.boundaries[0].t);
    EXPECT_DOUBLE_EQ(4, result.boundaries[1].t);
    ASSERT_EQ(3L, result.insideSegments.size());
    EXPECT_EQ(false, result.insideSegments.get(0));
    EXPECT_EQ(true, result.insideSegments.get(1));
    EXPECT_EQ(false, result.insideSegments.get(2));
}

TEST(RaySolidClassification, InvalidUnsupportedAndIndeterminateClearReusedResult) {
    Sphere sphere(1);
    Torus unsupported(2,1);
    Ray ray(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
    RaySolidClassification result;
    sphere.classifyRay(ray, RayCrossingQuery(0,4), result);
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,
        unsupported.classifyRay(ray, RayCrossingQuery(0,4), result));
    EXPECT_TRUE(result.boundaries.isEmpty());
    EXPECT_TRUE(result.insideSegments.isEmpty());
    const RayCrossingQuery invalidQueries[] = {RayCrossingQuery(4,0), RayCrossingQuery(1,1),
            RayCrossingQuery(0,4,0.001), RayCrossingQuery(0,4,0,0.001),
            RayCrossingQuery(java::Double::NaN,4)};
    for (const RayCrossingQuery& query : invalidQueries) {
        sphere.classifyRay(ray, RayCrossingQuery(0,4), result);
        EXPECT_EQ(RaySolidQueryStatus::InvalidInput, sphere.classifyRay(ray, query, result));
        EXPECT_TRUE(result.boundaries.isEmpty());
        EXPECT_TRUE(result.insideSegments.isEmpty());
    }
    ray.setDirection(Vector3Dd(0,0,0));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput,
        sphere.classifyRay(ray, RayCrossingQuery(0,4), result));
    ray.setDirection(Vector3Dd(1e300,0,0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate,
        sphere.classifyRay(ray, RayCrossingQuery(0,4), result));
    EXPECT_TRUE(result.insideSegments.isEmpty());
}


TEST(RaySolidClassification, UnrepresentableRadiusIsNotReportedAsEmpty) {
    Sphere tiny(1e-300);
    Ray ray(Vector3Dd(0,0,0), Vector3Dd(1,0,0));
    RaySolidClassification result;
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate,
        tiny.classifyRay(ray, RayCrossingQuery(-1e-301,1e-301), result));
    EXPECT_TRUE(result.boundaries.isEmpty());
    EXPECT_TRUE(result.insideSegments.isEmpty());
}


TEST(RaySolidClassification, NativeListsRetainCapacityAndCopiesAreIndependent) {
    Sphere sphere(1);
    Ray ray(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
    RaySolidClassification result;
    sphere.classifyRay(ray, RayCrossingQuery(0,5), result);
    ASSERT_EQ(2L, result.boundaries.size());
    RaySolidBoundary* boundaryStorage = result.boundaries.data();
    bool* segmentStorage = result.insideSegments.data();
    RaySolidClassification copied(result);
    RaySolidClassification assigned;
    assigned = result;
    sphere.classifyRay(ray, RayCrossingQuery(2.5,3.5), result);
    EXPECT_TRUE(result.boundaries.isEmpty());
    EXPECT_EQ(boundaryStorage, result.boundaries.data());
    EXPECT_EQ(segmentStorage, result.insideSegments.data());
    ASSERT_EQ(2L, copied.boundaries.size());
    ASSERT_EQ(2L, assigned.boundaries.size());
    copied.boundaries[0].t = 99;
    EXPECT_DOUBLE_EQ(2, assigned.boundaries[0].t);
    EXPECT_EQ(3L, copied.insideSegments.size());
    EXPECT_EQ(1L, result.insideSegments.size());
    EXPECT_TRUE(result.insideSegments.get(0));
}

TEST(RaySolidClassification, JavaDoubleClassifiesSpecialValues) {
    EXPECT_TRUE(java::Double::isNaN(java::Double::NaN));
    EXPECT_FALSE(java::Double::isFinite(java::Double::NaN));
    EXPECT_FALSE(java::Double::isFinite(java::Double::POSITIVE_INFINITY));
    EXPECT_FALSE(java::Double::isFinite(java::Double::NEGATIVE_INFINITY));
    EXPECT_TRUE(java::Double::isFinite(java::Double::MAX_VALUE));
    EXPECT_TRUE(java::Double::isFinite(java::Double::MIN_VALUE));
    EXPECT_FALSE(java::Double::isNaN(0));
}


#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
TEST(GeometryBoundsContract, ExplicitStatesAndSphereBounds) {
    GeometryBounds bounds;
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, bounds.getKind());
    Sphere sphere(2);
    sphere.getBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    EXPECT_DOUBLE_EQ(-2, bounds.getMinimum().x());
    EXPECT_DOUBLE_EQ(2, bounds.getMaximum().z());
    Box box(1,1,1);
    box.getBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    bounds.setEmpty();
    EXPECT_EQ(GeometryBounds::Kind::Empty, bounds.getKind());
    bounds.setUnbounded();
    EXPECT_EQ(GeometryBounds::Kind::Unbounded, bounds.getKind());
    EXPECT_FALSE(bounds.setFinite(Vector3Dd(2,0,0), Vector3Dd(1,1,1)));
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, bounds.getKind());
    EXPECT_FALSE(bounds.setFinite(Vector3Dd(), Vector3Dd(java::Double::POSITIVE_INFINITY,1,1)));
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, bounds.getKind());
}
