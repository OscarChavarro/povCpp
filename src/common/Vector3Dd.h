#ifndef __VECTOR3DD_H__
#define __VECTOR3DD_H__

#include "common/FrameConfig.h"
#include <cmath>

extern double VTemp;

class Vector3Dd {
  public:
    double x;
    double y;
    double z;
};

class VectorOps {
  public:
    static inline void vAdd(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.x + c.x;
        a.y = b.y + c.y;
        a.z = b.z + c.z;
    }

    static inline void vSub(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.x - c.x;
        a.y = b.y - c.y;
        a.z = b.z - c.z;
    }

    static inline void vScale(Vector3Dd &a, const Vector3Dd &b, double k)
    {
        a.x = b.x * k;
        a.y = b.y * k;
        a.z = b.z * k;
    }

    static inline void vInverseScale(Vector3Dd &a, const Vector3Dd &b, double k)
    {
        a.x = b.x / k;
        a.y = b.y / k;
        a.z = b.z / k;
    }

    static inline void vDot(double &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a = b.x * c.x + b.y * c.y + b.z * c.z;
    }

    static inline void vCross(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.y * c.z - b.z * c.y;
        a.y = b.z * c.x - b.x * c.z;
        a.z = b.x * c.y - b.y * c.x;
    }

    static inline void vEvaluate(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = b.x * c.x;
        a.y = b.y * c.y;
        a.z = b.z * c.z;
    }

    static inline double vSqr(const Vector3Dd &a)
    {
        return a.x * a.x + a.y * a.y + a.z * a.z;
    }

    static inline double sqr(double a)
    {
        return a * a;
    }

    static inline void vSquareTerms(Vector3Dd &a, const Vector3Dd &b)
    {
        a.x = b.x * b.x;
        a.y = b.y * b.y;
        a.z = b.z * b.z;
    }

    static inline void vLength(double &a, const Vector3Dd &b)
    {
        a = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
    }

    static inline void vNormalize(Vector3Dd &a, const Vector3Dd &b)
    {
        VTemp = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
        a.x = b.x / VTemp;
        a.y = b.y / VTemp;
        a.z = b.z / VTemp;
    }

    static inline void vHalf(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a.x = 0.5 * (b.x + c.x);
        a.y = 0.5 * (b.y + c.y);
        a.z = 0.5 * (b.z + c.z);
    }

    static inline void makeVector(Vector3Dd *v, double a, double b, double c)
    {
        v->x = a;
        v->y = b;
        v->z = c;
    }
};

#endif
