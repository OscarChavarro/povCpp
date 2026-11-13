#ifndef VSDK_CSG_SOLID_EQUIVALENCE_H
#define VSDK_CSG_SOLID_EQUIVALENCE_H
class SimpleBody;
class Matrix4x4d;
class Vector3Dd;
class Box;
class Sphere;
class PlanarHalfSpace;
class QuadricSurface;
// Conservative, exact sufficient conditions for equality of toolkit solids.
class CsgSolidEquivalence {
private:
    static bool sameEllipsoidLinear(const Matrix4x4d& a, double radiusA,
        const Matrix4x4d& b, double radiusB);
    static bool sameTranslation(const Matrix4x4d& a, const Matrix4x4d& b);
    static bool signedPermutation(const Matrix4x4d& a, const Matrix4x4d& b,
        int* axes, int* signs);
    static bool proportional(const double* a, const double* b, int count);
    static bool sameBox(const Box& a, const Matrix4x4d& pa, const Box& b, const Matrix4x4d& pb);
    static bool axisPlane(const double* coefficients, const Matrix4x4d& placement,
        int& axis, double& scale, double& offset, double& root, bool& positive);
    static bool sameOrientedPlane(const double* a, const Matrix4x4d& pa,
        const double* b, const Matrix4x4d& pb);
    static bool sameHalfSpace(const PlanarHalfSpace& a, const Matrix4x4d& pa,
        const PlanarHalfSpace& b, const Matrix4x4d& pb);
    static bool sphereQuadric(const Sphere& sphere, const Matrix4x4d& ps,
        const QuadricSurface& quadric, const Matrix4x4d& pq);
    static bool halfSpaceQuadric(const PlanarHalfSpace& plane, const Matrix4x4d& pp,
        const QuadricSurface& quadric, const Matrix4x4d& pq);
    static bool worldQuadric(const QuadricSurface& quadric, const Matrix4x4d& placement, double* result);
    static bool sameQuadric(const QuadricSurface& a, const Matrix4x4d& pa,
        const QuadricSurface& b, const Matrix4x4d& pb);
public:
    static bool same(const SimpleBody& a, const SimpleBody& b);
};
#endif
