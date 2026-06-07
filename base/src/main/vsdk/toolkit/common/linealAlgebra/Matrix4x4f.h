#ifndef __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIX4X4F_H__
#define __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIX4X4F_H__


#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Df.h"
#include "java/lang/String.h"

class Matrix4x4f {
    Matrix4x4d d_;

public:
    Matrix4x4f() : d_() {}
    Matrix4x4f(const Matrix4x4f& other) : d_(other.d_) {}
    explicit Matrix4x4f(const float values[4][4]);

    static Matrix4x4f copyOf(const Matrix4x4f& other) { return Matrix4x4f(other); }
    static Matrix4x4f copyOf(const float values[4][4]) { return Matrix4x4f(values); }
    static Matrix4x4f identityMatrix() { return Matrix4x4f(); }

    Matrix4x4f identity() const { return identityMatrix(); }
    float get(int row, int col) const { return (float)d_.get(row, col); }
    float** toArrayCopy() const;
    Matrix4x4f withVal(int row, int col, float val) const;
    Matrix4x4f withoutTranslation() const;
    Vector3Df extractTranslation() const;
    Matrix4x4f withTranslation(const Vector3Df& t) const;

    Matrix4x4f orthogonalProjection(float l, float r, float d, float u, float n, float f) const;
    Matrix4x4f canonicalPerspectiveProjection() const;
    Matrix4x4f frustumProjection(float l, float r, float d, float u, float n, float f) const;

    Matrix4x4f translation(float x, float y, float z) const;
    Matrix4x4f scale(float sx, float sy, float sz) const;
    Matrix4x4f scale(const Vector3Df& s) const { return scale(s.x(), s.y(), s.z()); }
    Matrix4x4f translation(const Vector3Df& t) const { return translation(t.x(), t.y(), t.z()); }

    Matrix4x4f eulerAnglesRotation(float yaw, float pitch, float roll) const;
    Matrix4x4f axisRotation(float angle, const Vector3Df& axis) const;
    Matrix4x4f axisRotation(float angle, float x, float y, float z) const;

    Matrix4x4f inverse() const;
    Matrix4x4f invert() const;
    Matrix4x4f cofactors() const;
    Matrix4x4f transpose() const;

    Matrix4x4f multiply(float a) const;
    Vector3Df multiply(const Vector3Df& e) const;
    Vector4Df multiply(const Vector4Df& e) const;
    Matrix4x4f multiply(const Matrix4x4f& second) const;
    float determinant() const;
    java::String* toString() const;
    double* exportToDoubleArrayRowOrder() const;
    float* exportToFloatArrayRowOrder() const;
    double* exportToDoubleArrayColumnOrder() const;
    float* exportToFloatArrayColumnOrder() const;

    Quaterniond exportToQuaternion() const;
    Matrix4x4f importFromQuaternion(const Quaterniond& q) const;

    float obtainEulerYawAngle() const;
    float obtainEulerPitchAngle() const;
    float obtainEulerRollAngle() const;

    bool epsilonEquals(const Matrix4x4f& other) const { return epsilonEquals(other, 1e-6f); }
    bool epsilonEquals(const Matrix4x4f& other, float eps) const;

    bool operator==(const Matrix4x4f& other) const { return d_ == other.d_; }
    bool equals(const Matrix4x4f& other) const { return (*this) == other; }
    int hashCode() const;
};


#endif // __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_MATRIX4X4F_H__
