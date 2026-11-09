#ifndef __LINEAR_ALGEBRA_ENGINE__
#define __LINEAR_ALGEBRA_ENGINE__

#include "vsdk/toolkit/common/linealAlgebra/MatrixNxM.h"
#include "vsdk/toolkit/common/linealAlgebra/processing/Strategies.h"
class LinearAlgebraEngine {
private:
    ComputeStrategy strategy;

public:
    explicit LinearAlgebraEngine(ComputeStrategy strategy = NAIVE_COFACTOR_CPU);

    static LinearAlgebraEngine defaultEngine();
    static LinearAlgebraEngine fromStrategy(ComputeStrategy strategy);

    double determinant(const MatrixNxM& m) const;
    MatrixNxM inverse(const MatrixNxM& m) const;

    static double determinantDefault(const MatrixNxM& m);
    static MatrixNxM inverseDefault(const MatrixNxM& m);
};

#endif
