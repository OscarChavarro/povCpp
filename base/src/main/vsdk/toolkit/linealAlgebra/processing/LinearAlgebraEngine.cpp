#include "vsdk/toolkit/linealAlgebra/processing/LinearAlgebraEngine.h"

#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/exceptions/MatrixExceptions.h"

static void requireSquare(const MatrixNxM& matrix)
{
    if ( matrix.getNumRows() != matrix.getNumColumns() ) {
        throw MatrixNotSquareException("Matrix must be square");
    }
}

static double** allocSquare(int n)
{
    double** a = new double*[n];
    for ( int i = 0; i < n; ++i ) {
        a[i] = new double[n];
        for ( int j = 0; j < n; ++j ) a[i][j] = 0.0;
    }
    return a;
}

static void freeSquare(double** a, int n)
{
    for ( int i = 0; i < n; ++i ) delete[] a[i];
    delete[] a;
}

static double** toArraySquare(const MatrixNxM& matrix)
{
    int n = matrix.getNumRows();
    double** a = allocSquare(n);
    for ( int i = 0; i < n; ++i ) {
        for ( int j = 0; j < n; ++j ) {
            a[i][j] = matrix.getVal(i, j);
        }
    }
    return a;
}

static double determinantRecursive(double** matrix, int n)
{
    if ( n == 1 ) return matrix[0][0];
    if ( n == 2 ) return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];

    double accum = 0.0;
    for ( int col = 0; col < n; ++col ) {
        double** minor = allocSquare(n - 1);
        for ( int i = 1; i < n; ++i ) {
            for ( int j = 0, mj = 0; j < n; ++j ) {
                if ( j == col ) continue;
                minor[i - 1][mj++] = matrix[i][j];
            }
        }
        double sign = ((col % 2) == 0) ? 1.0 : -1.0;
        accum += sign * matrix[0][col] * determinantRecursive(minor, n - 1);
        freeSquare(minor, n - 1);
    }
    return accum;
}

const char* NaiveCofactorCpuStrategy::id() const { return "naive-cofactor-cpu"; }

double NaiveCofactorCpuStrategy::determinant(const MatrixNxM& matrix) const
{
    requireSquare(matrix);
    int n = matrix.getNumRows();
    double** a = toArraySquare(matrix);
    double d = determinantRecursive(a, n);
    freeSquare(a, n);
    return d;
}

MatrixNxM NaiveCofactorCpuStrategy::inverse(const MatrixNxM& matrix) const
{
    requireSquare(matrix);
    int n = matrix.getNumRows();
    double** source = toArraySquare(matrix);
    double det = determinantRecursive(source, n);
    if ( std::abs(det) < VSDK::EPSILON ) {
        freeSquare(source, n);
        throw MatrixSingularException("Trying to invert a matrix with zero determinant");
    }

    MatrixNxM out(n, n);
    for ( int row = 0; row < n; ++row ) {
        for ( int col = 0; col < n; ++col ) {
            double** minor = allocSquare(n - 1);
            for ( int i = 0, mi = 0; i < n; ++i ) {
                if ( i == row ) continue;
                for ( int j = 0, mj = 0; j < n; ++j ) {
                    if ( j == col ) continue;
                    minor[mi][mj++] = source[i][j];
                }
                mi++;
            }
            double sign = ((row + col) % 2 == 0) ? 1.0 : -1.0;
            double cofactor = sign * determinantRecursive(minor, n - 1);
            out = out.withVal(col, row, cofactor / det);
            freeSquare(minor, n - 1);
        }
    }

    freeSquare(source, n);
    return out;
}

const char* LuCpuStrategy::id() const { return "lu-cpu"; }

double LuCpuStrategy::determinant(const MatrixNxM& matrix) const
{
    requireSquare(matrix);
    int n = matrix.getNumRows();
    double** lu = toArraySquare(matrix);
    int pivSign = 1;

    for ( int k = 0; k < n; ++k ) {
        int p = k;
        double max = std::abs(lu[k][k]);
        for ( int i = k + 1; i < n; ++i ) {
            double v = std::abs(lu[i][k]);
            if ( v > max ) { max = v; p = i; }
        }
        if ( std::abs(max) < VSDK::EPSILON ) {
            freeSquare(lu, n);
            throw MatrixSingularException("Matrix is singular during LU decomposition");
        }
        if ( p != k ) {
            double* tmp = lu[p];
            lu[p] = lu[k];
            lu[k] = tmp;
            pivSign = -pivSign;
        }

        for ( int i = k + 1; i < n; ++i ) {
            lu[i][k] /= lu[k][k];
            for ( int j = k + 1; j < n; ++j ) lu[i][j] -= lu[i][k] * lu[k][j];
        }
    }

    double det = pivSign;
    for ( int i = 0; i < n; ++i ) det *= lu[i][i];
    freeSquare(lu, n);
    return det;
}

MatrixNxM LuCpuStrategy::inverse(const MatrixNxM& matrix) const
{
    GaussCpuStrategy g;
    return g.inverse(matrix);
}

const char* GaussCpuStrategy::id() const { return "gauss-cpu"; }

double GaussCpuStrategy::determinant(const MatrixNxM& matrix) const
{
    requireSquare(matrix);
    int n = matrix.getNumRows();
    double** a = toArraySquare(matrix);
    int sign = 1;

    for ( int k = 0; k < n; ++k ) {
        int pivot = k;
        double max = std::abs(a[k][k]);
        for ( int i = k + 1; i < n; ++i ) {
            double candidate = std::abs(a[i][k]);
            if ( candidate > max ) { max = candidate; pivot = i; }
        }
        if ( std::abs(max) < VSDK::EPSILON ) {
            freeSquare(a, n);
            return 0.0;
        }

        if ( pivot != k ) {
            double* tmp = a[pivot];
            a[pivot] = a[k];
            a[k] = tmp;
            sign = -sign;
        }

        for ( int i = k + 1; i < n; ++i ) {
            double factor = a[i][k] / a[k][k];
            for ( int j = k + 1; j < n; ++j ) a[i][j] -= factor * a[k][j];
            a[i][k] = 0.0;
        }
    }

    double det = sign;
    for ( int i = 0; i < n; ++i ) det *= a[i][i];
    freeSquare(a, n);
    return det;
}

MatrixNxM GaussCpuStrategy::inverse(const MatrixNxM& matrix) const
{
    requireSquare(matrix);
    int n = matrix.getNumRows();
    double** a = toArraySquare(matrix);
    double** inv = allocSquare(n);
    for ( int i = 0; i < n; ++i ) inv[i][i] = 1.0;

    for ( int col = 0; col < n; ++col ) {
        int pivot = col;
        double max = std::abs(a[col][col]);
        for ( int i = col + 1; i < n; ++i ) {
            double candidate = std::abs(a[i][col]);
            if ( candidate > max ) { max = candidate; pivot = i; }
        }

        if ( std::abs(max) < VSDK::EPSILON ) {
            freeSquare(a, n);
            freeSquare(inv, n);
            throw MatrixSingularException("Matrix is singular during Gauss-Jordan elimination");
        }

        if ( pivot != col ) {
            double* tmp = a[pivot]; a[pivot] = a[col]; a[col] = tmp;
            tmp = inv[pivot]; inv[pivot] = inv[col]; inv[col] = tmp;
        }

        double pv = a[col][col];
        for ( int j = 0; j < n; ++j ) {
            a[col][j] /= pv;
            inv[col][j] /= pv;
        }

        for ( int i = 0; i < n; ++i ) {
            if ( i == col ) continue;
            double factor = a[i][col];
            for ( int j = 0; j < n; ++j ) {
                a[i][j] -= factor * a[col][j];
                inv[i][j] -= factor * inv[col][j];
            }
        }
    }

    MatrixNxM out(n, n);
    for ( int i = 0; i < n; ++i ) for ( int j = 0; j < n; ++j ) out = out.withVal(i, j, inv[i][j]);

    freeSquare(a, n);
    freeSquare(inv, n);
    return out;
}

LinearAlgebraEngine::LinearAlgebraEngine(ComputeStrategy strategy) : strategy_(strategy) {}
LinearAlgebraEngine LinearAlgebraEngine::defaultEngine() { return LinearAlgebraEngine(NAIVE_COFACTOR_CPU); }
LinearAlgebraEngine LinearAlgebraEngine::fromStrategy(ComputeStrategy strategy) { return LinearAlgebraEngine(strategy); }

double LinearAlgebraEngine::determinant(const MatrixNxM& m) const
{
    if ( strategy_ == LU_CPU ) { LuCpuStrategy s; return s.determinant(m); }
    if ( strategy_ == GAUSS_CPU ) { GaussCpuStrategy s; return s.determinant(m); }
    NaiveCofactorCpuStrategy s;
    return s.determinant(m);
}

MatrixNxM LinearAlgebraEngine::inverse(const MatrixNxM& m) const
{
    if ( strategy_ == LU_CPU ) { LuCpuStrategy s; return s.inverse(m); }
    if ( strategy_ == GAUSS_CPU ) { GaussCpuStrategy s; return s.inverse(m); }
    NaiveCofactorCpuStrategy s;
    return s.inverse(m);
}

double LinearAlgebraEngine::determinantDefault(const MatrixNxM& m) { return defaultEngine().determinant(m); }
MatrixNxM LinearAlgebraEngine::inverseDefault(const MatrixNxM& m) { return defaultEngine().inverse(m); }
