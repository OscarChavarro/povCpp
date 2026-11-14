#ifndef VSDK_QUADRIC_RAY_EXPANSION_H
#define VSDK_QUADRIC_RAY_EXPANSION_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"

// PovRay 1.0 expansion of a quadric along a ray, Q(o + t d) = a t^2 + b t + c,
// from products of the ray's coordinates that a renderer computes once per
// ray and reuses for every quadric it tests: the componentwise squares o*o,
// d*d and o*d and the mixed products (x y, x z, y z) of o with o, of d with d
// and the symmetric mixed product of o with d. The constant term splits into
// the part without mixed products, which a renderer may cache per viewpoint,
// and the mixed part. Roots come from QuadricSurface::solve. This is the
// evaluation order of PovRay's renderer; QuadricSurface::roots evaluates the
// same polynomial from the ray alone, in another order. The functions are
// inline because renderers call them in their innermost loop.
class QuadricRayExpansion {
public:
    // Mixed products (b.x c.y, b.x c.z, b.y c.z).
    static Vector3Dd mixedProducts(const Vector3Dd& b, const Vector3Dd& c)
    {
        return Vector3Dd(b.x()*c.y(), b.x()*c.z(), b.y()*c.z());
    }

    // Symmetric mixed product of origin and direction, the sum of
    // mixedProducts(direction, origin) and mixedProducts(origin, direction).
    static Vector3Dd symmetricMixedProducts(const Vector3Dd& origin, const Vector3Dd& direction)
    {
        return mixedProducts(direction, origin).add(mixedProducts(origin, direction));
    }

    // a, or zero when the quadric has no square or mixed terms.
    static double squareTerm(const QuadricSurface& quadric, bool hasSquareTerms,
        const Vector3Dd& directionSquares, const Vector3Dd& directionMixed)
    {
        if (!hasSquareTerms) return 0.0;
        double result = quadric.squareCoefficients().dotProduct(directionSquares);
        result += quadric.mixedCoefficients().dotProduct(directionMixed);
        return result;
    }

    static double linearTerm(const QuadricSurface& quadric, const Vector3Dd& originDirection,
        const Vector3Dd& direction, const Vector3Dd& originDirectionMixed)
    {
        double result = quadric.squareCoefficients().dotProduct(originDirection);
        result *= 2.0;
        result += quadric.linearCoefficients().dotProduct(direction);
        result += quadric.mixedCoefficients().dotProduct(originDirectionMixed);
        return result;
    }

    // c without the mixed products of the origin.
    static double constantTermWithoutMixed(const QuadricSurface& quadric,
        const Vector3Dd& originSquares, const Vector3Dd& origin)
    {
        double result = quadric.squareCoefficients().dotProduct(originSquares);
        result += quadric.linearCoefficients().dotProduct(origin);
        result += quadric.constantCoefficient();
        return result;
    }

    // c from its part without mixed products.
    static double constantTerm(const QuadricSurface& quadric, double withoutMixed,
        const Vector3Dd& originMixed)
    {
        return withoutMixed + quadric.mixedCoefficients().dotProduct(originMixed);
    }
};

#endif
