#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
#include "java/lang/String.h"

Matrix4x4f::Matrix4x4f(const float values[4][4])
{
    double dvals[4][4];
    for ( int i = 0; i < 4; ++i ) for ( int j = 0; j < 4; ++j ) dvals[i][j] = (double)values[i][j];
    d_ = Matrix4x4d(dvals);
}

Matrix4x4f Matrix4x4f::withVal(int row, int col, float val) const { Matrix4x4f o; o.d_ = d_.withVal(row, col, val); return o; }
float** Matrix4x4f::toArrayCopy() const
{
    float** out = new float*[4];
    for ( int i = 0; i < 4; ++i ) {
        out[i] = new float[4];
        for ( int j = 0; j < 4; ++j ) out[i][j] = get(i, j);
    }
    return out;
}
Matrix4x4f Matrix4x4f::withoutTranslation() const { Matrix4x4f o; o.d_ = d_.withoutTranslation(); return o; }
Vector3Df Matrix4x4f::extractTranslation() const { return Vector3Df(d_.extractTranslation()); }
Matrix4x4f Matrix4x4f::withTranslation(const Vector3Df& t) const { Matrix4x4f o; o.d_ = d_.withTranslation(Vector3Dd(t.x(), t.y(), t.z())); return o; }

Matrix4x4f Matrix4x4f::orthogonalProjection(float l, float r, float d, float u, float n, float f) const { Matrix4x4f o; o.d_ = d_.orthogonalProjection(l, r, d, u, n, f); return o; }
Matrix4x4f Matrix4x4f::canonicalPerspectiveProjection() const { Matrix4x4f o; o.d_ = d_.canonicalPerspectiveProjection(); return o; }
Matrix4x4f Matrix4x4f::frustumProjection(float l, float r, float d, float u, float n, float f) const { Matrix4x4f o; o.d_ = d_.frustumProjection(l, r, d, u, n, f); return o; }

Matrix4x4f Matrix4x4f::translation(float x, float y, float z) const { Matrix4x4f o; o.d_ = d_.translation(x, y, z); return o; }
Matrix4x4f Matrix4x4f::scale(float sx, float sy, float sz) const { Matrix4x4f o; o.d_ = d_.scale(sx, sy, sz); return o; }

Matrix4x4f Matrix4x4f::eulerAnglesRotation(float yaw, float pitch, float roll) const { Matrix4x4f o; o.d_ = d_.eulerAnglesRotation(yaw, pitch, roll); return o; }
Matrix4x4f Matrix4x4f::axisRotation(float angle, const Vector3Df& axis) const { return axisRotation(angle, axis.x(), axis.y(), axis.z()); }
Matrix4x4f Matrix4x4f::axisRotation(float angle, float x, float y, float z) const { Matrix4x4f o; o.d_ = d_.axisRotation(angle, x, y, z); return o; }

Matrix4x4f Matrix4x4f::inverse() const { return invert(); }
Matrix4x4f Matrix4x4f::invert() const { Matrix4x4f o; o.d_ = d_.invert(); return o; }
Matrix4x4f Matrix4x4f::cofactors() const { Matrix4x4f o; o.d_ = d_.cofactors(); return o; }
Matrix4x4f Matrix4x4f::transpose() const { Matrix4x4f o; o.d_ = d_.transpose(); return o; }

Matrix4x4f Matrix4x4f::multiply(float a) const { Matrix4x4f o; o.d_ = d_.multiply(a); return o; }
Vector3Df Matrix4x4f::multiply(const Vector3Df& e) const { return Vector3Df(d_.multiply(Vector3Dd(e.x(), e.y(), e.z()))); }
Vector4Df Matrix4x4f::multiply(const Vector4Df& e) const { return Vector4Df(d_.multiply(Vector4Dd(e.x(), e.y(), e.z(), e.w()))); }
Matrix4x4f Matrix4x4f::multiply(const Matrix4x4f& second) const { Matrix4x4f o; o.d_ = d_.multiply(second.d_); return o; }
float Matrix4x4f::determinant() const { return (float)d_.determinant(); }

Quaterniond Matrix4x4f::exportToQuaternion() const { return d_.exportToQuaternion(); }
Matrix4x4f Matrix4x4f::importFromQuaternion(const Quaterniond& q) const { Matrix4x4f o; o.d_ = d_.importFromQuaternion(q); return o; }

float Matrix4x4f::obtainEulerYawAngle() const { return (float)d_.obtainEulerYawAngle(); }
float Matrix4x4f::obtainEulerPitchAngle() const { return (float)d_.obtainEulerPitchAngle(); }
float Matrix4x4f::obtainEulerRollAngle() const { return (float)d_.obtainEulerRollAngle(); }

bool Matrix4x4f::epsilonEquals(const Matrix4x4f& other, float eps) const { return d_.epsilonEquals(other.d_, eps); }
java::String* Matrix4x4f::toString() const { return d_.toString(); }
double* Matrix4x4f::exportToDoubleArrayRowOrder() const { return d_.exportToDoubleArrayRowOrder(); }
float* Matrix4x4f::exportToFloatArrayRowOrder() const { return d_.exportToFloatArrayRowOrder(); }
double* Matrix4x4f::exportToDoubleArrayColumnOrder() const { return d_.exportToDoubleArrayColumnOrder(); }
float* Matrix4x4f::exportToFloatArrayColumnOrder() const { return d_.exportToFloatArrayColumnOrder(); }

int Matrix4x4f::hashCode() const
{
    return d_.hashCode();
}
