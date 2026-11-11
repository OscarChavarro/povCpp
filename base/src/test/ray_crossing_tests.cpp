#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "RayCrossingTestCollector.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"

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
    Box box(1,1,1);
    Sphere sphere(1);
    Ray ray(Vector3Dd(3,0,0), Vector3Dd(1,0,0));
    RayCrossingTestCollector hits;
    EXPECT_EQ(RayCrossingSupport::Unsupported,
        box.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    EXPECT_EQ(RayCrossingSupport::Supported,
        sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    EXPECT_TRUE(hits.roots.isEmpty());
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
    Box unsupported(1,1,1);
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
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, bounds.getKind());
    bounds.setEmpty();
    EXPECT_EQ(GeometryBounds::Kind::Empty, bounds.getKind());
    bounds.setUnbounded();
    EXPECT_EQ(GeometryBounds::Kind::Unbounded, bounds.getKind());
    EXPECT_FALSE(bounds.setFinite(Vector3Dd(2,0,0), Vector3Dd(1,1,1)));
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, bounds.getKind());
    EXPECT_FALSE(bounds.setFinite(Vector3Dd(), Vector3Dd(java::Double::POSITIVE_INFINITY,1,1)));
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, bounds.getKind());
}
