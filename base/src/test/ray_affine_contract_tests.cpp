#include <gtest/gtest.h>
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "RayCrossingTestCollector.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"

TEST(RayAffineContract, CopiesPreserveUnchangedComponents) {
    const Ray original = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0), 1);
    const Ray atExit = original.withT(2);
    EXPECT_DOUBLE_EQ(2, atExit.getDirection().x());
    EXPECT_DOUBLE_EQ(1, atExit.getOrigin().add(atExit.getDirection().multiply(atExit.getT())).x());
    EXPECT_DOUBLE_EQ(1, original.getT());
    const Ray shifted = original.withOrigin(Vector3Dd(-5,0,0));
    EXPECT_DOUBLE_EQ(2, shifted.getDirection().x());
    EXPECT_DOUBLE_EQ(1, shifted.getT());
    const Ray redirected = original.withDirection(Vector3Dd(4,1,0));
    EXPECT_DOUBLE_EQ(4, redirected.getDirection().x());
    EXPECT_DOUBLE_EQ(1, redirected.getDirection().y());
    EXPECT_DOUBLE_EQ(-3, redirected.getOrigin().x());
    EXPECT_DOUBLE_EQ(1, redirected.getT());
    const Ray copied = Ray::copyOf(original);
    EXPECT_TRUE(copied.equals(original));
    EXPECT_EQ(0, java::Double::compare(original.withT(-0.0).getT(), -0.0));
}

TEST(RayAffineContract, ExplicitAffineConstructionPreservesLegacyConstructor) {
    Ray legacy(Vector3Dd(), Vector3Dd(2,0,0));
    EXPECT_DOUBLE_EQ(1, legacy.getDirection().x());
    Ray affine = Ray::fromAffine(Vector3Dd(), Vector3Dd(2,0,0));
    EXPECT_DOUBLE_EQ(2, affine.getDirection().x());
    EXPECT_DOUBLE_EQ(0, affine.getT());
}

TEST(SphereFirstHitContract, FirstHitCrossingsAndDetailUseSameParameter) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(1, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(-1, hit.point.x());
    EXPECT_DOUBLE_EQ(-1, hit.normal.x());
    ASSERT_NE(nullptr, hit.getRay());
    EXPECT_DOUBLE_EQ(2, hit.getRay()->getDirection().x());
    RayCrossingTestCollector crossings;
    sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), crossings);
    ASSERT_EQ(2L, crossings.roots.size());
    EXPECT_DOUBLE_EQ(crossings.roots.get(0), hit.getHitDistance());
    Ray* pointerHit = sphere.doIntersectionFirstHit(ray);
    ASSERT_NE(nullptr, pointerHit);
    EXPECT_DOUBLE_EQ(hit.getHitDistance(), pointerHit->getT());
    EXPECT_DOUBLE_EQ(2, pointerHit->getDirection().x());
    delete pointerHit;
}

TEST(SphereFirstHitContract, InteriorExitAndClippedFarRoot) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(0.5, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_DOUBLE_EQ(1, hit.normal.x());
    ray = ray.withOrigin(Vector3Dd(-3,0,0));
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(1.25,2), &hit));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_FALSE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(1.25,1.75), &hit));
    EXPECT_TRUE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(2,2), nullptr));
    EXPECT_FALSE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(3,2), &hit));
}

TEST(SphereFirstHitContract, TangencyAndDistanceOnly) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(-2,1,0), Vector3Dd(1,0,0));
    RayHit hit(RayHit::DETAIL_NONE, false);
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_EQ(nullptr, hit.getRay());
    ray.setDirection(Vector3Dd());
    EXPECT_FALSE(sphere.doIntersectionFirstHit(ray, &hit));
    Sphere oblique(java::Math::sqrt(5.0));
    ray = Ray::fromAffine(Vector3Dd(-6,-3,0), Vector3Dd(2,1,0));
    ASSERT_TRUE(oblique.doIntersectionFirstHit(ray, &hit));
    EXPECT_NEAR(2, hit.getHitDistance(), 1e-12);
}

TEST(SphereFirstHitContract, SceneFastPathAndDetailAgreeForAffineRay) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    Ray ray = Ray::fromAffine(Vector3Dd(7,0,0), Vector3Dd(2,0,0));
    RayHit distance(RayHit::DETAIL_NONE, false);
    RayHit detail(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &distance));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &detail));
    EXPECT_DOUBLE_EQ(1, distance.getHitDistance());
    EXPECT_DOUBLE_EQ(distance.getHitDistance(), detail.getHitDistance());
    EXPECT_DOUBLE_EQ(9, detail.point.x());
    EXPECT_DOUBLE_EQ(-1, detail.normal.x());
    ASSERT_NE(nullptr, detail.getRay());
    EXPECT_DOUBLE_EQ(2, detail.getRay()->getDirection().x());
    ray = ray.withOrigin(Vector3Dd(10,0,0));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &distance));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &detail));
    EXPECT_DOUBLE_EQ(0.5, distance.getHitDistance());
    EXPECT_DOUBLE_EQ(distance.getHitDistance(), detail.getHitDistance());
    EXPECT_DOUBLE_EQ(11, detail.point.x());
}

TEST(SphereFirstHitContract, NegativeNonuniformScalePreservesWorldParameter) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    Ray ray = Ray::fromAffine(Vector3Dd(4,0,0), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(8, hit.point.x());
    EXPECT_DOUBLE_EQ(-1, hit.normal.x());
    ASSERT_NE(nullptr, hit.getRay());
    EXPECT_DOUBLE_EQ(2, hit.getRay()->getDirection().x());
    ray = ray.withOrigin(Vector3Dd(10,0,0));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(1, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(12, hit.point.x());
    EXPECT_DOUBLE_EQ(1, hit.normal.x());
}

TEST(SphereFirstHitContract, InwardShadowBiasFindsTheAnalyticExit) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Vector3Dd surfacePoint(1,0,0);
    const Vector3Dd lightDirection = Vector3Dd(-0.1,1,0).normalized();
    const double bias = 1e-6;
    Ray shadowRay = Ray::fromAffine(surfacePoint.add(lightDirection.multiply(bias)),
        lightDirection);
    EXPECT_EQ(Geometry::INSIDE, sphere.doContainmentTest(shadowRay.getOrigin(), 0));
    RayHit hit(RayHit::DETAIL_NONE, false);
    ASSERT_TRUE(body.doIntersectionFirstHit(shadowRay, &hit));
    // On the unit sphere, the other chord endpoint lies at -2*(p dot d).
    const double expectedExit = -2 * surfacePoint.dotProduct(lightDirection) - bias;
    EXPECT_NEAR(expectedExit, hit.getHitDistance(), 1e-12);
    EXPECT_GT(hit.getHitDistance(), bias);
}


TEST(SphereDetailContract, RequestedAndAvailableAreDistinctAndResettable) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(0,0,2), Vector3Dd(0,0,-1));
    RayHit hit(RayHit::DETAIL_ALL);
    EXPECT_FALSE(hit.isDetailAvailabilityKnown());
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, hit.getAvailableDetailMask());
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_ALL), hit.getRequiredDetailMask());
    RayHit copied(hit);
    EXPECT_TRUE(copied.isDetailAvailabilityKnown());
    EXPECT_EQ(hit.getAvailableDetailMask(), copied.getAvailableDetailMask());
    hit.resetForDistanceOnly();
    EXPECT_FALSE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_NONE), hit.getAvailableDetailMask());
    EXPECT_TRUE(copied.isDetailAvailabilityKnown());
}

#include "vsdk/toolkit/environment/geometry/element/GeometryQueryWorkspace.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

TEST(GeometryQueryContract, DistinguishesUnsupportedMissAndInvalidInput) {
    Sphere sphere(1);
    const Geometry* geometry = &sphere;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    GeometryQueryWorkspace workspace;
    RayHit& hit = workspace.hitResult();
    hit.reset(RayHit::DETAIL_POINT);
    RayBoundary firstBoundary;
    EXPECT_EQ(RayFirstHitStatus::Hit, geometry->queryFirstHit(ray, RayCrossingQuery(1.25,2), &hit, &firstBoundary));
    EXPECT_EQ(&sphere, firstBoundary.geometry);
    EXPECT_DOUBLE_EQ(2, firstBoundary.t);
    EXPECT_FALSE(firstBoundary.normalReversed);
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_POINT), hit.getAvailableDetailMask());
    EXPECT_EQ(RayFirstHitStatus::Miss, geometry->queryFirstHit(ray, RayCrossingQuery(1.25,1.75), &hit, &firstBoundary));
    EXPECT_EQ(nullptr, firstBoundary.geometry);
    EXPECT_FALSE(hit.hasHitDistance());
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, geometry->queryFirstHit(ray, RayCrossingQuery(2,1), &hit));
    ray.setDirection(Vector3Dd(0,0,0));
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, geometry->queryFirstHit(ray, RayCrossingQuery(0,3), &hit));
    ray.setDirection(Vector3Dd(1e300,0,0));
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, geometry->queryFirstHit(ray, RayCrossingQuery(0,3), &hit));
    Box box(1,1,1);
    EXPECT_EQ(RayFirstHitStatus::Unsupported, box.queryFirstHit(ray, RayCrossingQuery(0,3), &hit));
}

TEST(GeometryQueryContract, WorkspaceAndExtensibleProvenanceAreIndependent) {
    Sphere sphere(1);
    SimpleBody instance;
    instance.setGeometryReference(&sphere);
    GeometryQueryWorkspace workspace;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        sphere.classifyRay(ray, RayCrossingQuery(0,3), workspace.solidResult()));
    ASSERT_EQ(2L, workspace.solidResult().boundaries.size());
    RaySolidBoundary& boundary = workspace.solidResult().boundaries[0];
    EXPECT_EQ(&sphere, boundary.geometry);
    EXPECT_EQ(-1, boundary.getProvenance().getSurfaceIndex());
    ASSERT_TRUE(boundary.getProvenance().setSurfaceIndex(7));
    EXPECT_FALSE(boundary.getProvenance().setSurfaceIndex(-2));
    boundary.normalReversed = true;
    Matrix4x4d forward = Matrix4x4d().translation(1,2,3);
    Matrix4x4d inverse = Matrix4x4d().translation(-1,-2,-3);
    RayInstanceStep step(&instance, forward, inverse);
    for (int i = 0; i < 32; i++) ASSERT_TRUE(boundary.appendInstance(step));
    EXPECT_FALSE(boundary.appendInstance(RayInstanceStep()));
    EXPECT_EQ(32L, boundary.getInstanceCount());
    RaySolidBoundary copied(boundary);
    boundary.clearInstances();
    EXPECT_EQ(7, boundary.getProvenance().getSurfaceIndex());
    EXPECT_EQ(32L, copied.getInstanceCount());
    EXPECT_EQ(7, copied.getProvenance().getSurfaceIndex());
    EXPECT_TRUE(copied.normalReversed);
    RayInstanceStep recovered;
    ASSERT_TRUE(copied.getInstance(31, recovered));
    EXPECT_EQ(&instance, recovered.instance);
    EXPECT_DOUBLE_EQ(1, recovered.childToParent.multiply(Vector3Dd()).x());
    EXPECT_DOUBLE_EQ(0, recovered.parentToChild.multiply(recovered.childToParent.multiply(Vector3Dd())).x());
    EXPECT_FALSE(copied.getInstance(32, recovered));
    workspace.clear();
    EXPECT_TRUE(workspace.solidResult().boundaries.isEmpty());
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, workspace.boundsResult().getKind());
    EXPECT_FALSE(workspace.hitResult().hasHitDistance());
    EXPECT_EQ(32L, copied.getInstanceCount());
}


TEST(GeometryQueryContract, SceneInstancePreservesAffineParameterAndProvenance) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    Ray ray = Ray::fromAffine(Vector3Dd(4,0,0), Vector3Dd(2,0,0));
    GeometryQueryWorkspace workspace;
    ASSERT_EQ(RaySolidQueryStatus::Supported, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    ASSERT_EQ(2L, workspace.solidResult().boundaries.size());
    EXPECT_DOUBLE_EQ(2, workspace.solidResult().boundaries[0].t);
    EXPECT_DOUBLE_EQ(4, workspace.solidResult().boundaries[1].t);
    EXPECT_EQ(&sphere, workspace.solidResult().boundaries[0].geometry);
    RayInstanceStep step;
    ASSERT_TRUE(workspace.solidResult().boundaries[0].getInstance(0, step));
    EXPECT_EQ(&body, step.instance);
    const Vector3Dd worldPoint = ray.getOrigin().add(ray.getDirection().multiply(2));
    const Vector3Dd localPoint = step.parentToChild.multiply(worldPoint);
    EXPECT_DOUBLE_EQ(1, localPoint.x());
    EXPECT_DOUBLE_EQ(worldPoint.x(), step.childToParent.multiply(localPoint).x());
    body.setScale(Vector3Dd(0,3,1));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    EXPECT_TRUE(workspace.solidResult().boundaries.isEmpty());
}


TEST(GeometryQueryContract, PointContainmentSeparatesBoundaryAndUnsupported) {
    Sphere sphere(1);
    const Geometry* geometry = &sphere;
    EXPECT_EQ(GeometryContainment::Inside, geometry->classifyPoint(Vector3Dd(0,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, geometry->classifyPoint(Vector3Dd(1,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, geometry->classifyPoint(Vector3Dd(1.01,0,0), 0.02));
    EXPECT_EQ(GeometryContainment::Outside, geometry->classifyPoint(Vector3Dd(2,0,0), 0));
    EXPECT_EQ(GeometryContainment::InvalidInput, geometry->classifyPoint(Vector3Dd(), -1));
    EXPECT_EQ(GeometryContainment::InvalidInput, geometry->classifyPoint(Vector3Dd(), java::Double::NaN));
    EXPECT_EQ(GeometryContainment::Indeterminate, geometry->classifyPoint(Vector3Dd(1e300,0,0), 0));
    Sphere tiny(1e-300);
    EXPECT_EQ(GeometryContainment::Indeterminate, tiny.classifyPoint(Vector3Dd(2e-300,0,0), 0));
    Box box(1,1,1);
    EXPECT_EQ(GeometryContainment::Unsupported, box.classifyPoint(Vector3Dd(), 0));
}

TEST(GeometryQueryContract, CrossingFailuresAreNotReportedAsMisses) {
    Sphere sphere(1);
    RayCrossingTestCollector crossings;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(0,0,0));
    EXPECT_EQ(RayCrossingSupport::InvalidInput, sphere.emitRayCrossings(ray, RayCrossingQuery(0,3), crossings));
    ray.setDirection(Vector3Dd(1e300,0,0));
    EXPECT_EQ(RayCrossingSupport::Indeterminate, sphere.emitRayCrossings(ray, RayCrossingQuery(0,3), crossings));
    EXPECT_TRUE(crossings.roots.isEmpty());
}

TEST(GeometryQueryContract, ShearRoundTripPreservesTheRayParameter) {
    Sphere sphere(1);
    const Matrix4x4d shear = Matrix4x4d::identityMatrix().withVal(0,1,0.5);
    const Matrix4x4d inverse = Matrix4x4d::identityMatrix().withVal(0,1,-0.5);
    const Ray local = Ray::fromAffine(Vector3Dd(-3,0.5,0), Vector3Dd(2,0,0));
    const Ray world = Ray::fromAffine(shear.multiply(local.getOrigin()),
        shear.withoutTranslation().multiply(local.getDirection()));
    const Ray recovered = Ray::fromAffine(inverse.multiply(world.getOrigin()),
        inverse.withoutTranslation().multiply(world.getDirection()));
    RayHit localHit(RayHit::DETAIL_POINT, false);
    RayHit recoveredHit(RayHit::DETAIL_POINT, false);
    ASSERT_EQ(RayFirstHitStatus::Hit, sphere.queryFirstHit(local, RayCrossingQuery(0,4), &localHit));
    ASSERT_EQ(RayFirstHitStatus::Hit, sphere.queryFirstHit(recovered, RayCrossingQuery(0,4), &recoveredHit));
    EXPECT_DOUBLE_EQ(localHit.getHitDistance(), recoveredHit.getHitDistance());
    const Vector3Dd expected = shear.multiply(localHit.point);
    const Vector3Dd worldAtT = world.getOrigin().add(world.getDirection().multiply(recoveredHit.getHitDistance()));
    EXPECT_DOUBLE_EQ(expected.x(), worldAtT.x());
    EXPECT_DOUBLE_EQ(expected.y(), worldAtT.y());
}


TEST(GeometryQueryContract, SurfaceEventsUseTheSameExtensibleProvenanceModel) {
    Sphere sphere(1);
    SimpleBody body;
    RayCrossing event(1, &sphere, RayCrossingKind::Entering);
    RayInstanceStep step(&body, Matrix4x4d(), Matrix4x4d());
    ASSERT_TRUE(event.getProvenance().setSurfaceIndex(3));
    ASSERT_TRUE(event.getProvenance().appendInstance(step));
    event.normalReversed = true;
    RayCrossing copy(event);
    EXPECT_TRUE(copy.normalReversed);
    event.getProvenance().clear();
    EXPECT_EQ(-1, event.getProvenance().getSurfaceIndex());
    EXPECT_EQ(3, copy.getProvenance().getSurfaceIndex());
    EXPECT_EQ(1L, copy.getProvenance().getInstanceCount());
    RayInstanceStep restored;
    ASSERT_TRUE(copy.getProvenance().getInstance(0, restored));
    EXPECT_EQ(&body, restored.instance);
}


#include "java/lang/Thread.h"
#include "GeometryWorkspaceTestWorker.h"
TEST(GeometryQueryContract, SeparateWorkerWorkspacesCanRunConcurrently) {
    Sphere sphere(1);
    GeometryWorkspaceTestWorker first(&sphere), second(&sphere);
    java::Thread firstThread(&first), secondThread(&second);
    firstThread.start(); secondThread.start();
    firstThread.join(); secondThread.join();
    EXPECT_TRUE(first.passed());
    EXPECT_TRUE(second.passed());
    EXPECT_NE(nullptr, first.boundaryStorage());
    EXPECT_NE(nullptr, second.boundaryStorage());
    EXPECT_NE(first.boundaryStorage(), second.boundaryStorage());
}


TEST(GeometryQueryContract, NestedWorkspaceScratchIsStableAndRequiresLifoRelease) {
    GeometryQueryWorkspace workspace;
    java::ArrayList<RaySolidClassification*> acquired;
    for (int i = 0; i < 32; i++) {
        RaySolidClassification* slot = workspace.acquireSolidScratch();
        ASSERT_NE(nullptr, slot);
        slot->insideSegments.add(true);
        acquired.add(slot);
    }
    EXPECT_FALSE(workspace.releaseSolidScratch(acquired[0]));
    EXPECT_TRUE(acquired[0]->insideSegments.get(0));
    for (int i = 31; i >= 0; i--) ASSERT_TRUE(workspace.releaseSolidScratch(acquired[i]));
    RaySolidClassification* reused = workspace.acquireSolidScratch();
    EXPECT_EQ(acquired[0], reused);
    EXPECT_TRUE(reused->insideSegments.isEmpty());
    workspace.clear();
    EXPECT_FALSE(workspace.releaseSolidScratch(reused));
    EXPECT_EQ(reused, workspace.acquireSolidScratch());
}


TEST(GeometryQueryContract, SceneContainmentHasExplicitWorldTolerancePolicy) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    EXPECT_EQ(GeometryContainment::Inside, body.classifyPoint(Vector3Dd(10,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(8,0,0), 0));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(8.05,0,0), 0.1));
    body.setScale(Vector3Dd(-2,-2,-2));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(7.95,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), -1));
    body.setScale(Vector3Dd(0,2,2));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), 0));
    body.setScale(Vector3Dd(1e-12,2,2));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    GeometryQueryWorkspace workspace;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setScale(Vector3Dd(java::Double::POSITIVE_INFINITY,2,2));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setScale(Vector3Dd(1,1,1));
    body.setRotation(Matrix4x4d::identityMatrix().withVal(0,0,0));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setRotation(Matrix4x4d::identityMatrix().withVal(0,1,0.5));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setRotation(Matrix4x4d());
    body.setPosition(Vector3Dd(java::Double::POSITIVE_INFINITY,0,0));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setPosition(Vector3Dd(1e308,0,0));
    body.setScale(Vector3Dd(1e-5,1,1));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
}


TEST(GeometryQueryContract, OpenSurfaceDoesNotAcquireFictitiousSolidInterior) {
    InfinitePlane plane(1,0,0,0);
    const Ray ray = Ray::fromAffine(Vector3Dd(-2,0,0), Vector3Dd(1,0,0));
    RayHit legacy(RayHit::DETAIL_NONE, false);
    EXPECT_TRUE(plane.doIntersectionFirstHit(ray, &legacy));
    GeometryQueryWorkspace workspace;
    EXPECT_EQ(RayFirstHitStatus::Unsupported,
        plane.queryFirstHit(ray, RayCrossingQuery(0,4), &workspace.hitResult()));
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,
        plane.classifyRay(ray, RayCrossingQuery(0,4), workspace.solidResult()));
    EXPECT_TRUE(workspace.solidResult().insideSegments.isEmpty());
}


TEST(GeometryQueryContract, SmallSphereNormalIsUnitAndZeroNormalUnavailable) {
    Sphere sphere(1e-4);
    Ray ray = Ray::fromAffine(Vector3Dd(0,0,3e-4), Vector3Dd(0,0,-1));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_EQ(RayFirstHitStatus::Hit, sphere.queryFirstHit(ray, RayCrossingQuery(0,1), &hit));
    EXPECT_NEAR(1, hit.normal.z(), 1e-12);
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL), hit.getAvailableDetailMask());
    Sphere unit(1);
    Ray center = Ray::fromAffine(Vector3Dd(), Vector3Dd(1,0,0));
    hit.reset(RayHit::DETAIL_NORMAL);
    unit.doExtraInformation(center, 0, &hit);
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(0, hit.getAvailableDetailMask() & RayHit::DETAIL_NORMAL);
}
