#include <gtest/gtest.h>
#include <cmath>
#include "java/lang/Double.h"
#include "java/lang/IllegalArgumentException.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "ImplicitSolidTestFactory.h"
#include "RayCrossingTestCollector.h"

TEST(PolynomialSurfaceContract, TermIndicesFollowPovRayOrderAndRoundTrip) {
    EXPECT_EQ(PolynomialSurface::getTermIndex(4, 4, 0, 0), 0);
    EXPECT_EQ(PolynomialSurface::getTermIndex(4, 1, 3, 0), 10);
    EXPECT_EQ(PolynomialSurface::getTermIndex(4, 0, 0, 0), 34);
    EXPECT_EQ(PolynomialSurface::getTermIndex(2, 1, 1, 1), -1);
    EXPECT_EQ(PolynomialSurface::getTermIndex(3, -1, 0, 0), -1);
    for (int order = PolynomialSurface::MINIMUM_ORDER; order <= PolynomialSurface::MAXIMUM_ORDER; ++order) {
        for (int i = 0; i < PolynomialSurface::getTermCount(order); ++i) {
            int x, y, z, w;
            PolynomialSurface::getTermPowers(order, i, x, y, z, w);
            EXPECT_EQ(x + y + z + w, order);
            EXPECT_EQ(PolynomialSurface::getTermIndex(order, x, y, z), i);
        }
    }
    EXPECT_THROW(PolynomialSurface(1, false), java::IllegalArgumentException);
    EXPECT_THROW(PolynomialSurface(8, false), java::IllegalArgumentException);
}

TEST(PolynomialSurfaceContract, RayPolynomialsReproduceTheFieldAlongTheRay) {
    PolynomialSurface quartic(4, false);
    for (int i = 0; i < 35; ++i) quartic.getCoefficients()[i] = 0.25*((i*7)%11) - 1.0;
    const Vector3Dd origin(0.3, -0.7, 1.1), direction(0.48, 0.6, 0.64);
    double special[5], general[5];
    quartic.getQuarticRayPolynomial(origin, direction, special);
    quartic.getRayPolynomial(origin, direction, general, 0.0);
    for (double t = -2; t <= 2; t += 0.5) {
        const double expected = quartic.evaluate(origin.add(direction.multiply(t)));
        EXPECT_NEAR(ImplicitSolidTestFactory::evaluateUnivariate(special, 4, t), expected, 1e-9*(1 + std::fabs(expected)));
        EXPECT_NEAR(ImplicitSolidTestFactory::evaluateUnivariate(general, 4, t), expected, 1e-9*(1 + std::fabs(expected)));
    }
}

TEST(PolynomialSurfaceContract, TorusCrossingsClassificationAndDetail) {
    for (int sturm = 0; sturm < 2; ++sturm) {
        const PolynomialSurface solid = ImplicitSolidTestFactory::createTorus(sturm != 0);
        const Ray ray = Ray::fromAffine(Vector3Dd(-5, 0, 0), Vector3Dd(2, 0, 0));
        const RayCrossingQuery query(0, java::Double::POSITIVE_INFINITY);
        RayCrossingTestCollector crossings;
        ASSERT_EQ(solid.emitRayCrossings(ray, query, crossings), RayCrossingSupport::Supported);
        ASSERT_EQ(crossings.roots.size(), 4);
        const double expected[4] = {1.25, 1.75, 3.25, 3.75};
        for (int i = 0; i < 4; ++i) {
            EXPECT_NEAR(crossings.roots[i], expected[i], 1e-9);
            EXPECT_EQ(crossings.kinds[i], i % 2 == 0 ? RayCrossingKind::Entering : RayCrossingKind::Leaving);
        }
        RaySolidClassification classification;
        ASSERT_EQ(solid.classifyRay(ray, query, classification), RaySolidQueryStatus::Supported);
        ASSERT_EQ(classification.boundaries.size(), 4);
        ASSERT_EQ(classification.insideSegments.size(), 5);
        for (int i = 0; i < 5; ++i) EXPECT_EQ(classification.insideSegments[i], i % 2 == 1);

        RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
        RayBoundary boundary;
        ASSERT_EQ(solid.queryFirstHit(ray, query, &hit, &boundary), RayFirstHitStatus::Hit);
        EXPECT_NEAR(boundary.t, 1.25, 1e-9);
        EXPECT_NEAR(hit.point.x(), -2.5, 1e-9);
        EXPECT_NEAR(hit.normal.x(), -1, 1e-9);
        EXPECT_EQ(solid.queryFirstHit(Ray(Vector3Dd(0, 0, 5), Vector3Dd(0, 0, -1)), query, nullptr),
            RayFirstHitStatus::Miss);
    }
}

TEST(PolynomialSurfaceContract, CertifiedContainmentAndUnboundedBounds) {
    const PolynomialSurface solid = ImplicitSolidTestFactory::createTorus(false);
    EXPECT_EQ(solid.classifyPointCertified(Vector3Dd(2, 0, 0), 0.1), GeometryContainment::Inside);
    EXPECT_EQ(solid.classifyPointCertified(Vector3Dd(0, 0, 0), 0.1), GeometryContainment::Outside);
    EXPECT_EQ(solid.classifyPointCertified(Vector3Dd(2.5, 0, 0), 0.01), GeometryContainment::Boundary);
    EXPECT_EQ(solid.classifyPoint(Vector3Dd(0, 2, 0.2), 0), GeometryContainment::Inside);
    EXPECT_EQ(solid.classifyBoxCertified(Vector3Dd(0, 0, 3), Vector3Dd(0.5, 0.5, 0.5)),
        GeometryContainment::Outside);
    GeometryBounds bounds;
    solid.getBounds(bounds);
    EXPECT_EQ(bounds.getKind(), GeometryBounds::Kind::Unbounded);
}

TEST(PolynomialSurfaceContract, SexticUsesTheGeneralExpansion) {
    // |p|^6 - 1: the unit sphere written as an order 6 polynomial.
    PolynomialSurface solid(6, false);
    double* a = solid.getCoefficients();
    a[PolynomialSurface::getTermIndex(6, 6, 0, 0)] = 1;
    a[PolynomialSurface::getTermIndex(6, 0, 6, 0)] = 1;
    a[PolynomialSurface::getTermIndex(6, 0, 0, 6)] = 1;
    a[PolynomialSurface::getTermIndex(6, 4, 2, 0)] = 3;
    a[PolynomialSurface::getTermIndex(6, 4, 0, 2)] = 3;
    a[PolynomialSurface::getTermIndex(6, 2, 4, 0)] = 3;
    a[PolynomialSurface::getTermIndex(6, 0, 4, 2)] = 3;
    a[PolynomialSurface::getTermIndex(6, 2, 0, 4)] = 3;
    a[PolynomialSurface::getTermIndex(6, 0, 2, 4)] = 3;
    a[PolynomialSurface::getTermIndex(6, 2, 2, 2)] = 6;
    a[PolynomialSurface::getTermIndex(6, 0, 0, 0)] = -1;
    RaySolidClassification classification;
    ASSERT_EQ(solid.classifyRay(Ray(Vector3Dd(-3, 0.6, 0), Vector3Dd(1, 0, 0)),
        RayCrossingQuery(0, 10), classification), RaySolidQueryStatus::Supported);
    ASSERT_EQ(classification.boundaries.size(), 2);
    EXPECT_NEAR(classification.boundaries[0].t, 2.2, 1e-8);
    EXPECT_NEAR(classification.boundaries[1].t, 3.8, 1e-8);
    EXPECT_EQ(solid.classifyRay(Ray(Vector3Dd(-3, 0, 0), Vector3Dd(1, 0, 0)),
        RayCrossingQuery(java::Double::NEGATIVE_INFINITY, 10), classification),
        RaySolidQueryStatus::Unsupported);
}

TEST(BlobSolidContract, SingleComponentIsTheThresholdSphere) {
    // (1 - d^2/4)^2 > 0.5 inside d^2 < 4 (1 - sqrt(0.5)).
    const double radius = std::sqrt(4.0*(1.0 - std::sqrt(0.5)));
    for (int sturm = 0; sturm < 2; ++sturm) {
        BlobSolid blob(0.5, sturm != 0);
        blob.addComponent(BlobComponent(Vector3Dd(0, 0, 0), 2.0, 1.0));
        const Ray ray = Ray::fromAffine(Vector3Dd(-5, 0, 0), Vector3Dd(1, 0, 0));
        RaySolidClassification classification;
        ASSERT_EQ(blob.classifyRay(ray, RayCrossingQuery(0, 20), classification), RaySolidQueryStatus::Supported);
        ASSERT_EQ(classification.boundaries.size(), 2);
        EXPECT_NEAR(classification.boundaries[0].t, 5 - radius, 1e-9);
        EXPECT_NEAR(classification.boundaries[1].t, 5 + radius, 1e-9);
        EXPECT_FALSE(classification.insideSegments[0]);
        EXPECT_TRUE(classification.insideSegments[1]);
        RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
        ASSERT_TRUE(blob.doIntersectionFirstHit(ray, &hit));
        EXPECT_NEAR(hit.normal.x(), -1, 1e-12);
        EXPECT_EQ(blob.classifyPointCertified(Vector3Dd(0, 0, 0), 0.1), GeometryContainment::Inside);
        EXPECT_EQ(blob.classifyPointCertified(Vector3Dd(3, 0, 0), 0.5), GeometryContainment::Outside);
        EXPECT_EQ(blob.classifyPointCertified(Vector3Dd(0, radius, 0), 0.01), GeometryContainment::Boundary);
    }
}

TEST(BlobSolidContract, OverlappingComponentsMergeAndBoundsFollowStrengthSigns) {
    BlobSolid blob(0.6, false);
    blob.addComponent(BlobComponent(Vector3Dd(-0.8, 0, 0), 1.5, 1.0));
    blob.addComponent(BlobComponent(Vector3Dd(0.8, 0, 0), 1.5, 1.0));
    RayCrossingTestCollector crossings;
    ASSERT_EQ(blob.emitRayCrossings(Ray::fromAffine(Vector3Dd(-5, 0, 0), Vector3Dd(1, 0, 0)),
        RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), crossings), RayCrossingSupport::Supported);
    ASSERT_EQ(crossings.roots.size(), 2);
    EXPECT_EQ(crossings.kinds[0], RayCrossingKind::Entering);
    EXPECT_EQ(crossings.kinds[1], RayCrossingKind::Leaving);
    EXPECT_EQ(blob.classifyPoint(Vector3Dd(0, 0, 0), 0), GeometryContainment::Inside);
    GeometryBounds bounds;
    blob.getBounds(bounds);
    ASSERT_EQ(bounds.getKind(), GeometryBounds::Kind::Finite);
    EXPECT_LE(bounds.getMinimum().x(), -2.3);
    EXPECT_GE(bounds.getMaximum().x(), 2.3);
    EXPECT_LE(bounds.getMaximum().x(), 2.3 + 1e-12);
    blob.addComponent(BlobComponent(Vector3Dd(10, 0, 0), 1.0, -1.0));
    blob.getBounds(bounds);
    EXPECT_LE(bounds.getMaximum().x(), 2.3 + 1e-12);
    BlobSolid everywhere(-0.1, false);
    everywhere.addComponent(BlobComponent(Vector3Dd(0, 0, 0), 1.0, 1.0));
    everywhere.getBounds(bounds);
    EXPECT_EQ(bounds.getKind(), GeometryBounds::Kind::Unbounded);
    EXPECT_TRUE(BlobComponent(Vector3Dd(0, 0, 0), 1e-6, 1.0).isDegenerate());
}
