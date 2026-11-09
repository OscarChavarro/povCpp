#include <gtest/gtest.h>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"

/*
Exercises the classification of points against boxes (centered at the
origin) and cones (axis along Z, base at z = 0), as used by the voxelization
of solids. C++ counterpart of Java's `BoxConeContainmentTest`.
*/
namespace {
const double TOLERANCE = 0.01;
}

TEST(BoxConeContainmentTest, BoxPointsInsideLimitAndOutsideAreDistinguished) {
    // Arrange
    Box box(2, 4, 6);

    // Act & Assert
    EXPECT_EQ(box.doContainmentTest(Vector3Dd(0.5, -1.5, 2.5), TOLERANCE), Geometry::INSIDE);
    EXPECT_EQ(box.doContainmentTest(Vector3Dd(1.0, 0, 0), TOLERANCE), Geometry::LIMIT);
    EXPECT_EQ(box.doContainmentTest(Vector3Dd(0, -2.005, 0), TOLERANCE), Geometry::LIMIT);
    EXPECT_EQ(box.doContainmentTest(Vector3Dd(0, 0, 3.1), TOLERANCE), Geometry::OUTSIDE);
    EXPECT_EQ(box.doContainmentTest(Vector3Dd(1.5, 0, 0), TOLERANCE), Geometry::OUTSIDE);
}

TEST(BoxConeContainmentTest, ConePointsRespectSideAndCaps) {
    // Arrange: radius 1 at z = 0, apex at z = 2
    Cone cone(1, 0, 2);

    // Act & Assert
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0, 0, 1), TOLERANCE), Geometry::INSIDE);
    // At z = 1 the radius is 0.5
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0.45, 0, 1), TOLERANCE), Geometry::INSIDE);
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0.6, 0, 1), TOLERANCE), Geometry::OUTSIDE);
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0, 0.5, 1), TOLERANCE), Geometry::LIMIT);
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0.2, 0, 0), TOLERANCE), Geometry::LIMIT);
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0, 0, -0.5), TOLERANCE), Geometry::OUTSIDE);
    EXPECT_EQ(cone.doContainmentTest(Vector3Dd(0, 0, 2.5), TOLERANCE), Geometry::OUTSIDE);
}

TEST(BoxConeContainmentTest, CylinderBehavesAsAStraightTube) {
    // Arrange
    Cone cylinder(1, 1, 2);

    // Act & Assert
    EXPECT_EQ(cylinder.doContainmentTest(Vector3Dd(0.9, 0, 1.9), TOLERANCE), Geometry::INSIDE);
    EXPECT_EQ(cylinder.doContainmentTest(Vector3Dd(0, 1.0, 1), TOLERANCE), Geometry::LIMIT);
    EXPECT_EQ(cylinder.doContainmentTest(Vector3Dd(0.8, 0.8, 1), TOLERANCE), Geometry::OUTSIDE);
}
