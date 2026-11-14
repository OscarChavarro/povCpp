#include <gtest/gtest.h>
#include <cmath>
#include <cstdio>
#include <string>
#include "java/lang/Double.h"
#include "java/lang/Exception.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/ReaderAltimetry.h"
#include "vsdk/toolkit/io/geometry/ReaderMitScene.h"
#include "HeightFieldTestFactory.h"

TEST(HeightFieldSolidContract, PlaneFieldHasExactCrossingsNormalAndContainment) {
    const HeightFieldSolid solid = HeightFieldTestFactory::createPlane();
    ASSERT_TRUE(solid.isValid());
    const Ray ray = Ray::fromAffine(Vector3Dd(1.3, 10, 2.6), Vector3Dd(0, -1, 0));
    RaySolidClassification classification;
    ASSERT_EQ(solid.classifyRay(ray, RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), classification),
        RaySolidQueryStatus::Supported);
    ASSERT_EQ(classification.boundaries.size(), 2);
    EXPECT_NEAR(classification.boundaries[0].t, 10 - 2.3, 1e-12);
    EXPECT_EQ(classification.boundaries[0].surfaceIndex, 0);
    EXPECT_NEAR(classification.boundaries[1].t, 10, 1e-12);
    EXPECT_EQ(classification.boundaries[1].surfaceIndex, 3);
    ASSERT_EQ(classification.insideSegments.size(), 3);
    EXPECT_FALSE(classification.insideSegments[0]);
    EXPECT_TRUE(classification.insideSegments[1]);
    EXPECT_FALSE(classification.insideSegments[2]);

    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    ASSERT_EQ(solid.queryFirstHit(ray, RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), &hit),
        RayFirstHitStatus::Hit);
    const double length = std::sqrt(0.25 + 1 + 0.0625);
    EXPECT_NEAR(hit.normal.x(), -0.5/length, 1e-12);
    EXPECT_NEAR(hit.normal.y(), 1/length, 1e-12);
    EXPECT_NEAR(hit.normal.z(), -0.25/length, 1e-12);

    // The PovRay traversal agrees on a slanted ray.
    const Ray slanted = Ray::fromAffine(Vector3Dd(1.3, 10, 2.6), Vector3Dd(0.1, -1, -0.05));
    ASSERT_EQ(solid.classifyRay(slanted, RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), classification),
        RaySolidQueryStatus::Supported);
    ASSERT_FALSE(classification.boundaries.isEmpty());
    double depth = 0;
    ASSERT_EQ(solid.traceFirstHit(Vector3Dd(1.3, 10, 2.6), Vector3Dd(0.1, -1, -0.05), depth),
        HeightFieldTraceResult::Hit);
    EXPECT_NEAR(depth, classification.boundaries[0].t, 1e-9);
    EXPECT_EQ(solid.traceFirstHit(Vector3Dd(1.3, 10, 2.6), Vector3Dd(0, 1, 0), depth),
        HeightFieldTraceResult::MissedBounds);

    EXPECT_EQ(solid.classifyPoint(Vector3Dd(1.3, 1, 2.6), 0), GeometryContainment::Inside);
    EXPECT_EQ(solid.classifyPoint(Vector3Dd(1.3, 3, 2.6), 0), GeometryContainment::Outside);
    EXPECT_EQ(solid.classifyPointCertified(Vector3Dd(1.3, 1, 2.6), 1e-6), GeometryContainment::Inside);
    EXPECT_EQ(solid.classifyPointCertified(Vector3Dd(1.3, 3.9, 2.6), 1e-6), GeometryContainment::Outside);
    EXPECT_EQ(solid.classifyPointCertified(Vector3Dd(1.3, 2.3, 2.6), 1e-6), GeometryContainment::Boundary);
    EXPECT_EQ(solid.classifyPoint(Vector3Dd(3.5, 1, 2), 0), GeometryContainment::Outside);

    GeometryBounds bounds;
    solid.getBounds(bounds);
    ASSERT_EQ(bounds.getKind(), GeometryBounds::Kind::Finite);
    EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 1 + 1.5 + 0.75);
}

TEST(HeightFieldSolidContract, TraversalAndExactClassificationAgreeOnHills) {
    const HeightFieldSolid solid = HeightFieldTestFactory::createHills();
    long compared = 0;
    for (int i = 0; i < 400; ++i) {
        const Vector3Dd origin(-3 + 0.055*i, 9, 1.013 + 0.037*i);
        const Vector3Dd direction(std::cos(0.3*i), -1.1 - 0.4*std::sin(0.7*i), std::sin(0.3*i));
        const Ray ray = Ray::fromAffine(origin, direction);
        RaySolidClassification classification;
        ASSERT_EQ(solid.classifyRay(ray, RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), classification),
            RaySolidQueryStatus::Supported) << i;
        // Membership between boundaries matches point containment.
        for (long k = 0; k + 1 < classification.boundaries.size(); ++k) {
            const double t = 0.5*(classification.boundaries[k].t + classification.boundaries[k + 1].t);
            const GeometryContainment c = solid.classifyPoint(origin.add(direction.multiply(t)), 0);
            EXPECT_EQ(c == GeometryContainment::Inside, (bool)classification.insideSegments[k + 1]) << i;
        }
        double depth = 0;
        if (solid.traceFirstHit(origin, direction, depth) != HeightFieldTraceResult::Hit) continue;
        // The PovRay traversal may report hits on cells just outside the box.
        const Vector3Dd p = origin.add(direction.multiply(depth));
        if (p.x() < 1 || p.x() > 15 || p.z() < 1 || p.z() > 15) continue;
        ASSERT_FALSE(classification.boundaries.isEmpty()) << i;
        const RayBoundary& first = classification.boundaries[0];
        if (first.surfaceIndex != 0) continue;
        EXPECT_NEAR(depth, first.t, 1e-6*(1 + first.t)) << i;
        ++compared;
    }
    EXPECT_GT(compared, 150);
}

TEST(HeightFieldSolidContract, CertifiedBoxesAndInvalidGrids) {
    const HeightFieldSolid solid = HeightFieldTestFactory::createPlane();
    EXPECT_EQ(solid.classifyBoxCertified(Vector3Dd(2, 0.5, 2), Vector3Dd(0.4, 0.2, 0.4)), GeometryContainment::Inside);
    EXPECT_EQ(solid.classifyBoxCertified(Vector3Dd(2, 4.6, 2), Vector3Dd(0.4, 0.2, 0.4)), GeometryContainment::Outside);
    EXPECT_EQ(solid.classifyBoxCertified(Vector3Dd(2, 2.5, 2), Vector3Dd(0.4, 0.5, 0.4)), GeometryContainment::Indeterminate);
    HeightFieldSolid outside(3, 3, Vector3Dd(0, 0, 0), Vector3Dd(4, 1, 2));
    EXPECT_FALSE(outside.isValid());
    RaySolidClassification classification;
    EXPECT_EQ(outside.classifyRay(Ray::fromAffine(Vector3Dd(1, 5, 1), Vector3Dd(0, -1, 0)),
        RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), classification), RaySolidQueryStatus::Indeterminate);
}

TEST(ReaderAltimetryFormat, ReadsGridsAndRejectsMalformedFiles) {
    const std::string path = testing::TempDir() + "reader_altimetry.alt";
    FILE* file = fopen(path.c_str(), "w");
    ASSERT_NE(nullptr, file);
    fputs("# comment\naltimetry 3 2 # columns rows\n0 1 2\n3 4 5\n", file);
    fclose(file);
    HeightFieldSolid* solid = ReaderAltimetry::importHeightField(path.c_str());
    ASSERT_NE(nullptr, solid);
    EXPECT_EQ(solid->getColumnCount(), 3);
    EXPECT_EQ(solid->getRowCount(), 2);
    EXPECT_DOUBLE_EQ(solid->getHeight(2, 1), 5);
    EXPECT_DOUBLE_EQ(solid->getMinimum().y(), 0);
    EXPECT_DOUBLE_EQ(solid->getMaximum().y(), 5);
    EXPECT_DOUBLE_EQ(solid->getMaximum().x(), 2);
    EXPECT_TRUE(solid->hasCurrentBlocks());
    delete solid;
    const char* malformed[5] = {"altimetry 3 2\n0 1 2\n3 4\n", "altimetry 3 2\n0 1 2\n3 4 5 6\n",
        "altimetry 1 2\n0\n1\n", "heights 2 2\n0 0 0 0\n", "altimetry 2 2\n0 x 0 0\n"};
    for (int i = 0; i < 5; ++i) {
        file = fopen(path.c_str(), "w");
        ASSERT_NE(nullptr, file);
        fputs(malformed[i], file); fclose(file);
        EXPECT_THROW(ReaderAltimetry::importHeightField(path.c_str()), VSDKFatalException) << i;
    }
    std::remove(path.c_str());
}

TEST(ReaderMitPrimitives, HeightfieldSceneReadsAltimetryRelativeToTheScene) {
    SimpleScene scene;
    ReaderMitScene reader;
    reader.importEnvironment(VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/heightfield_terrain.ray", &scene);
    java::ArrayList<SimpleBody*>& bodies = scene.getSimpleBodies();
    ASSERT_EQ(2, bodies.size());
    const HeightFieldSolid* terrain = dynamic_cast<const HeightFieldSolid*>(bodies[0]->getGeometry());
    ASSERT_NE(nullptr, terrain);
    EXPECT_EQ(65, terrain->getColumnCount());
    EXPECT_DOUBLE_EQ(0.6, terrain->getMinimum().y());
    // Straight down onto the grid centre (local 32, 32) lands on its height.
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    ASSERT_TRUE(bodies[0]->doIntersectionFirstHit(
        Ray::fromAffine(Vector3Dd(0, 0, 10), Vector3Dd(0, 0, -1)), &hit));
    EXPECT_NEAR(10 - 0.15*terrain->getHeight(32, 32), hit.getHitDistance(), 1e-9);
    EXPECT_GT(hit.normal.z(), 0);
}

TEST(ReaderMitPrimitives, MalformedHeightfieldLinesAreRejected) {
    const std::string altimetry = testing::TempDir() + "reader_mit_hf.alt";
    FILE* file = fopen(altimetry.c_str(), "w");
    ASSERT_NE(nullptr, file);
    fputs("altimetry 2 2\n0 1\n1 2\n", file); fclose(file);
    const std::string camera = "eye 0 5 5\nlookat 0 0 0\nup 0 1 0\n";
    const std::string bodies[4] = {"heightfield\n", "heightfield missing.alt\n",
        "heightfield reader_mit_hf.alt water 3\n", "heightfield reader_mit_hf.alt level 1\n"};
    const std::string path = testing::TempDir() + "reader_mit_hf.ray";
    for (int i = 0; i < 4; ++i) {
        file = fopen(path.c_str(), "w");
        ASSERT_NE(nullptr, file);
        fputs((camera + bodies[i]).c_str(), file); fclose(file);
        SimpleScene scene;
        ReaderMitScene reader;
        EXPECT_THROW(reader.importEnvironment(path.c_str(), &scene), java::Exception) << i;
    }
    file = fopen(path.c_str(), "w");
    fputs((camera + "heightfield reader_mit_hf.alt water 0.5\n").c_str(), file); fclose(file);
    SimpleScene scene;
    ReaderMitScene reader;
    reader.importEnvironment(path.c_str(), &scene);
    ASSERT_EQ(1, scene.getSimpleBodies().size());
    std::remove(path.c_str());
    std::remove(altimetry.c_str());
}
