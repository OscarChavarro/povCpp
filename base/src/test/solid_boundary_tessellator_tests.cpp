#include <cmath>
#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobComponent.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBody.h"
#include "vsdk/toolkit/processing/BezierPatchTessellator.h"
#include "vsdk/toolkit/processing/SolidBoundaryTessellator.h"
#include "TessellationMeshCheck.h"

// A placed sphere: a closed, outward mesh of genus 0 whose vertices lie on the
// sphere, with the area and volume of the sphere within the resolution.
TEST(SolidBoundaryTessellatorContract, SphereMeshIsClosedOutwardAndOnTheSurface) {
    SimpleBody ball;
    ball.setGeometry(new Sphere(1.0));
    ball.setPosition(Vector3Dd(0.3, -0.2, 0.1));
    GeometryBounds region;
    ASSERT_TRUE(region.setFinite(Vector3Dd(-0.8, -1.3, -1.0), Vector3Dd(1.4, 0.9, 1.2)));
    SolidBoundaryTessellationReport report;
    TriangleMesh* mesh = SolidBoundaryTessellator::tessellate(ball, region, 32, report);
    ASSERT_NE(nullptr, mesh);
    EXPECT_TRUE(TessellationMeshCheck::isClosedAndOriented(*mesh));
    EXPECT_EQ(2, TessellationMeshCheck::eulerCharacteristic(*mesh));
    EXPECT_NEAR(4.0*M_PI/3.0, TessellationMeshCheck::enclosedVolume(*mesh), 0.03*4.0*M_PI/3.0);
    EXPECT_NEAR(4.0*M_PI, TessellationMeshCheck::area(*mesh), 0.05*4.0*M_PI);
    EXPECT_EQ(0, report.midpointCrossings);
    EXPECT_EQ(0, report.regionCrossings);
    EXPECT_EQ(0, report.pointClassifiedLines);
    EXPECT_EQ(report.triangles, mesh->getNumTriangles());
    EXPECT_EQ(report.boundaryVertices + report.cellVertices, mesh->getNumVertices());
    EXPECT_GT(report.boundaryVertices, 0.95*mesh->getNumVertices());
    const java::ArrayList<double>& normals = mesh->getVertexNormals();
    int onSurface = 0;
    for (int v = 0; v < mesh->getNumVertices(); v++) {
        const Vector3Dd p = TessellationMeshCheck::vertex(*mesh, v).subtract(ball.getPosition());
        EXPECT_NEAR(1.0, p.length(), report.cellSize);
        if (std::fabs(p.length() - 1.0) < 1e-9) onSurface++;
        // Vertex normals are the sphere's outward normals.
        const Vector3Dd n(normals[v*3], normals[v*3 + 1], normals[v*3 + 2]);
        EXPECT_GT(n.dotProduct(p.normalized()), 0.9);
    }
    EXPECT_EQ(report.boundaryVertices, onSurface);
    delete mesh;
}

// A CSG difference: the cavity is a second shell facing into it.
TEST(SolidBoundaryTessellatorContract, CsgDifferenceKeepsTheCavity) {
    for (int e = 0; e < 2; e++) {
        CsgBody body(CsgOperation::DIFFERENCE, e == 0 ? CsgBodyEvaluator::ROTH : CsgBodyEvaluator::CROSSING_MEMBERSHIP);
        SimpleBody* outer = new SimpleBody();
        outer->setGeometry(new Box(2.0, 2.0, 2.0));
        SimpleBody* hole = new SimpleBody();
        hole->setGeometry(new Sphere(0.6));
        ASSERT_TRUE(body.addLeaf(0, outer));
        ASSERT_TRUE(body.addLeaf(0, hole));
        GeometryBounds region;
        ASSERT_TRUE(region.setFinite(Vector3Dd(-1.23, -1.23, -1.23), Vector3Dd(1.23, 1.23, 1.23)));
        SolidBoundaryTessellationReport report;
        TriangleMesh* mesh = SolidBoundaryTessellator::tessellate(body, region, 24, report);
        ASSERT_NE(nullptr, mesh);
        EXPECT_TRUE(TessellationMeshCheck::isClosedAndOriented(*mesh)) << e;
        // Two spheres' worth of Euler characteristic: the box shell and the cavity.
        EXPECT_EQ(4, TessellationMeshCheck::eulerCharacteristic(*mesh)) << e;
        const double expected = 8.0 - 4.0*M_PI*0.216/3.0;
        EXPECT_NEAR(expected, TessellationMeshCheck::enclosedVolume(*mesh), 0.04*expected) << e;
        EXPECT_EQ(0, report.regionCrossings) << e;
        EXPECT_GT(report.exactCrossings, 0) << e;
        delete mesh;
    }
}

// A blob of two merged components is one closed shell.
TEST(SolidBoundaryTessellatorContract, MergedBlobIsOneShell) {
    BlobSolid* blob = new BlobSolid(0.6, false);
    blob->addComponent(BlobComponent(Vector3Dd(-0.8, 0, 0), 1.5, 1.0));
    blob->addComponent(BlobComponent(Vector3Dd(0.8, 0, 0), 1.5, 1.0));
    SimpleBody body;
    body.setGeometry(blob);
    GeometryBounds region;
    body.getSolidWorldBounds(region);
    ASSERT_EQ(GeometryBounds::Kind::Finite, region.getKind());
    SolidBoundaryTessellationReport report;
    TriangleMesh* mesh = SolidBoundaryTessellator::tessellate(body, region.inflated(0.1), 40, report);
    ASSERT_NE(nullptr, mesh);
    EXPECT_TRUE(TessellationMeshCheck::isClosedAndOriented(*mesh));
    EXPECT_EQ(2, TessellationMeshCheck::eulerCharacteristic(*mesh));
    EXPECT_GT(TessellationMeshCheck::enclosedVolume(*mesh), 0);
    EXPECT_EQ(0, report.regionCrossings);
    for (int v = 0; v < mesh->getNumVertices(); v++) {
        const Vector3Dd p = TessellationMeshCheck::vertex(*mesh, v);
        // Every vertex is within a cell of the threshold surface.
        EXPECT_LT(std::fabs(blob->evaluateField(p)), 1.0) << v;
    }
    delete mesh;
}

// A solid larger than the region is closed by the region's sides.
TEST(SolidBoundaryTessellatorContract, RegionClosesASolidThatLeavesIt) {
    SimpleBody ball;
    ball.setGeometry(new Sphere(3.0));
    GeometryBounds region;
    ASSERT_TRUE(region.setFinite(Vector3Dd(-1, -1, -1), Vector3Dd(1, 1, 1)));
    SolidBoundaryTessellationReport report;
    TriangleMesh* mesh = SolidBoundaryTessellator::tessellate(ball, region, 8, report);
    ASSERT_NE(nullptr, mesh);
    EXPECT_TRUE(TessellationMeshCheck::isClosedAndOriented(*mesh));
    EXPECT_NEAR(8.0, TessellationMeshCheck::enclosedVolume(*mesh), 1e-9);
    EXPECT_GT(report.regionCrossings, 0);
    EXPECT_EQ(0, report.exactCrossings);
    delete mesh;
}

TEST(SolidBoundaryTessellatorContract, InvalidRequestsAndEmptyResults) {
    SimpleBody ball;
    ball.setGeometry(new Sphere(1.0));
    SolidBoundaryTessellationReport report;
    GeometryBounds region;
    EXPECT_EQ(nullptr, SolidBoundaryTessellator::tessellate(ball, region, 8, report));
    region.setUnbounded();
    EXPECT_EQ(nullptr, SolidBoundaryTessellator::tessellate(ball, region, 8, report));
    ASSERT_TRUE(region.setFinite(Vector3Dd(-1, -1, -1), Vector3Dd(1, 1, 1)));
    EXPECT_EQ(nullptr, SolidBoundaryTessellator::tessellate(ball, region, 0, report));
    EXPECT_EQ(nullptr, SolidBoundaryTessellator::tessellate(ball, region,
        SolidBoundaryTessellator::MAXIMUM_RESOLUTION + 1, report));
    // A region away from the solid gives an empty mesh.
    ASSERT_TRUE(region.setFinite(Vector3Dd(5, 5, 5), Vector3Dd(6, 6, 6)));
    TriangleMesh* mesh = SolidBoundaryTessellator::tessellate(ball, region, 8, report);
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(0, mesh->getNumTriangles());
    delete mesh;
}

// The patch mesh samples the Bezier surface and faces its u x v side.
TEST(BezierPatchTessellatorContract, VerticesLieOnTheBezierSurface) {
    Vector3Dd net[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            net[i][j] = Vector3Dd(i, j, (i == 1 || i == 2) && (j == 1 || j == 2) ? 0.6 : 0.2*i);
        }
    }
    SubdividedBezierPatch patch(BezierPatchSubdivision::UniformGrid, 4, 4, 0.0, net);
    TriangleMesh* mesh = BezierPatchTessellator::tessellate(patch, 6, 9);
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(7*10, mesh->getNumVertices());
    EXPECT_EQ(2*6*9, mesh->getNumTriangles());
    const java::ArrayList<double>& uvs = mesh->getVertexUvs();
    const java::ArrayList<double>& faceNormals = mesh->getTriangleNormals();
    const java::ArrayList<int>& indices = mesh->getTriangleIndexes();
    for (int v = 0; v < mesh->getNumVertices(); v++) {
        const Vector3Dd expected = patch.evaluate(uvs[v*2], uvs[v*2 + 1]);
        EXPECT_LT(TessellationMeshCheck::vertex(*mesh, v).subtract(expected).length(), 1e-12);
    }
    for (int t = 0; t < mesh->getNumTriangles(); t++) {
        double u = 0, v = 0;
        for (int k = 0; k < 3; k++) {
            u += uvs[indices[t*3 + k]*2]/3.0;
            v += uvs[indices[t*3 + k]*2 + 1]/3.0;
        }
        const Vector3Dd n(faceNormals[t*3], faceNormals[t*3 + 1], faceNormals[t*3 + 2]);
        EXPECT_GT(n.dotProduct(BezierPatchTessellator::surfaceNormal(patch, u, v)), 0.9) << t;
    }
    // The vertex normals are S_u x S_v, here +z at the center of the flat-sided net.
    const Vector3Dd center = BezierPatchTessellator::surfaceNormal(patch, 0.5, 0.5);
    EXPECT_GT(center.z(), 0.9);
    delete mesh;
    EXPECT_EQ(nullptr, BezierPatchTessellator::tessellate(patch, 0, 3));
    EXPECT_EQ(nullptr, BezierPatchTessellator::tessellate(patch, 3, BezierPatchTessellator::MAXIMUM_STEPS + 1));
}
