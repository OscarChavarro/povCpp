#ifndef __VECTOR_OPS_H__
#define __VECTOR_OPS_H__

#include "common/Vector3D.h"

extern double VTemp;

class VectorOps {
  public:
    static inline void vAdd(Vector3D &a, const Vector3D &b, const Vector3D &c)
    {
        a.x = b.x + c.x;
        a.y = b.y + c.y;
        a.z = b.z + c.z;
    }

    static inline void vSub(Vector3D &a, const Vector3D &b, const Vector3D &c)
    {
        a.x = b.x - c.x;
        a.y = b.y - c.y;
        a.z = b.z - c.z;
    }

    static inline void vScale(Vector3D &a, const Vector3D &b, double k)
    {
        a.x = b.x * k;
        a.y = b.y * k;
        a.z = b.z * k;
    }

    static inline void vInverseScale(Vector3D &a, const Vector3D &b, double k)
    {
        a.x = b.x / k;
        a.y = b.y / k;
        a.z = b.z / k;
    }

    static inline void vDot(double &a, const Vector3D &b, const Vector3D &c)
    {
        a = b.x * c.x + b.y * c.y + b.z * c.z;
    }

    static inline void vCross(Vector3D &a, const Vector3D &b, const Vector3D &c)
    {
        a.x = b.y * c.z - b.z * c.y;
        a.y = b.z * c.x - b.x * c.z;
        a.z = b.x * c.y - b.y * c.x;
    }

    static inline void vEvaluate(Vector3D &a, const Vector3D &b, const Vector3D &c)
    {
        a.x = b.x * c.x;
        a.y = b.y * c.y;
        a.z = b.z * c.z;
    }

    static inline double vSqr(const Vector3D &a)
    {
        return a.x * a.x + a.y * a.y + a.z * a.z;
    }

    static inline double sqr(double a)
    {
        return a * a;
    }

    static inline void vSquareTerms(Vector3D &a, const Vector3D &b)
    {
        a.x = b.x * b.x;
        a.y = b.y * b.y;
        a.z = b.z * b.z;
    }

    static inline void vLength(double &a, const Vector3D &b)
    {
        a = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
    }

    static inline void vNormalize(Vector3D &a, const Vector3D &b)
    {
        VTemp = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
        a.x = b.x / VTemp;
        a.y = b.y / VTemp;
        a.z = b.z / VTemp;
    }

    static inline void vHalf(Vector3D &a, const Vector3D &b, const Vector3D &c)
    {
        a.x = 0.5 * (b.x + c.x);
        a.y = 0.5 * (b.y + c.y);
        a.z = 0.5 * (b.z + c.z);
    }

    static inline void makeVector(Vector3D *v, double a, double b, double c)
    {
        v->x = a;
        v->y = b;
        v->z = c;
    }
};

#endif
