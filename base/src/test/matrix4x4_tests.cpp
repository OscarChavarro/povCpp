#include <cmath>

#include <gtest/gtest.h>
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
TEST(Matrix4x4dTest, TranslationRotationInverse) {
    Matrix4x4d t = Matrix4x4d().translation(5, -2, 1.5);
    EXPECT_TRUE(t.multiply(Vector3Dd(1,2,3)).epsilonEquals(Vector3Dd(6,0,4.5), 1e-9));

    Matrix4x4d r = Matrix4x4d().axisRotation(M_PI/2.0, 0,0,1);
    EXPECT_TRUE(r.multiply(Vector3Dd(1,0,0)).epsilonEquals(Vector3Dd(0,1,0), 1e-8));

    Matrix4x4d m = Matrix4x4d().scale(2,3,4).multiply(Matrix4x4d().translation(5,-1,2));
    EXPECT_TRUE(m.multiply(m.invert()).epsilonEquals(Matrix4x4d(), 1e-6));
}

TEST(Matrix4x4fTest, BasicOps) {
    Matrix4x4f t = Matrix4x4f().translation(5, -2, 1.5f);
    EXPECT_TRUE(t.multiply(Vector3Df(1,2,3)).epsilonEquals(Vector3Df(6,0,4.5f), 1e-4f));

    Matrix4x4f m = Matrix4x4f().scale(2,3,4).multiply(Matrix4x4f().translation(5,-1,2));
    EXPECT_TRUE(m.multiply(m.invert()).epsilonEquals(Matrix4x4f(), 1e-4f));
}
