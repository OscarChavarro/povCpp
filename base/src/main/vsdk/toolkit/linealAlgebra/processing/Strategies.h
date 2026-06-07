#ifndef __VSDK_TOOLKIT_PROCESSING_LINEALALGEBRA_STRATEGIES_H__
#define __VSDK_TOOLKIT_PROCESSING_LINEALALGEBRA_STRATEGIES_H__

#include "vsdk/toolkit/common/linealAlgebra/MatrixNxM.h"

class DeterminantStrategy {
public:
    virtual ~DeterminantStrategy() {}
    virtual const char* id() const = 0;
    virtual double determinant(const MatrixNxM& matrix) const = 0;
};

class InverseStrategy {
public:
    virtual ~InverseStrategy() {}
    virtual const char* id() const = 0;
    virtual MatrixNxM inverse(const MatrixNxM& matrix) const = 0;
};

class NaiveCofactorCpuStrategy : public DeterminantStrategy, public InverseStrategy {
public:
    virtual const char* id() const;
    virtual double determinant(const MatrixNxM& matrix) const;
    virtual MatrixNxM inverse(const MatrixNxM& matrix) const;
};

class LuCpuStrategy : public DeterminantStrategy, public InverseStrategy {
public:
    virtual const char* id() const;
    virtual double determinant(const MatrixNxM& matrix) const;
    virtual MatrixNxM inverse(const MatrixNxM& matrix) const;
};

class GaussCpuStrategy : public DeterminantStrategy, public InverseStrategy {
public:
    virtual const char* id() const;
    virtual double determinant(const MatrixNxM& matrix) const;
    virtual MatrixNxM inverse(const MatrixNxM& matrix) const;
};

enum ComputeStrategy {
    NAIVE_COFACTOR_CPU,
    LU_CPU,
    GAUSS_CPU
};

#endif // __VSDK_TOOLKIT_PROCESSING_LINEALALGEBRA_STRATEGIES_H__
