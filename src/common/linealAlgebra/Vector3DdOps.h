#ifndef __VECTOR3DD_OPS_H__
#define __VECTOR3DD_OPS_H__

#include <cmath>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Bit-exact reconstructions of the legacy mutating ops, returning the
// immutable Vitral Vector3Dd. Arithmetic order matches the original
// src/common/linealAlgebra/Vector3Dd.h so golden images stay byte-identical.
namespace Vec3 {

inline Vector3Dd scaled(const Vector3Dd &b, double k) {
    return Vector3Dd(b.x() * k, b.y() * k, b.z() * k);
}
inline Vector3Dd inverseScaled(const Vector3Dd &b, double k) {
    return Vector3Dd(b.x() / k, b.y() / k, b.z() / k);
}
inline Vector3Dd normalized(const Vector3Dd &b) {
    const double vTemp = std::sqrt(b.x()*b.x() + b.y()*b.y() + b.z()*b.z());
    return Vector3Dd(b.x() / vTemp, b.y() / vTemp, b.z() / vTemp);
}
inline Vector3Dd squareTerms(const Vector3Dd &b) {
    return Vector3Dd(b.x()*b.x(), b.y()*b.y(), b.z()*b.z());
}
inline Vector3Dd evaluated(const Vector3Dd &b, const Vector3Dd &c) {
    return Vector3Dd(b.x()*c.x(), b.y()*c.y(), b.z()*c.z());
}
inline Vector3Dd half(const Vector3Dd &b, const Vector3Dd &c) {
    return Vector3Dd(0.5*(b.x()+c.x()), 0.5*(b.y()+c.y()), 0.5*(b.z()+c.z()));
}
inline double sqr(double a) { return a * a; }

} // namespace Vec3

#endif
