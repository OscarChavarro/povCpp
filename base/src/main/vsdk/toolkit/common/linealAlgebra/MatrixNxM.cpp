#include "vsdk/toolkit/common/linealAlgebra/MatrixNxM.h"

#include <cmath>
#include <cstring>
#include <cstdio>

#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/linealAlgebra/processing/LinearAlgebraEngine.h"
#include "vsdk/toolkit/common/linealAlgebra/exceptions/MatrixExceptions.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

int MatrixNxM::index(int row, int column) const
{
    return row * numColumns_ + column;
}

MatrixNxM::MatrixNxM(int n, int m)
{
    if ( n <= 0 || m <= 0 ) {
        throw MatrixDimensionMismatchException("Invalid matrix size: rows and columns must be > 0");
    }
    numRows_ = n;
    numColumns_ = m;
    m_ = new double[numRows_ * numColumns_];
    for ( int i = 0; i < numRows_ * numColumns_; ++i ) m_[i] = 0.0;
    for ( int i = 0; i < numRows_ && i < numColumns_; ++i ) m_[index(i, i)] = 1.0;
}

MatrixNxM::MatrixNxM(const MatrixNxM& other)
{
    numRows_ = other.numRows_;
    numColumns_ = other.numColumns_;
    m_ = new double[numRows_ * numColumns_];
    for ( int i = 0; i < numRows_ * numColumns_; ++i ) m_[i] = other.m_[i];
}

MatrixNxM::~MatrixNxM()
{
    delete[] m_;
    m_ = 0;
}

MatrixNxM& MatrixNxM::operator=(const MatrixNxM& other)
{
    if ( this == &other ) return *this;
    double* replacement = new double[other.numRows_ * other.numColumns_];
    for ( int i = 0; i < other.numRows_ * other.numColumns_; ++i ) replacement[i] = other.m_[i];
    delete[] m_;
    m_ = replacement;
    numRows_ = other.numRows_;
    numColumns_ = other.numColumns_;
    return *this;
}

MatrixNxM MatrixNxM::copyOf(const MatrixNxM& other)
{
    return MatrixNxM(other);
}

MatrixNxM MatrixNxM::identity() const
{
    return MatrixNxM(numRows_, numColumns_);
}

int MatrixNxM::getNumRows() const { return numRows_; }
int MatrixNxM::getNumColumns() const { return numColumns_; }

double MatrixNxM::getVal(int row, int column) const
{
    if ( row < 0 || row >= numRows_ || column < 0 || column >= numColumns_ ) {
        throw MatrixIndexOutOfBoundsException("Invalid matrix position");
    }
    return m_[index(row, column)];
}

MatrixNxM MatrixNxM::withVal(int row, int column, double val) const
{
    if ( row < 0 || row >= numRows_ || column < 0 || column >= numColumns_ ) {
        throw MatrixIndexOutOfBoundsException("Invalid matrix position");
    }
    MatrixNxM r(*this);
    r.m_[r.index(row, column)] = val;
    return r;
}

MatrixNxM MatrixNxM::inverse() const { return LinearAlgebraEngine::inverseDefault(*this); }

MatrixNxM MatrixNxM::cofactors() const
{
    MatrixNxM r(numRows_, numColumns_);
    for ( int row = 0; row < numRows_; row++ ) {
        for ( int column = 0; column < numColumns_; column++ ) {
            MatrixNxM minor = buildMinor(row, column);
            double sign = ((row + column) % 2 == 0) ? 1.0 : -1.0;
            r.m_[r.index(row, column)] = sign * minor.determinant();
        }
    }
    return r;
}

MatrixNxM MatrixNxM::transpose() const
{
    MatrixNxM r(numColumns_, numRows_);
    for ( int row = 0; row < numRows_; row++ ) {
        for ( int column = 0; column < numColumns_; column++ ) {
            r.m_[r.index(column, row)] = m_[index(row, column)];
        }
    }
    return r;
}

MatrixNxM MatrixNxM::multiply(double a) const
{
    MatrixNxM r(numRows_, numColumns_);
    for ( int row = 0; row < numRows_; row++ ) {
        for ( int column = 0; column < numColumns_; column++ ) {
            r.m_[r.index(row, column)] = a * m_[index(row, column)];
        }
    }
    return r;
}

MatrixNxM MatrixNxM::multiply(const MatrixNxM& other) const
{
    if ( numColumns_ != other.numRows_ ) {
        throw MatrixDimensionMismatchException("When multiplying matrices, first operand number of columns must match second operand number of rows.");
    }

    MatrixNxM r(numRows_, other.numColumns_);
    for ( int rowA = 0; rowA < numRows_; rowA++ ) {
        for ( int columnB = 0; columnB < other.numColumns_; columnB++ ) {
            double accum = 0;
            for ( int rowB = 0; rowB < numColumns_; rowB++ ) {
                accum += m_[index(rowA, rowB)] * other.m_[other.index(rowB, columnB)];
            }
            r.m_[r.index(rowA, columnB)] = accum;
        }
    }
    return r;
}

MatrixNxM MatrixNxM::buildMinor(int row, int column) const
{
    if ( numColumns_ <= 1 || numRows_ <= 1 ) {
        throw MatrixDimensionMismatchException("Matrix must be at least of size 2x2 to have a minor matrix");
    }
    if ( row < 0 || row >= numRows_ || column < 0 || column >= numColumns_ ) {
        throw MatrixIndexOutOfBoundsException("Invalid matrix position");
    }

    MatrixNxM minor(numRows_ - 1, numColumns_ - 1);
    for ( int r1 = 0, r2 = 0; r1 < numRows_; r1++ ) {
        if ( r1 == row ) continue;
        for ( int c1 = 0, c2 = 0; c1 < numColumns_; c1++ ) {
            if ( c1 == column ) continue;
            minor.m_[minor.index(r2, c2)] = m_[index(r1, c1)];
            c2++;
        }
        r2++;
    }

    return minor;
}

double MatrixNxM::determinant() const { return LinearAlgebraEngine::determinantDefault(*this); }

java::String* MatrixNxM::toString() const
{
    char msg[16384];
    int pos = 0;
    pos += std::snprintf(msg + pos, sizeof(msg) - pos, "\n------------------------------\n");
    pos += std::snprintf(msg + pos, sizeof(msg) - pos, "  - Matrix of %d rows by %d columns\n", numRows_, numColumns_);
    for ( int row = 0; row < numRows_; row++ ) {
        for ( int column = 0; column < numColumns_; column++ ) {
            pos += std::snprintf(msg + pos, sizeof(msg) - pos, "%s ", VSDK::formatDouble(m_[index(row, column)]).c_str());
        }
        pos += std::snprintf(msg + pos, sizeof(msg) - pos, "\n");
    }
    std::snprintf(msg + pos, sizeof(msg) - pos, "------------------------------\n");
    return new java::String(msg);
}

bool MatrixNxM::epsilonEquals(const MatrixNxM& other) const { return epsilonEquals(other, 1e-6); }

bool MatrixNxM::epsilonEquals(const MatrixNxM& other, double epsilon) const
{
    if ( epsilon < 0.0 ) {
        Logger::reportMessage("MatrixNxM", Logger::ERROR, "epsilonEquals", "epsilon must be >= 0");
        throw VSDKFatalException("epsilon must be >= 0");
    }
    if ( numRows_ != other.numRows_ || numColumns_ != other.numColumns_ ) return false;
    for ( int i = 0; i < numRows_ * numColumns_; ++i ) {
        if ( std::abs(m_[i] - other.m_[i]) > epsilon ) return false;
    }
    return true;
}

bool MatrixNxM::operator==(const MatrixNxM& other) const
{
    if ( numRows_ != other.numRows_ || numColumns_ != other.numColumns_ ) return false;
    for ( int i = 0; i < numRows_ * numColumns_; ++i ) if ( m_[i] != other.m_[i] ) return false;
    return true;
}

static unsigned int hashDouble(double val) {
    unsigned char bytes[sizeof(double)];
    memcpy(bytes, &val, sizeof(double));
    unsigned int h = 0u;
    for (int i = 0; i < (int)sizeof(double); ++i)
        h = h * 31u + static_cast<unsigned int>(bytes[i]);
    return h;
}

int MatrixNxM::hashCode() const
{
    unsigned int result = 31u * (unsigned int)numRows_ + (unsigned int)numColumns_;
    for ( int i = 0; i < numRows_ * numColumns_; ++i ) {
        result = 31u * result + hashDouble(m_[i]);
    }
    return (int)result;
}
