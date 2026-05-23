#ifndef __VECTOR3DD_H__
#define __VECTOR3DD_H__

#include "common/LegacyBoolean.h"
#include <cmath>

extern double VTemp;

class Vector3Dd {
  public:
    double x;
    double y;
    double z;

    Vector3Dd() {}
    Vector3Dd(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    inline double
    length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    inline double
    sqr() const
    {
        return x * x + y * y + z * z;
    }

    inline void
    normalize()
    {
        VTemp = std::sqrt(x * x + y * y + z * z);
        x = x / VTemp;
        y = y / VTemp;
        z = z / VTemp;
    }

    inline void
    scale(double k)
    {
        x = x * k;
        y = y * k;
        z = z * k;
    }

    inline void
    inverseScale(double k)
    {
        x = x / k;
        y = y / k;
        z = z / k;
    }

    inline void
    squareTerms()
    {
        x = x * x;
        y = y * y;
        z = z * z;
    }

    inline void
    add(const Vector3Dd &b)
    {
        x = x + b.x;
        y = y + b.y;
        z = z + b.z;
    }

    inline void
    sub(const Vector3Dd &b)
    {
        x = x - b.x;
        y = y - b.y;
        z = z - b.z;
    }

    inline void
    evaluate(const Vector3Dd &b)
    {
        x = x * b.x;
        y = y * b.y;
        z = z * b.z;
    }

    inline Vector3Dd
    crossProduct(const Vector3Dd &b) const
    {
        return Vector3Dd(
            y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
    }

    inline double
    dotProduct(const Vector3Dd &b) const
    {
        return x * b.x + y * b.y + z * b.z;
    }

    inline Vector3Dd
    half(const Vector3Dd &b) const
    {
        return Vector3Dd(0.5 * (x + b.x), 0.5 * (y + b.y), 0.5 * (z + b.z));
    }
};

class VectorOps {
  public:
    static inline void
    vAdd(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.x + c.x;
        a.y = b.y + c.y;
        a.z = b.z + c.z;
    }

    static inline void
    vSub(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.x - c.x;
        a.y = b.y - c.y;
        a.z = b.z - c.z;
    }

    static inline void
    vScale(Vector3Dd &a, const Vector3Dd &b, double k)
    {
        a.x = b.x * k;
        a.y = b.y * k;
        a.z = b.z * k;
    }

    static inline void
    vInverseScale(Vector3Dd &a, const Vector3Dd &b, double k)
    {
        a.x = b.x / k;
        a.y = b.y / k;
        a.z = b.z / k;
    }

    static inline void
    vDot(double &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a = b.x * c.x + b.y * c.y + b.z * c.z;
    }

    static inline void
    vCross(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.y * c.z - b.z * c.y;
        a.y = b.z * c.x - b.x * c.z;
        a.z = b.x * c.y - b.y * c.x;
    }

    static inline void
    vEvaluate(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.x * c.x;
        a.y = b.y * c.y;
        a.z = b.z * c.z;
    }

    static inline double
    sqr(double a)
    {
        return a * a;
    }

    static inline void
    vSquareTerms(Vector3Dd &a, const Vector3Dd &b)
    {
        a.x = b.x * b.x;
        a.y = b.y * b.y;
        a.z = b.z * b.z;
    }

    static inline void
    vNormalize(Vector3Dd &a, const Vector3Dd &b)
    {
        VTemp = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
        a.x = b.x / VTemp;
        a.y = b.y / VTemp;
        a.z = b.z / VTemp;
    }

    static inline void
    vHalf(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = 0.5 * (b.x + c.x);
        a.y = 0.5 * (b.y + c.y);
        a.z = 0.5 * (b.z + c.z);
    }

    static inline void
    makeVector(Vector3Dd *v, double a, double b, double c)
    {
        v->x = a;
        v->y = b;
        v->z = c;
    }
};

#endif
