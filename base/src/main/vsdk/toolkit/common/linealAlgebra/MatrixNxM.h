#ifndef __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIXNXM_H__
#define __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIXNXM_H__
#include "java/lang/String.h"

class MatrixNxM {
private:
    int numRows_;
    int numColumns_;
    double* m_;

    int index(int row, int column) const;

public:
    MatrixNxM(int n, int m);
    MatrixNxM(const MatrixNxM& other);
    ~MatrixNxM();
    MatrixNxM& operator=(const MatrixNxM& other);

    static MatrixNxM copyOf(const MatrixNxM& other);

    MatrixNxM identity() const;
    int getNumRows() const;
    int getNumColumns() const;
    double getVal(int row, int column) const;
    MatrixNxM withVal(int row, int column, double val) const;

    MatrixNxM inverse() const;
    MatrixNxM cofactors() const;
    MatrixNxM transpose() const;
    MatrixNxM multiply(double a) const;
    MatrixNxM multiply(const MatrixNxM& other) const;
    MatrixNxM buildMinor(int row, int column) const;
    double determinant() const;
    java::String* toString() const;

    bool epsilonEquals(const MatrixNxM& other) const;
    bool epsilonEquals(const MatrixNxM& other, double epsilon) const;

    bool operator==(const MatrixNxM& other) const;
    bool equals(const MatrixNxM& other) const { return (*this) == other; }
    int hashCode() const;
};


#endif // __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIXNXM_H__
