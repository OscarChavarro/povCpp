#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetNonIntersectingClassifier.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

/*
Regression coverage for the boolean preflight that resolves operands without
real intersections ([MANT1988] table 15.1). Expected values match the Java
port (see the cross-port oracle of the boolean pipeline port plan).
*/
namespace {

typedef _PolyhedralBoundedSolidSetNonIntersectingClassifier Classifier;

PolyhedralBoundedSolid* createBox(double size, const Vector3Dd& offset)
{
    Box box(size, size, size);
    PolyhedralBoundedSolid* solid = box.exportToPolyhedralBoundedSolid();
    for ( long int i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        v->position = v->position.add(offset);
    }
    return solid;
}

}

TEST(PolyhedralBoundedSolidSetNonIntersectingClassifierTest, ContainedBoxIsDetected) {
    PolyhedralBoundedSolid* a = createBox(0.5, Vector3Dd(0, 0, 0));
    PolyhedralBoundedSolid* b = createBox(2.0, Vector3Dd(0, 0, 0));
    Classifier::_PreflightCache cache = Classifier::newPreflightCache(a, b);

    EXPECT_EQ(Geometry::INSIDE, cache.aInB());
    EXPECT_EQ(Geometry::OUTSIDE, cache.bInA());
    EXPECT_TRUE(Classifier::runContainmentOnlyPreflightCase(a, b, cache));
    EXPECT_FALSE(Classifier::runTouchingOnlyPreflightCase(a, b, cache));

    // A in B: the union is B
    PolyhedralBoundedSolid* result = new PolyhedralBoundedSolid();
    Classifier::runSetOpNoIntersectionCase(a, b, result, Classifier::UNION, cache);
    EXPECT_EQ(6, result->getPolygonsList().size());
    EXPECT_EQ(8, result->getVerticesList().size());
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result));
    delete result;
    delete a;
    delete b;
}

TEST(PolyhedralBoundedSolidSetNonIntersectingClassifierTest, SubtractingInnerBoxMakesTwoShells) {
    PolyhedralBoundedSolid* a = createBox(2.0, Vector3Dd(0, 0, 0));
    PolyhedralBoundedSolid* b = createBox(0.5, Vector3Dd(0.1, 0.2, 0.3));
    PolyhedralBoundedSolid* result = new PolyhedralBoundedSolid();

    Classifier::runSetOpNoIntersectionCase(a, b, result, Classifier::SUBTRACT);
    EXPECT_EQ(12, result->getPolygonsList().size());
    EXPECT_EQ(16, result->getVerticesList().size());
    EXPECT_EQ(12, result->getMaxFaceId());
    delete result;
    delete a;
    delete b;
}

TEST(PolyhedralBoundedSolidSetNonIntersectingClassifierTest, FaceTouchingBoxesAreTouchingOnly) {
    PolyhedralBoundedSolid* a = createBox(1.0, Vector3Dd(0, 0, 0));
    PolyhedralBoundedSolid* b = createBox(1.0, Vector3Dd(1, 0, 0));
    Classifier::_PreflightCache cache = Classifier::newPreflightCache(a, b);

    EXPECT_FALSE(Classifier::runContainmentOnlyPreflightCase(a, b, cache));
    EXPECT_TRUE(Classifier::runTouchingOnlyPreflightCase(a, b, cache));
    delete a;
    delete b;
}

TEST(PolyhedralBoundedSolidSetNonIntersectingClassifierTest, PartialCoplanarContactGivesLamina) {
    PolyhedralBoundedSolid* a = createBox(1.0, Vector3Dd(0, 0, 0));
    PolyhedralBoundedSolid* b = createBox(1.0, Vector3Dd(1, 0.5, 0.25));
    PolyhedralBoundedSolid* result = new PolyhedralBoundedSolid();

    EXPECT_FALSE(Classifier::runTouchingOnlyPreflightCase(a, b));
    ASSERT_EQ(result, Classifier::runPartialCoplanarFaceAreaCase(
        a, b, result, Classifier::INTERSECTION));
    // One rectangular lamina: two faces sharing a four vertex loop
    EXPECT_EQ(2, result->getPolygonsList().size());
    EXPECT_EQ(4, result->getVerticesList().size());
    delete result;
    delete a;
    delete b;
}

TEST(PolyhedralBoundedSolidSetNonIntersectingClassifierTest, OverlappingBoxesAreNotPreflighted) {
    PolyhedralBoundedSolid* a = createBox(1.0, Vector3Dd(0, 0, 0));
    PolyhedralBoundedSolid* b = createBox(1.0, Vector3Dd(0.5, 0.4, 0.3));
    PolyhedralBoundedSolid* result = new PolyhedralBoundedSolid();
    Classifier::_PreflightCache cache = Classifier::newPreflightCache(a, b);

    EXPECT_TRUE(cache.hasInteriorOverlap());
    EXPECT_FALSE(Classifier::runContainmentOnlyPreflightCase(a, b, cache));
    EXPECT_FALSE(Classifier::runTouchingOnlyPreflightCase(a, b, cache));
    EXPECT_EQ(0, Classifier::runPartialCoplanarFaceAreaCase(
        a, b, result, Classifier::INTERSECTION, cache));
    delete result;
    delete a;
    delete b;
}
