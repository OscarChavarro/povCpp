#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/SimpleTestGeometryLibrary.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"

/*
Exercises `PolyhedralBoundedSolid::doContainmentTest`, as the
`doContainmentTest` tests of Java's `PolyhedralBoundedSolidPredicatesTest`.
*/
namespace {

/**
As `PolyhedralBoundedSolidTestFixtures.createBoxSolid` of the Java tests.
*/
PolyhedralBoundedSolid* createBoxSolid(double sx, double sy, double sz,
                                       double tx, double ty, double tz)
{
    Box box(Vector3Dd(sx, sy, sz));
    PolyhedralBoundedSolid* solid = box.exportToPolyhedralBoundedSolid();
    Matrix4x4d translation;
    translation = translation.translation(tx, ty, tz);
    PolyhedralBoundedSolidModeler::applyTransformation(solid, translation);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

}

TEST(PolyhedralBoundedSolidPredicatesTest, BoxSolidContainmentTestDistinguishesFaces) {
    // Arrange: unit box centered at (2, 0, 0)
    PolyhedralBoundedSolid* solid = createBoxSolid(1, 1, 1, 2, 0, 0);
    double tolerance = 0.01;

    // Act & Assert
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(2, 0, 0), tolerance), Geometry::INSIDE);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(2.5, 0.1, -0.2), tolerance), Geometry::LIMIT);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(2, 0, 0.505), tolerance), Geometry::LIMIT);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(2, 0, 0.6), tolerance), Geometry::OUTSIDE);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(0, 0, 0), tolerance), Geometry::OUTSIDE);
    delete solid;
}

TEST(PolyhedralBoundedSolidPredicatesTest, PointInTheNotchOfANonConvexSolidIsOutside) {
    // Arrange: profile of [MANT1986].4 in the XZ plane, extruded along Y
    // from y = 0.4 to y = 0; its top has a notch between x = 0.37 and
    // x = 0.60 down to z = 0.30
    PolyhedralBoundedSolid* solid =
        SimpleTestGeometryLibrary::createTestObjectMANT1986_1();
    double tolerance = 0.001;

    // Act & Assert
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(0.48, 0.2, 0.40), tolerance), Geometry::OUTSIDE);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(0.48, 0.2, 0.20), tolerance), Geometry::INSIDE);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(0.90, 0.2, 0.40), tolerance), Geometry::INSIDE);
    EXPECT_EQ(solid->doContainmentTest(Vector3Dd(0.48, 0.2, 0.30), tolerance), Geometry::LIMIT);
    delete solid;
}
