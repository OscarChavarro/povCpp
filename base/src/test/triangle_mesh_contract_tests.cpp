#include <gtest/gtest.h>
#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "RayCrossingTestCollector.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"

TEST(TriangleMeshContract, OrderedCrossingsCarryFaceAndFirstHitDetails) {
    TriangleMesh mesh;
    mesh.initVertexPositionsArray(6);
    java::ArrayList<double>& positions = mesh.getVertexPositions();
    const double values[] = {0,0,0, 1,0,0, 0,1,0, 0,0,2, 1,0,2, 0,1,2};
    for (int i = 0; i < 18; ++i) positions[i] = values[i];
    mesh.initTriangleArrays(2);
    mesh.setTriangleAt(0, Triangle(0,1,2));
    mesh.setTriangleAt(1, Triangle(3,4,5));
    mesh.initVertexUvsArray();
    java::ArrayList<double>& uv = mesh.getVertexUvs();
    uv[6]=0; uv[7]=0; uv[8]=1; uv[9]=0; uv[10]=0; uv[11]=1;
    const Ray ray = Ray::fromAffine(Vector3Dd(0.25,0.25,4), Vector3Dd(0,0,-1));
    const RayCrossingQuery query(2,4);
    RayCrossingTestCollector events;
    ASSERT_EQ(RayCrossingSupport::Supported, mesh.emitRayCrossings(ray, query, events));
    ASSERT_EQ(2, events.roots.size());
    EXPECT_DOUBLE_EQ(2, events.roots[0]);
    EXPECT_DOUBLE_EQ(4, events.roots[1]);
    EXPECT_EQ(1, events.surfaceIndices[0]);
    EXPECT_EQ(0, events.surfaceIndices[1]);
    EXPECT_EQ(&mesh, events.geometries[0]);
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL | RayHit::DETAIL_UV, false);
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit, mesh.queryFirstHit(ray, query, &hit, &boundary));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(2, hit.point.z());
    EXPECT_DOUBLE_EQ(1, hit.normal.z());
    EXPECT_DOUBLE_EQ(0.25, hit.u);
    EXPECT_DOUBLE_EQ(0.25, hit.v);
    EXPECT_EQ(1, boundary.surfaceIndex);
    EXPECT_EQ(&mesh, boundary.geometry);
    EXPECT_EQ(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL | RayHit::DETAIL_UV,
        hit.getAvailableDetailMask());
    RayBoundary copied(boundary);
    EXPECT_EQ(1, copied.surfaceIndex);
    RayBoundary assigned;
    assigned = copied;
    EXPECT_EQ(1, assigned.surfaceIndex);
    RayHit deferred(RayHit::DETAIL_NORMAL | RayHit::DETAIL_UV, false);
    ASSERT_EQ(RayFirstHitStatus::Hit, mesh.querySurfaceDetail(ray, boundary, &deferred));
    EXPECT_DOUBLE_EQ(1, deferred.normal.z());
    EXPECT_DOUBLE_EQ(0.25, deferred.u);
    EXPECT_DOUBLE_EQ(0.25, deferred.v);
    boundary.normalReversed = true;
    ASSERT_EQ(RayFirstHitStatus::Hit, mesh.querySurfaceDetail(ray, boundary, &deferred));
    EXPECT_DOUBLE_EQ(-1, deferred.normal.z());
    boundary.surfaceIndex = 99;
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, mesh.querySurfaceDetail(ray, boundary, &deferred));
    EXPECT_FALSE(deferred.hasHitDistance());
    EXPECT_EQ(RayFirstHitStatus::Miss, mesh.queryFirstHit(ray, RayCrossingQuery(2.1,3.9), &hit, &boundary));
    EXPECT_FALSE(hit.hasHitDistance());
    EXPECT_EQ(-1, boundary.surfaceIndex);
}

TEST(TriangleMeshContract, CoplanarityInvalidMeshAndEmptyAreExplicit) {
    TriangleMesh mesh;
    mesh.initVertexPositionsArray(6);
    java::ArrayList<double>& positions = mesh.getVertexPositions();
    const double values[] = {0,0,0, 1,0,0, 0,1,0, 0.25,0,0, 0.25,1,0, 0.25,0,1};
    for (int i = 0; i < 18; ++i) positions[i] = values[i];
    mesh.initTriangleArrays(2);
    mesh.setTriangleAt(0, Triangle(0,1,2));
    mesh.setTriangleAt(1, Triangle(3,4,5));
    const Ray ray = Ray::fromAffine(Vector3Dd(0.25,0.25,2), Vector3Dd(0,0,-1));
    RayCrossingTestCollector events;
    EXPECT_EQ(RayCrossingSupport::Indeterminate, mesh.emitRayCrossings(ray, RayCrossingQuery(0,4), events));
    EXPECT_TRUE(events.roots.isEmpty());
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, mesh.queryFirstHit(ray, RayCrossingQuery(0,4), nullptr));
    mesh.getTriangleIndexes()[4] = 999;
    EXPECT_EQ(RayCrossingSupport::Indeterminate, mesh.emitRayCrossings(ray, RayCrossingQuery(0,4), events));
    GeometryBounds bounds;
    mesh.getBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, bounds.getKind());
    TriangleMesh empty;
    EXPECT_EQ(RayCrossingSupport::Supported, empty.emitRayCrossings(ray, RayCrossingQuery(0,4), events));
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, empty.queryFirstHit(
        Ray::fromAffine(Vector3Dd(), Vector3Dd()), RayCrossingQuery(0,4), nullptr));
    EXPECT_EQ(RayFirstHitStatus::Unsupported, empty.queryFirstHit(ray, RayCrossingQuery(0,4,0.01), nullptr));
}

TEST(TriangleMeshContract, MathematicalCopyDoesNotBorrowOwnedAppearance) {
    TriangleMesh* copied = nullptr;
    {
        TriangleMesh original;
        original.initVertexPositionsArray(3);
        java::ArrayList<double>& positions = original.getVertexPositions();
        const double values[] = {0,0,0, 1,0,0, 0,1,0};
        for (int i = 0; i < 9; ++i) positions[i] = values[i];
        original.initTriangleArrays(1);
        original.setTriangleAt(0, Triangle(0,1,2));
        java::ArrayList<SimpleMaterial*> materials;
        materials.add(new SimpleMaterial());
        original.setMaterials(materials);
        original.setOwnsMaterials(true);
        copied = dynamic_cast<TriangleMesh*>(original.copyGeometry());
        ASSERT_NE(nullptr, copied);
        EXPECT_TRUE(copied->getMaterials().isEmpty());
    }
    RayBoundary boundary;
    EXPECT_EQ(RayFirstHitStatus::Hit, copied->queryFirstHit(
        Ray::fromAffine(Vector3Dd(0.25,0.25,1), Vector3Dd(0,0,-1)),
        RayCrossingQuery(0,2), nullptr, &boundary));
    EXPECT_EQ(copied, boundary.geometry);
    delete copied;
}
