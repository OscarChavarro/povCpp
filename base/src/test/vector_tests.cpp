#include <gtest/gtest.h>
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Df.h"
TEST(Vector2DdTest, AddAndScale) {
    EXPECT_TRUE(Vector2Dd(1.5, -2.0).add(Vector2Dd(-0.5, 3.0)).epsilonEquals(Vector2Dd(1.0, 1.0), 1e-9));
    EXPECT_TRUE(Vector2Dd(2.0, -3.0).multiply(2.5).epsilonEquals(Vector2Dd(5.0, -7.5), 1e-12));
}

TEST(Vector2DfTest, AddAndScale) {
    EXPECT_TRUE(Vector2Df(1.5f, -2.0f).add(Vector2Df(-0.5f, 3.0f)).epsilonEquals(Vector2Df(1.0f, 1.0f), 1e-6f));
    EXPECT_TRUE(Vector2Df(2.0f, -3.0f).multiply(2.5f).epsilonEquals(Vector2Df(5.0f, -7.5f), 1e-5f));
}

TEST(Vector3DdTest, CrossAndNormalize) {
    Vector3Dd c = Vector3Dd(1,0,0).crossProduct(Vector3Dd(0,1,0));
    EXPECT_TRUE(c.epsilonEquals(Vector3Dd(0,0,1), 1e-9));
    EXPECT_TRUE(Vector3Dd(3,4,12).normalized().epsilonEquals(Vector3Dd(3,4,12).multiply(1.0/13.0), 1e-12));
}

TEST(Vector3DfTest, CrossAndNormalize) {
    Vector3Df c = Vector3Df(1,0,0).crossProduct(Vector3Df(0,1,0));
    EXPECT_TRUE(c.epsilonEquals(Vector3Df(0,0,1), 1e-5f));
    EXPECT_TRUE(Vector3Df(3,4,12).normalized().epsilonEquals(Vector3Df(3,4,12).multiply(1.0f/13.0f), 1e-5f));
}

TEST(Vector4Test, DivideByW) {
    EXPECT_TRUE(Vector4Dd(4,6,8,2).dividedByW().epsilonEquals(Vector4Dd(2,3,4,1), 1e-12));
    EXPECT_TRUE(Vector4Df(4,6,8,2).dividedByW().epsilonEquals(Vector4Df(2,3,4,1), 1e-5f));
}
