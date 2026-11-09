#include <cmath>

#include <gtest/gtest.h>
#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Quaternionf.h"
TEST(QuaterniondTest, RotationAndConjugate) {
    double half90 = M_PI / 4.0;
    Quaterniond q(Vector3Dd(0.0, 0.0, std::sin(half90)), std::cos(half90));
    EXPECT_TRUE(q.rotate(Vector3Dd(1,0,0)).epsilonEquals(Vector3Dd(0,1,0), 1e-8));
    EXPECT_TRUE(q.conjugated().direction().epsilonEquals(q.direction().multiply(-1.0), 1e-9));
}

TEST(QuaternionfTest, RotationAndConjugate) {
    float half90 = (float)M_PI / 4.0f;
    Quaternionf q(Vector3Df(0.0f, 0.0f, std::sin(half90)), std::cos(half90));
    EXPECT_TRUE(q.rotate(Vector3Df(1,0,0)).epsilonEquals(Vector3Df(0,1,0), 1e-4f));
    EXPECT_TRUE(q.conjugated().direction().epsilonEquals(q.direction().multiply(-1.0f), 1e-5f));
}
