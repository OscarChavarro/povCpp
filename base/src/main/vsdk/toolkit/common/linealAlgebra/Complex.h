#ifndef __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_COMPLEX_H__
#define __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_COMPLEX_H__

#include "java/lang/String.h"

class Complex {
public:
    double r;
    double i;

    Complex(double real, double imag) : r(real), i(imag) {}

    double abs() const;
    double phase() const;
    Complex plus(const Complex& b) const;
    Complex minus(const Complex& b) const;
    Complex times(const Complex& b) const;
    Complex times(double alpha) const;
    Complex conjugate() const;
    Complex reciprocal() const;
    Complex divides(const Complex& b) const;
    Complex exp() const;
    Complex sin() const;
    Complex cos() const;
    Complex tan() const;
    static Complex plus(const Complex& a, const Complex& b);
    java::String* toString() const;
};

#endif // __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_COMPLEX_H__
