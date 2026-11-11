#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/ReaderMitScene.h"

TEST(ReaderMitTriangles, CowImportsFacesMaterialsAndHasRealHits) {
    SimpleScene scene;
    ReaderMitScene reader;
    reader.importEnvironment(VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/cow.ray", &scene);
    java::ArrayList<SimpleBody*>& bodies = scene.getSimpleBodies();
    ASSERT_EQ(6, bodies.size());
    int total = 0;
    for (long int i = 0; i < bodies.size(); ++i) {
        TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(bodies[i]->getGeometry());
        ASSERT_NE(nullptr, mesh);
        ASSERT_NE(nullptr, bodies[i]->getMaterial());
        EXPECT_GT(mesh->getNumTriangles(), 0);
        total += mesh->getNumTriangles();
    }
    EXPECT_EQ(3640, total);
    TriangleMesh* ground = dynamic_cast<TriangleMesh*>(bodies[0]->getGeometry());
    ASSERT_NE(nullptr, ground);
    EXPECT_EQ(2, ground->getNumTriangles());
    GeometryBounds bounds;
    ground->getBounds(bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    EXPECT_DOUBLE_EQ(-10, bounds.getMinimum().x());
    EXPECT_DOUBLE_EQ(-4, bounds.getMinimum().y());
    EXPECT_DOUBLE_EQ(10, bounds.getMaximum().z());
    RayHit groundHit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    ASSERT_TRUE(ground->doIntersectionFirstHit(
        Ray::fromAffine(Vector3Dd(0,0,0), Vector3Dd(0,-1,0)), &groundHit));
    EXPECT_DOUBLE_EQ(-4, groundHit.point.y());

    TriangleMesh* cow = dynamic_cast<TriangleMesh*>(bodies[1]->getGeometry());
    ASSERT_NE(nullptr, cow);
    java::ArrayList<int>& indices = cow->getTriangleIndexes();
    java::ArrayList<double>& positions = cow->getVertexPositions();
    bool found = false;
    for (int i = 0; i < cow->getNumTriangles() && !found; ++i) {
        const int a = indices[i*3], b = indices[i*3+1], c = indices[i*3+2];
        const Vector3Dd p(positions[a*3], positions[a*3+1], positions[a*3+2]);
        const Vector3Dd q(positions[b*3], positions[b*3+1], positions[b*3+2]);
        const Vector3Dd r(positions[c*3], positions[c*3+1], positions[c*3+2]);
        const Vector3Dd cross = q.subtract(p).crossProduct(r.subtract(p));
        if (cross.dotProduct(cross) < 1e-8) continue;
        const Vector3Dd normal = cross.normalized();
        const Vector3Dd center = p.add(q).add(r).multiply(1.0/3.0);
        const Ray sample = Ray::fromAffine(center.add(normal), normal.multiply(-1));
        RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
        found = cow->doIntersectionFirstHit(sample, &hit);
        if (found) {
            RayHit typed(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
            RayBoundary boundary;
            EXPECT_EQ(RayFirstHitStatus::Hit,
                cow->queryFirstHit(sample, RayCrossingQuery(0,10), &typed, &boundary));
            EXPECT_EQ(cow, boundary.geometry);
            EXPECT_GE(boundary.surfaceIndex, 0);
        }
    }
    EXPECT_TRUE(found);
}

TEST(ReaderMitSolids, BoxAndHalfSpaceProduceActualSolidIntervals) {
    SimpleScene scene;
    ReaderMitScene reader;
    reader.importEnvironment(VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/box_offsets.ray", &scene);
    java::ArrayList<SimpleBody*>& bodies = scene.getSimpleBodies();
    ASSERT_EQ(3,bodies.size());
    ASSERT_NE(nullptr,dynamic_cast<PlanarHalfSpace*>(bodies[0]->getGeometry()));
    ASSERT_NE(nullptr,dynamic_cast<Box*>(bodies[1]->getGeometry()));
    RaySolidClassification result;
    const Ray boxRay=Ray::fromAffine(Vector3Dd(-2,0,0.5),Vector3Dd(1,0,0));
    ASSERT_EQ(RaySolidQueryStatus::Supported,bodies[1]->classifyRay(boxRay,RayCrossingQuery(0,10),result));
    ASSERT_EQ(2,result.boundaries.size());
    EXPECT_NEAR(0.8,result.boundaries[0].t,1e-12); EXPECT_NEAR(2.1,result.boundaries[1].t,1e-12);
    const Ray groundRay=Ray::fromAffine(Vector3Dd(0,0,2),Vector3Dd(0,0,-1));
    ASSERT_EQ(RaySolidQueryStatus::Supported,bodies[0]->classifyRay(groundRay,RayCrossingQuery(0,10),result));
    ASSERT_EQ(1,result.boundaries.size()); ASSERT_EQ(2,result.insideSegments.size());
    EXPECT_DOUBLE_EQ(2,result.boundaries[0].t); EXPECT_FALSE(result.insideSegments[0]); EXPECT_TRUE(result.insideSegments[1]);
}
