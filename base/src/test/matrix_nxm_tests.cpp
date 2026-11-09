#include <gtest/gtest.h>
#include "vsdk/toolkit/common/linealAlgebra/MatrixNxM.h"
#include "vsdk/toolkit/common/linealAlgebra/exceptions/MatrixExceptions.h"
TEST(MatrixNxMTest, DeterminantInverse) {
    MatrixNxM m = MatrixNxM(2,2).withVal(0,0,4).withVal(0,1,7).withVal(1,0,2).withVal(1,1,6);
    EXPECT_NEAR(m.determinant(), 10.0, 1e-9);
    EXPECT_TRUE(m.multiply(m.inverse()).epsilonEquals(MatrixNxM(2,2), 1e-8));
}

TEST(MatrixNxMTest, Exceptions) {
    EXPECT_THROW(MatrixNxM(0,2), MatrixDimensionMismatchException);
    EXPECT_THROW(MatrixNxM(2,3).determinant(), MatrixNotSquareException);
    EXPECT_THROW(MatrixNxM(1,1).buildMinor(0,0), MatrixDimensionMismatchException);
}
