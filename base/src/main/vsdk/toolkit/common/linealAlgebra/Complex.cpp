#include "vsdk/toolkit/common/linealAlgebra/Complex.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include <cmath>
#include <cstdio>

double Complex::abs() const { return std::hypot(r, i); }
double Complex::phase() const { return std::atan2(i, r); }
Complex Complex::plus(const Complex& b) const { return Complex(r + b.r, i + b.i); }
Complex Complex::minus(const Complex& b) const { return Complex(r - b.r, i - b.i); }
Complex Complex::times(const Complex& b) const { return Complex(r * b.r - i * b.i, r * b.i + i * b.r); }
Complex Complex::times(double alpha) const { return Complex(alpha * r, alpha * i); }
Complex Complex::conjugate() const { return Complex(r, -i); }
Complex Complex::reciprocal() const { double s = r * r + i * i; return Complex(r / s, -i / s); }
Complex Complex::divides(const Complex& b) const { return times(b.reciprocal()); }
Complex Complex::exp() const { return Complex(std::exp(r) * std::cos(i), std::exp(r) * std::sin(i)); }
Complex Complex::sin() const { return Complex(std::sin(r) * std::cosh(i), std::cos(r) * std::sinh(i)); }
Complex Complex::cos() const { return Complex(std::cos(r) * std::cosh(i), -std::sin(r) * std::sinh(i)); }
Complex Complex::tan() const { return sin().divides(cos()); }
Complex Complex::plus(const Complex& a, const Complex& b) { return Complex(a.r + b.r, a.i + b.i); }

java::String* Complex::toString() const
{
    char buf[256];
    if ( i < 0 ) {
        std::snprintf(buf, sizeof(buf), "%s - %si", VSDK::formatDouble(r).c_str(), VSDK::formatDouble(-i).c_str());
        return new java::String(buf);
    }
    std::snprintf(buf, sizeof(buf), "%s + %si", VSDK::formatDouble(r).c_str(), VSDK::formatDouble(i).c_str());
    return new java::String(buf);
}
