#ifndef __VSDK_TOOLKIT_PROCESSING_LINEALALGEBRA_LINEARALGEBRAENGINE_H__
#define __VSDK_TOOLKIT_PROCESSING_LINEALALGEBRA_LINEARALGEBRAENGINE_H__

#include "vsdk/toolkit/common/linealAlgebra/MatrixNxM.h"
#include "vsdk/toolkit/linealAlgebra/processing/Strategies.h"

class LinearAlgebraEngine {
private:
    ComputeStrategy strategy_;

public:
    explicit LinearAlgebraEngine(ComputeStrategy strategy = NAIVE_COFACTOR_CPU);

    static LinearAlgebraEngine defaultEngine();
    static LinearAlgebraEngine fromStrategy(ComputeStrategy strategy);

    double determinant(const MatrixNxM& m) const;
    MatrixNxM inverse(const MatrixNxM& m) const;

    static double determinantDefault(const MatrixNxM& m);
    static MatrixNxM inverseDefault(const MatrixNxM& m);
};

#endif // __VSDK_TOOLKIT_PROCESSING_LINEALALGEBRA_LINEARALGEBRAENGINE_H__
