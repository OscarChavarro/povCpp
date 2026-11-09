#include <cmath>
#include <gtest/gtest.h>
#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/SimpleTestGeometryLibrary.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

/*
Regression coverage for the core B-rep editing and validation services
(Euler operators, maximal faces, vertex welding, strict validation) that the
boolean pipeline relies on. Expected values match the Java port.
*/
namespace {

PolyhedralBoundedSolid* createBox()
{
    Box box(1, 2, 3);
    return box.exportToPolyhedralBoundedSolid();
}

}

TEST(PolyhedralBoundedSolidTopologyEditingTest, MaximizeFacesRemovesInessentialEdgeAndVertex) {
    PolyhedralBoundedSolid* solid = createBox();
    _PolyhedralBoundedSolidHalfEdge* he =
        solid->getPolygonsList().get(0)->boundariesList.get(0)->boundaryStartHalfEdge;
    PolyhedralBoundedSolidEulerOperators::lmef(
        solid, he, he->next()->next(), solid->getMaxFaceId() + 1);
    _PolyhedralBoundedSolidEdge* e = solid->getEdgesList().get(0);
    Vector3Dd mid = e->rightHalf->startingVertex->position.add(
        e->leftHalf->startingVertex->position).multiply(0.5);
    PolyhedralBoundedSolidEulerOperators::lmev(
        solid, e->rightHalf, e->leftHalf->next(), solid->getMaxVertexId() + 1, mid);
    ASSERT_EQ(7, solid->getPolygonsList().size());
    ASSERT_EQ(14, solid->getEdgesList().size());
    ASSERT_EQ(9, solid->getVerticesList().size());

    PolyhedralBoundedSolidTopologyEditing::maximizeFaces(solid);
    PolyhedralBoundedSolidTopologyEditing::compactIds(solid);

    EXPECT_EQ(6, solid->getPolygonsList().size());
    EXPECT_EQ(12, solid->getEdgesList().size());
    EXPECT_EQ(8, solid->getVerticesList().size());
    EXPECT_EQ(8, solid->getMaxVertexId());
    EXPECT_EQ(6, solid->getMaxFaceId());
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateStrict(solid));
    delete solid;
}

TEST(PolyhedralBoundedSolidTopologyEditingTest, WeldCollapsesZeroLengthStrut) {
    PolyhedralBoundedSolid* solid = createBox();
    _PolyhedralBoundedSolidHalfEdge* he =
        solid->getPolygonsList().get(2)->boundariesList.get(0)->boundaryStartHalfEdge;
    PolyhedralBoundedSolidEulerOperators::lmev(
        solid, he, he, solid->getMaxVertexId() + 1, he->startingVertex->position);
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
    java::String msg;

    EXPECT_FALSE(PolyhedralBoundedSolidGeometricValidator::validateNoCoincidentVertices(
        solid, context, &msg));
    EXPECT_EQ(1, PolyhedralBoundedSolidTopologyEditing::weldCoincidentVertices(
        solid, context));
    EXPECT_EQ(8, solid->getVerticesList().size());
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid));
    delete solid;
}

TEST(PolyhedralBoundedSolidTopologyEditingTest, RevertedFixtureKeepsStrictValidity) {
    PolyhedralBoundedSolid* solid =
        SimpleTestGeometryLibrary::createTestObjectMANT1986_1();
    solid->revert();
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateStrict(solid));
    delete solid;
}

TEST(PolyhedralBoundedSolidTopologyEditingTest, MefLooksUpBothFaces) {
    PolyhedralBoundedSolid* solid = createBox();
    _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(0);
    _PolyhedralBoundedSolidHalfEdge* he1 =
        face->boundariesList.get(0)->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* he2 = he1->next()->next();
    int v1 = he1->startingVertex->id;
    int v2 = he1->next()->startingVertex->id;
    int v3 = he2->startingVertex->id;
    int v4 = he2->next()->startingVertex->id;

    // As Java, `v1 -> v2` is searched in face `f1`, so an unknown `f1` fails
    EXPECT_FALSE(PolyhedralBoundedSolidEulerOperators::mef(
        solid, 99, face->id, v1, v2, v3, v4, 7));
    EXPECT_EQ(6, solid->getPolygonsList().size());
    EXPECT_TRUE(PolyhedralBoundedSolidEulerOperators::mef(
        solid, face->id, face->id, v1, v2, v3, v4, 7));
    EXPECT_EQ(7, solid->getPolygonsList().size());
    delete solid;
}

TEST(PolyhedralBoundedSolidTopologyEditingTest, BoxIdentityPreflight) {
    PolyhedralBoundedSolid* a = createBox();
    PolyhedralBoundedSolid* b = createBox();
    PolyhedralBoundedSolid* c =
        SimpleTestGeometryLibrary::createTestObjectMANT1986_1();
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::areGeometricallyIdentical(a, b, 1e-6));
    EXPECT_FALSE(PolyhedralBoundedSolidValidationEngine::areGeometricallyIdentical(a, c, 1e-6));
    delete a;
    delete b;
    delete c;
}

TEST(PolyhedralBoundedSolidTopologyEditingTest, RayStartingOnSurfaceKeepsNegativeZeroT) {
    PolyhedralBoundedSolid* solid = createBox();
    RayHit hit;
    ASSERT_TRUE(solid->doIntersectionFirstHit(
        Ray(Vector3Dd(0.5, 1, 1.5), Vector3Dd(1, 0, 0)), &hit));
    // As Java, the -0.0 distance survives `Ray::withT`
    EXPECT_TRUE(std::signbit(hit.getRay()->getT()));
    delete solid;
}

TEST(JavaDoubleCompareTest, DistinguishesSignedZeroAndOrdersNaNLast) {
    EXPECT_LT(java::Double::compare(-0.0, 0.0), 0);
    EXPECT_GT(java::Double::compare(0.0, -0.0), 0);
    EXPECT_EQ(0, java::Double::compare(1.5, 1.5));
    EXPECT_EQ(0, java::Double::compare(NAN, NAN));
    EXPECT_GT(java::Double::compare(NAN, 1e308), 0);
    EXPECT_LT(java::Double::compare(-1.0, 2.0), 0);
}
