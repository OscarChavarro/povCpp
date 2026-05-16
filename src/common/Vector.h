#ifndef __VECTOR_H__
#define __VECTOR_H__
/****************************************************************************
 *                     vector.h
 *
 *  This module contains macros to perform operations on vectors.
 *
 *****************************************************************************/

#include "common/Frame.h"

class Vector3D {
  public:
    DBL x, y, z;
};

/* Misc. Vector Math Macro Definitions */

extern DBL VTemp;

inline void VAdd(Vector3D &a, const Vector3D &b, const Vector3D &c)
{
    a.x = b.x + c.x;
    a.y = b.y + c.y;
    a.z = b.z + c.z;
}

inline void VSub(Vector3D &a, const Vector3D &b, const Vector3D &c)
{
    a.x = b.x - c.x;
    a.y = b.y - c.y;
    a.z = b.z - c.z;
}

inline void VScale(Vector3D &a, const Vector3D &b, DBL k)
{
    a.x = b.x * k;
    a.y = b.y * k;
    a.z = b.z * k;
}

inline void VInverseScale(Vector3D &a, const Vector3D &b, DBL k)
{
    a.x = b.x / k;
    a.y = b.y / k;
    a.z = b.z / k;
}

inline void VDot(DBL &a, const Vector3D &b, const Vector3D &c)
{
    a = b.x * c.x + b.y * c.y + b.z * c.z;
}

/* Cross Product - returns Vector (a) = (b) x (c)
    WARNING:  a must be different from b and c.*/
inline void VCross(Vector3D &a, const Vector3D &b, const Vector3D &c)
{
    a.x = b.y * c.z - b.z * c.y;
    a.y = b.z * c.x - b.x * c.z;
    a.z = b.x * c.y - b.y * c.x;
}

/* Evaluate - returns Vector (a) = Multiply Vector (b) by Vector (c) */
inline void VEvaluate(Vector3D &a, const Vector3D &b, const Vector3D &c)
{
    a.x = b.x * c.x;
    a.y = b.y * c.y;
    a.z = b.z * c.z;
}

/* Square a Vector */
inline DBL VSqr(const Vector3D &a)
{
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

/* Simple Scalar Square Macro */
inline DBL Sqr(DBL a)
{
    return a * a;
}

/* Square a Vector (b) and Assign to another Vector (a) */
inline void VSquareTerms(Vector3D &a, const Vector3D &b)
{
    a.x = b.x * b.x;
    a.y = b.y * b.y;
    a.z = b.z * b.z;
}

/* Vector Length - returs Scalar Euclidean Length (a) of Vector (b) */
inline void VLength(DBL &a, const Vector3D &b)
{
    a = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
}

/* Normalize a Vector - returns a vector (length of 1) that points at (b) */
inline void VNormalize(Vector3D &a, const Vector3D &b)
{
    VTemp = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
    a.x = b.x / VTemp;
    a.y = b.y / VTemp;
    a.z = b.z / VTemp;
}

/* Compute a Vector (a) Halfway Between Two Given Vectors (b) and (c) */
inline void VHalf(Vector3D &a, const Vector3D &b, const Vector3D &c)
{
    a.x = 0.5 * (b.x + c.x);
    a.y = 0.5 * (b.y + c.y);
    a.z = 0.5 * (b.z + c.z);
}

inline void makeVector(Vector3D *v, DBL a, DBL b, DBL c)
{
    v->x = a;
    v->y = b;
    v->z = c;
}

#endif
