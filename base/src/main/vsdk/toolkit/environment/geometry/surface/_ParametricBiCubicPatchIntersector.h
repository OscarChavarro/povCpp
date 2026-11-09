//= References:                                                             =
//= [WAYN1990] Knapp Wayne. "Ray with Bicubic Patch Intersection Problem",  =
//=            Ray Tracing News, volume 3, number 3, july 13 1990.          =
//= [FOLE1992] Foley, vanDam, Feiner, Hughes. "Computer Graphics, princi-   =
//=            ples and practice" - second edition, Addison Wesley, 1992.   =
//= [POVR1993] POV-Ray 1.0/2.x "bezier.c" bicubic_patch implementation, as  =
//=            ported to C++ in the povCpp project (ParametricPatch.cpp,    =
//=            ParametricBiCubicIntersection.cpp).                          =

#ifndef __PARAMETRIC_BI_CUBIC_PATCH_INTERSECTOR__
#define __PARAMETRIC_BI_CUBIC_PATCH_INTERSECTOR__


#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class Matrix4x4d;
class Ray;

/**
Ray / bicubic patch intersection support for `ParametricBiCubicPatch`.

This follows the "subdivision tree" strategy (POV-Ray bicubic_patch type 1/3,
as replicated on povCpp) discussed in [WAYN1990]: the patch is converted to
its Bezier control net, the net is recursively split by de Casteljau
subdivision until every sub-patch is flat enough, and a tree of bounding
spheres around the sub-patch control nets is built. Each ray walks the tree,
discarding sub-trees whose bounding sphere is missed, and leaf sub-patches are
tested as two triangles built from their corners.

Differences with respect to the povCpp version, aimed at rendering quality:
<UL>
  <LI> All leaves are at the same subdivision depth, so neighboring leaves
  share their corner points and the triangle approximation has no cracks
  (T-junctions) that would let rays leak through the surface.
  <LI> The flatness criterion measures the distance from the control net to
  the bilinear interpolation of the sub-patch corners. That bounds both the
  out of plane bending (as in povCpp) and the in plane curvature of the
  sub-patch borders.
  <LI> The (u, v) parameters of the triangle hit are refined with a few Newton
  iterations on the exact surface, so the reported point, normal, tangent and
  texture coordinates are the ones of the true bicubic surface.
</UL>

Instances are immutable after construction, so they can be shared by
multiple raytracing threads.
*/
class _ParametricBiCubicPatchIntersector {
public:
    /**
    Result of a ray/patch query, in patch parameter space.
    */
    struct PatchHit {
        double t;
        double u;
        double v;
        double pointX;
        double pointY;
        double pointZ;
        double triangleNx;
        double triangleNy;
        double triangleNz;

        PatchHit();
    };

    /**
    Builds the intersection structures for the patch with the given power
    basis coefficient matrices (`M * G * Mt` for each coordinate).
    @param coefficientMatrixX x coordinate coefficients
    @param coefficientMatrixY y coordinate coefficients
    @param coefficientMatrixZ z coordinate coefficients
    */
    _ParametricBiCubicPatchIntersector(const Matrix4x4d& coefficientMatrixX,
                                       const Matrix4x4d& coefficientMatrixY,
                                       const Matrix4x4d& coefficientMatrixZ);
    ~_ParametricBiCubicPatchIntersector();

    /**
    @return the number of de Casteljau subdivision levels used for the
    leaves of the bounding sphere tree
    */
    int getSubdivisionDepth() const;

    /**
    @param i control point index on the s (u) direction, in [0, 3]
    @param j control point index on the t (v) direction, in [0, 3]
    @return the Bezier control point (i, j) equivalent to current patch
    */
    Vector3Dd getBezierControlPoint(int i, int j) const;

    /**
    Bounding box of the Bezier control net. By the convex hull property it
    contains the whole patch.
    @param outMinMax receives the min-max array as specified by
    `Geometry::getMinMax`
    */
    void getMinMax(double outMinMax[6]) const;

    /**
    Finds the nearest intersection of the ray with the patch.
    @param ray ray to test, its direction does not need to be normalized
    @param outHit receives the nearest hit with positive ray parameter
    @return false if the ray misses the patch (Java returns null)
    */
    bool intersect(const Ray& ray, PatchHit& outHit) const;

    /**
    Evaluates the patch and its first partial derivatives.
    @param u parameter in the s direction
    @param v parameter in the t direction
    @param out receives point (0..2), dS/du (3..5) and dS/dv (6..8)
    */
    void evaluate(double u, double v, double out[9]) const;

private:
    struct PatchNode;

    /// Bezier control net, stored as 16 points of 3 coordinates, with point
    /// (i, j) at index 3*(4*i + j). Index i follows the s (u) parameter and
    /// index j the t (v) parameter of `ParametricBiCubicPatch::evaluate`.
    double bezierNet[48];
    /// Owned by this intersector
    PatchNode* root;
    int subdivisionDepth;

    _ParametricBiCubicPatchIntersector(const _ParametricBiCubicPatchIntersector&);
    _ParametricBiCubicPatchIntersector& operator=(
        const _ParametricBiCubicPatchIntersector&);

    void storeBezierCoordinate(const Matrix4x4d& coefficients, int coordinate);
    static int requiredSubdivisionDepth(const double net[48], int depth,
                                        double tolerance);
    PatchNode* buildTree(const double net[48], int depth,
                         double u0, double u1, double v0, double v1) const;
    static void splitInFour(const double net[48], double quarters[4][48]);
    static void splitU(const double net[48], double low[48], double high[48]);
    static void splitV(const double net[48], double low[48], double high[48]);
    static void deCasteljau(double p0, double p1, double p2, double p3,
                            double low[], double high[],
                            int start, int stride);
    static double subPatchFlatness(const double net[48]);
    static void boundingSphere(const double net[48], double outSphere[4]);
    static void copyPoint(const double net[48], int i, int j,
                          double target[], int targetIndex);
    void walkTree(const PatchNode* node, const double o[3], const double d[3],
                  double directionLengthSquared, PatchHit& best) const;
    static bool sphericalBoundsCheck(const PatchNode* node, const double o[3],
                                     const double d[3],
                                     double directionLengthSquared,
                                     double maxT);
    static bool intersectTriangle(const double o[3], const double d[3],
                                  const double corners[12],
                                  int a, int b, int c, double result[3]);
    void acceptHit(const PatchNode* node, const double o[3], const double d[3],
                   const double corners[12], int a, int b, int c,
                   double t, double u, double v, PatchHit& best) const;
    bool refineOnSurface(const double o[3], const double d[3],
                         double uvt[3]) const;
    static double determinant3x3(double a, double b, double c,
                                 double d, double e, double f,
                                 double g, double h, double i);
    static void bernstein(double t, double b[4], double db[4]);
    static double clamp01(double value);
};

#endif
