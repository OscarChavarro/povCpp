#ifndef __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_VECTOR3DD_H__
#define __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_VECTOR3DD_H__


#include <cmath>
#include <cstring>
#include "java/lang/String.h"

class Vector3Dd {
    double x_;
    double y_;
    double z_;

    static unsigned int hashDouble(double val) {
        unsigned char bytes[sizeof(double)];
        memcpy(bytes, &val, sizeof(double));
        unsigned int h = 0u;
        for (int i = 0; i < (int)sizeof(double); ++i)
            h = h * 31u + static_cast<unsigned int>(bytes[i]);
        return h;
    }

public:
    Vector3Dd() : x_(0.0), y_(0.0), z_(0.0) {}
    Vector3Dd(double x, double y, double z) : x_(x), y_(y), z_(z) {}
    Vector3Dd(const Vector3Dd& other) : x_(other.x_), y_(other.y_), z_(other.z_) {}

    static Vector3Dd copyOf(const Vector3Dd& other) { return Vector3Dd(other); }

    Vector3Dd multiply(double a) const { return Vector3Dd(a * x_, a * y_, a * z_); }
    Vector3Dd crossProduct(const Vector3Dd& other) const;
    double dotProduct(const Vector3Dd& other) const;
    Vector3Dd normalized() const;
    double length() const;
    Vector3Dd add(const Vector3Dd& b) const;
    Vector3Dd subtract(const Vector3Dd& b) const;
    float* exportToFloatArrayVector() const;
    double obtainSphericalThetaAngle() const;
    double obtainSphericalPhiAngle() const;
    static Vector3Dd fromSpherical(double r, double theta, double phi);

    Vector3Dd withX(double nx) const { return Vector3Dd(nx, y_, z_); }
    Vector3Dd withY(double ny) const { return Vector3Dd(x_, ny, z_); }
    Vector3Dd withZ(double nz) const { return Vector3Dd(x_, y_, nz); }
    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }

    bool epsilonEquals(const Vector3Dd& other) const { return epsilonEquals(other, 1e-6); }
    bool epsilonEquals(const Vector3Dd& other, double epsilon) const;

    bool operator==(const Vector3Dd& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; }
    bool equals(const Vector3Dd& other) const { return (*this) == other; }
    int hashCode() const {
        unsigned int result = hashDouble(x_);
        result = 31u * result + hashDouble(y_);
        result = 31u * result + hashDouble(z_);
        return (int)result;
    }
    java::String* toString() const;
};


#endif // __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_VECTOR3DD_H__
