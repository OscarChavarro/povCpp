#ifndef __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIX4X4D_H__
#define __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIX4X4D_H__


#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
#include "java/lang/String.h"

class Matrix4x4d {
    double m_[4][4];

public:
    Matrix4x4d();
    Matrix4x4d(const Matrix4x4d& other);
    explicit Matrix4x4d(const double values[4][4]);

    static Matrix4x4d copyOf(const Matrix4x4d& other) { return Matrix4x4d(other); }
    static Matrix4x4d copyOf(const double values[4][4]) { return Matrix4x4d(values); }
    static Matrix4x4d identityMatrix();

    Matrix4x4d identity() const;
    double get(int row, int col) const;
    Matrix4x4d withVal(int row, int col, double val) const;
    double** toArrayCopy() const;
    Matrix4x4d withoutTranslation() const;
    Vector3Dd extractTranslation() const;
    Matrix4x4d withTranslation(const Vector3Dd& t) const;

    Matrix4x4d orthogonalProjection(double l, double r, double d, double u, double n, double f) const;
    Matrix4x4d canonicalPerspectiveProjection() const;
    Matrix4x4d frustumProjection(double l, double r, double d, double u, double n, double f) const;

    Matrix4x4d translation(double x, double y, double z) const;
    Matrix4x4d scale(double sx, double sy, double sz) const;
    Matrix4x4d scale(const Vector3Dd& s) const;
    Matrix4x4d translation(const Vector3Dd& t) const;

    Matrix4x4d eulerAnglesRotation(double yaw, double pitch, double roll) const;
    Matrix4x4d axisRotation(double angle, const Vector3Dd& axis) const;
    Matrix4x4d axisRotation(double angle, double x, double y, double z) const;

    Matrix4x4d inverse() const;
    Matrix4x4d invert() const;
    Matrix4x4d cofactors() const;
    Matrix4x4d transpose() const;

    Matrix4x4d multiply(double a) const;
    Vector3Dd multiply(const Vector3Dd& e) const;
    Vector4Dd multiply(const Vector4Dd& e) const;
    Matrix4x4d multiply(const Matrix4x4d& second) const;

    double determinant() const;
    java::String* toString() const;
    double* exportToDoubleArrayRowOrder() const;
    float* exportToFloatArrayRowOrder() const;
    double* exportToDoubleArrayColumnOrder() const;
    float* exportToFloatArrayColumnOrder() const;

    Quaterniond exportToQuaternion() const;
    Matrix4x4d importFromQuaternion(const Quaterniond& q) const;

    double obtainEulerYawAngle() const;
    double obtainEulerPitchAngle() const;
    double obtainEulerRollAngle() const;

    bool epsilonEquals(const Matrix4x4d& other) const { return epsilonEquals(other, 1e-6); }
    bool epsilonEquals(const Matrix4x4d& other, double epsilon) const;

    bool operator==(const Matrix4x4d& other) const;
    bool equals(const Matrix4x4d& other) const { return (*this) == other; }
    int hashCode() const;
};


#endif // __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIX4X4D_H__
