#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include <cfloat>
#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/surface/_ParametricBiCubicPatchIntersector.h"

namespace {

/// Inverse of the Bezier basis matrix [FOLE1992].11.28, used to convert
/// a power basis coefficient matrix into a Bezier control net.
const double INVERSE_BEZIER_MATRIX[4][4] = {
    {0.0, 0.0,       0.0,       1.0},
    {0.0, 0.0,       1.0 / 3.0, 1.0},
    {0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0},
    {1.0, 1.0,       1.0,       1.0}
};

const double DEFAULT_RELATIVE_FLATNESS = 1.0e-3;
const int MIN_SUBDIVISION_DEPTH = 2;
const int MAX_SUBDIVISION_DEPTH = 7;
const int MAX_NEWTON_ITERATIONS = 6;
const double NEWTON_TOLERANCE = 1.0e-12;
const double BARYCENTRIC_TOLERANCE = 1.0e-9;

}

/**
Node in the bounding sphere hierarchy. Interior nodes have `children`,
leaves have the four corner points of the sub-patch and its parametric
range.
*/
struct _ParametricBiCubicPatchIntersector::PatchNode {
    double centerX;
    double centerY;
    double centerZ;
    double radiusSquared;
    /// Empty for leaves; owned by this node
    java::ArrayList<PatchNode*> children;
    double corners[12];
    double uMin;
    double uMax;
    double vMin;
    double vMax;

    PatchNode(const double net[48], bool leaf,
              double u0, double u1, double v0, double v1)
        : uMin(u0), uMax(u1), vMin(v0), vMax(v1)
    {
        double sphere[4];
        boundingSphere(net, sphere);
        centerX = sphere[0];
        centerY = sphere[1];
        centerZ = sphere[2];
        radiusSquared = sphere[3];
        if ( leaf ) {
            // Corners (u0, v0), (u0, v1), (u1, v1) and (u1, v0)
            copyPoint(net, 0, 0, corners, 0);
            copyPoint(net, 0, 3, corners, 1);
            copyPoint(net, 3, 3, corners, 2);
            copyPoint(net, 3, 0, corners, 3);
        }
    }

    ~PatchNode()
    {
        for ( long i = 0; i < children.size(); i++ ) {
            delete children[i];
        }
    }

    bool isLeaf() const
    {
        return children.isEmpty();
    }
};

_ParametricBiCubicPatchIntersector::PatchHit::PatchHit()
    : t(DBL_MAX), u(0), v(0), pointX(0), pointY(0), pointZ(0),
      triangleNx(0), triangleNy(0), triangleNz(0)
{
}

_ParametricBiCubicPatchIntersector::_ParametricBiCubicPatchIntersector(
    const Matrix4x4d& coefficientMatrixX,
    const Matrix4x4d& coefficientMatrixY,
    const Matrix4x4d& coefficientMatrixZ)
{
    storeBezierCoordinate(coefficientMatrixX, 0);
    storeBezierCoordinate(coefficientMatrixY, 1);
    storeBezierCoordinate(coefficientMatrixZ, 2);

    double rootSphere[4];
    boundingSphere(bezierNet, rootSphere);
    double tolerance =
        DEFAULT_RELATIVE_FLATNESS * std::sqrt(rootSphere[3]);
    if ( tolerance < VSDK::EPSILON * VSDK::EPSILON ) {
        tolerance = VSDK::EPSILON * VSDK::EPSILON;
    }
    int depth = requiredSubdivisionDepth(bezierNet, 0, tolerance);
    if ( depth < MIN_SUBDIVISION_DEPTH ) {
        depth = MIN_SUBDIVISION_DEPTH;
    }
    subdivisionDepth = depth;
    root = buildTree(bezierNet, 0, 0.0, 1.0, 0.0, 1.0);
}

_ParametricBiCubicPatchIntersector::~_ParametricBiCubicPatchIntersector()
{
    delete root;
}

int _ParametricBiCubicPatchIntersector::getSubdivisionDepth() const
{
    return subdivisionDepth;
}

Vector3Dd _ParametricBiCubicPatchIntersector::getBezierControlPoint(
    int i, int j) const
{
    int index = 3 * (4 * i + j);
    return Vector3Dd(bezierNet[index], bezierNet[index + 1],
        bezierNet[index + 2]);
}

void _ParametricBiCubicPatchIntersector::getMinMax(double minMax[6]) const
{
    minMax[0] = DBL_MAX;
    minMax[1] = DBL_MAX;
    minMax[2] = DBL_MAX;
    minMax[3] = -DBL_MAX;
    minMax[4] = -DBL_MAX;
    minMax[5] = -DBL_MAX;
    for ( int k = 0; k < 16; k++ ) {
        for ( int c = 0; c < 3; c++ ) {
            double value = bezierNet[3 * k + c];
            if ( value < minMax[c] ) {
                minMax[c] = value;
            }
            if ( value > minMax[c + 3] ) {
                minMax[c + 3] = value;
            }
        }
    }
}

bool _ParametricBiCubicPatchIntersector::intersect(const Ray& ray,
                                                   PatchHit& outHit) const
{
    Vector3Dd origin = ray.getOrigin();
    Vector3Dd direction = ray.getDirection();
    double o[3] = { origin.x(), origin.y(), origin.z() };
    double d[3] = { direction.x(), direction.y(), direction.z() };
    double directionLengthSquared = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];

    if ( directionLengthSquared <= 0.0 ) {
        return false;
    }

    PatchHit best;
    walkTree(root, o, d, directionLengthSquared, best);
    if ( best.t == DBL_MAX ) {
        return false;
    }
    outHit = best;
    return true;
}

void _ParametricBiCubicPatchIntersector::evaluate(double u, double v,
                                                  double out[9]) const
{
    double bu[4];
    double dbu[4];
    double bv[4];
    double dbv[4];

    bernstein(u, bu, dbu);
    bernstein(v, bv, dbv);
    for ( int k = 0; k < 9; k++ ) {
        out[k] = 0.0;
    }
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            int index = 3 * (4 * i + j);
            double w = bu[i] * bv[j];
            double wu = dbu[i] * bv[j];
            double wv = bu[i] * dbv[j];
            for ( int c = 0; c < 3; c++ ) {
                double p = bezierNet[index + c];
                out[c] += w * p;
                out[3 + c] += wu * p;
                out[6 + c] += wv * p;
            }
        }
    }
}

//= Construction ======================================================

/**
Computes B = Minv * C * Minv^t, where C is a power basis coefficient
matrix and Minv the inverse Bezier basis matrix, and stores B into the
given coordinate of the Bezier net.
*/
void _ParametricBiCubicPatchIntersector::storeBezierCoordinate(
    const Matrix4x4d& coefficients, int coordinate)
{
    double temp[4][4];
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            double accum = 0.0;
            for ( int k = 0; k < 4; k++ ) {
                accum += INVERSE_BEZIER_MATRIX[i][k] * coefficients.get(k, j);
            }
            temp[i][j] = accum;
        }
    }
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            double accum = 0.0;
            for ( int k = 0; k < 4; k++ ) {
                accum += temp[i][k] * INVERSE_BEZIER_MATRIX[j][k];
            }
            bezierNet[3 * (4 * i + j) + coordinate] = accum;
        }
    }
}

int _ParametricBiCubicPatchIntersector::requiredSubdivisionDepth(
    const double net[48], int depth, double tolerance)
{
    if ( depth >= MAX_SUBDIVISION_DEPTH ||
         subPatchFlatness(net) < tolerance ) {
        return depth;
    }
    double quarters[4][48];
    splitInFour(net, quarters);
    int maxDepth = depth;
    for ( int q = 0; q < 4; q++ ) {
        int childDepth =
            requiredSubdivisionDepth(quarters[q], depth + 1, tolerance);
        if ( childDepth > maxDepth ) {
            maxDepth = childDepth;
            if ( maxDepth >= MAX_SUBDIVISION_DEPTH ) {
                break;
            }
        }
    }
    return maxDepth;
}

/**
Port of povCpp `parametricTreeBuilder`, with a uniform leaf depth.
@return a new node, owned by the caller
*/
_ParametricBiCubicPatchIntersector::PatchNode*
_ParametricBiCubicPatchIntersector::buildTree(
    const double net[48], int depth,
    double u0, double u1, double v0, double v1) const
{
    if ( depth >= subdivisionDepth ) {
        return new PatchNode(net, true, u0, u1, v0, v1);
    }
    double um = (u0 + u1) / 2.0;
    double vm = (v0 + v1) / 2.0;
    double quarters[4][48];
    splitInFour(net, quarters);
    PatchNode* node = new PatchNode(net, false, u0, u1, v0, v1);
    node->children.reserve(4);
    node->children.add(
        buildTree(quarters[0], depth + 1, u0, um, v0, vm));
    node->children.add(
        buildTree(quarters[1], depth + 1, u0, um, vm, v1));
    node->children.add(
        buildTree(quarters[2], depth + 1, um, u1, v0, vm));
    node->children.add(
        buildTree(quarters[3], depth + 1, um, u1, vm, v1));
    return node;
}

/**
Splits the net in its four quarters, in order (low u, low v),
(low u, high v), (high u, low v) and (high u, high v)
*/
void _ParametricBiCubicPatchIntersector::splitInFour(const double net[48],
                                                     double quarters[4][48])
{
    double lowU[48];
    double highU[48];
    splitU(net, lowU, highU);

    splitV(lowU, quarters[0], quarters[1]);
    splitV(highU, quarters[2], quarters[3]);
}

/**
Splits the net at u = 1/2 (povCpp `parametricSplitUpDown`).
*/
void _ParametricBiCubicPatchIntersector::splitU(const double net[48],
                                                double low[48],
                                                double high[48])
{
    for ( int j = 0; j < 4; j++ ) {
        for ( int c = 0; c < 3; c++ ) {
            deCasteljau(net[3*(0+j)+c], net[3*(4+j)+c],
                net[3*(8+j)+c], net[3*(12+j)+c],
                low, high, 3*j + c, 12);
        }
    }
}

/**
Splits the net at v = 1/2 (povCpp `parametricSplitLeftRight`).
*/
void _ParametricBiCubicPatchIntersector::splitV(const double net[48],
                                                double low[48],
                                                double high[48])
{
    for ( int i = 0; i < 4; i++ ) {
        for ( int c = 0; c < 3; c++ ) {
            int base = 12*i + c;
            deCasteljau(net[base], net[base+3], net[base+6], net[base+9],
                low, high, base, 3);
        }
    }
}

void _ParametricBiCubicPatchIntersector::deCasteljau(
    double p0, double p1, double p2, double p3,
    double low[], double high[], int start, int stride)
{
    double p01 = (p0 + p1) / 2.0;
    double p12 = (p1 + p2) / 2.0;
    double p23 = (p2 + p3) / 2.0;
    double p012 = (p01 + p12) / 2.0;
    double p123 = (p12 + p23) / 2.0;
    double middle = (p012 + p123) / 2.0;

    low[start] = p0;
    low[start + stride] = p01;
    low[start + 2*stride] = p012;
    low[start + 3*stride] = middle;
    high[start] = middle;
    high[start + stride] = p123;
    high[start + 2*stride] = p23;
    high[start + 3*stride] = p3;
}

/**
Maximum distance between the control net and the bilinear patch spanned
by its four corners, plus the deviation of that bilinear patch with
respect to its two triangles (half the corner twist). When this value is
small, the two corner triangles approximate the sub-patch.
*/
double _ParametricBiCubicPatchIntersector::subPatchFlatness(
    const double net[48])
{
    double maxDistanceSquared = 0.0;
    for ( int i = 0; i < 4; i++ ) {
        double a = i / 3.0;
        for ( int j = 0; j < 4; j++ ) {
            double b = j / 3.0;
            double distanceSquared = 0.0;
            for ( int c = 0; c < 3; c++ ) {
                double bilinear =
                    (1-a)*(1-b)*net[c] + (1-a)*b*net[9+c] +
                    a*(1-b)*net[36+c] + a*b*net[45+c];
                double delta = net[3*(4*i + j) + c] - bilinear;
                distanceSquared += delta * delta;
            }
            if ( distanceSquared > maxDistanceSquared ) {
                maxDistanceSquared = distanceSquared;
            }
        }
    }
    double twistSquared = 0.0;
    for ( int c = 0; c < 3; c++ ) {
        double twist = (net[c] - net[9+c] - net[36+c] + net[45+c]) / 4.0;
        twistSquared += twist * twist;
    }
    return std::sqrt(maxDistanceSquared) + std::sqrt(twistSquared);
}

/**
Port of povCpp `parametricBoundingSphere`.
@param outSphere receives the center (0..2) and squared radius (3) of a
sphere containing the 16 control points of the net
*/
void _ParametricBiCubicPatchIntersector::boundingSphere(const double net[48],
                                                        double outSphere[4])
{
    double xc = 0.0;
    double yc = 0.0;
    double zc = 0.0;
    for ( int k = 0; k < 16; k++ ) {
        xc += net[3*k];
        yc += net[3*k + 1];
        zc += net[3*k + 2];
    }
    xc /= 16.0;
    yc /= 16.0;
    zc /= 16.0;
    double r0 = 0.0;
    for ( int k = 0; k < 16; k++ ) {
        double x0 = net[3*k] - xc;
        double y0 = net[3*k + 1] - yc;
        double z0 = net[3*k + 2] - zc;
        double r1 = x0*x0 + y0*y0 + z0*z0;
        if ( r1 > r0 ) {
            r0 = r1;
        }
    }
    // Small inflation so that numerically flat leaves keep a usable
    // bounding volume
    double inflation = 1.0e-9 * (1.0 + std::abs(xc) + std::abs(yc) +
        std::abs(zc));
    double radius = std::sqrt(r0) + inflation;
    outSphere[0] = xc;
    outSphere[1] = yc;
    outSphere[2] = zc;
    outSphere[3] = radius * radius;
}

void _ParametricBiCubicPatchIntersector::copyPoint(const double net[48],
                                                   int i, int j,
                                                   double target[],
                                                   int targetIndex)
{
    int source = 3 * (4 * i + j);
    target[3*targetIndex] = net[source];
    target[3*targetIndex + 1] = net[source + 1];
    target[3*targetIndex + 2] = net[source + 2];
}

//= Ray queries =======================================================

/**
Port of povCpp `parametricTreeWalker`, keeping only the nearest hit and
pruning sub-trees farther than the current nearest hit.
*/
void _ParametricBiCubicPatchIntersector::walkTree(
    const PatchNode* node, const double o[3], const double d[3],
    double directionLengthSquared, PatchHit& best) const
{
    if ( !sphericalBoundsCheck(node, o, d, directionLengthSquared,
             best.t) ) {
        return;
    }
    if ( !node->isLeaf() ) {
        for ( size_t i = 0; i < node->children.size(); i++ ) {
            walkTree(node->children[i], o, d, directionLengthSquared,
                     best);
        }
        return;
    }

    // Triangulate this sub-patch, then check for intersections in the
    // triangles (0, 1, 2) and (0, 2, 3)
    const double* c = node->corners;
    double result[3];
    if ( intersectTriangle(o, d, c, 0, 1, 2, result) &&
         result[0] < best.t ) {
        double u = node->uMin + result[2] * (node->uMax - node->uMin);
        double v = node->vMin + (result[1] + result[2]) * (node->vMax - node->vMin);
        acceptHit(node, o, d, c, 0, 1, 2, result[0], u, v, best);
    }
    if ( intersectTriangle(o, d, c, 0, 2, 3, result) &&
         result[0] < best.t ) {
        double u = node->uMin + (result[1] + result[2]) * (node->uMax - node->uMin);
        double v = node->vMin + result[1] * (node->vMax - node->vMin);
        acceptHit(node, o, d, c, 0, 2, 3, result[0], u, v, best);
    }
}

/**
Port of povCpp `sphericalBoundsCheck`, extended to reject spheres whose
entry point lies beyond the nearest hit already found.
*/
bool _ParametricBiCubicPatchIntersector::sphericalBoundsCheck(
    const PatchNode* node, const double o[3], const double d[3],
    double directionLengthSquared, double maxT)
{
    double x = node->centerX - o[0];
    double y = node->centerY - o[1];
    double z = node->centerZ - o[2];
    double distanceSquared = x*x + y*y + z*z;
    if ( distanceSquared < node->radiusSquared ) {
        // Ray starts inside sphere - assume it intersects
        return true;
    }
    double projection = x*d[0] + y*d[1] + z*d[2];
    if ( projection <= 0.0 ) {
        return false;
    }
    double closestDistanceSquared =
        distanceSquared - (projection * projection) / directionLengthSquared;
    if ( closestDistanceSquared >= node->radiusSquared ) {
        return false;
    }
    double halfChord = std::sqrt(
        (node->radiusSquared - closestDistanceSquared) / directionLengthSquared);
    double entryT = projection / directionLengthSquared - halfChord;
    return entryT < maxT;
}

/**
Moller-Trumbore ray/triangle test over the leaf corners a, b and c.
@param result receives t (0) and the barycentric coordinates of the hit
with respect to b (1) and c (2)
@return true if the ray hits the triangle at a positive distance
*/
bool _ParametricBiCubicPatchIntersector::intersectTriangle(
    const double o[3], const double d[3], const double corners[12],
    int a, int b, int c, double result[3])
{
    double ax = corners[3*a];
    double ay = corners[3*a + 1];
    double az = corners[3*a + 2];
    double e1x = corners[3*b] - ax;
    double e1y = corners[3*b + 1] - ay;
    double e1z = corners[3*b + 2] - az;
    double e2x = corners[3*c] - ax;
    double e2y = corners[3*c + 1] - ay;
    double e2z = corners[3*c + 2] - az;

    double px = d[1]*e2z - d[2]*e2y;
    double py = d[2]*e2x - d[0]*e2z;
    double pz = d[0]*e2y - d[1]*e2x;
    double determinant = e1x*px + e1y*py + e1z*pz;
    double scale = std::abs(e1x) + std::abs(e1y) + std::abs(e1z) +
        std::abs(e2x) + std::abs(e2y) + std::abs(e2z);
    if ( std::abs(determinant) <= 1.0e-14 * scale * scale ) {
        // Degenerate triangle or ray parallel to its plane
        return false;
    }
    double inverse = 1.0 / determinant;

    double sx = o[0] - ax;
    double sy = o[1] - ay;
    double sz = o[2] - az;
    double b1 = (sx*px + sy*py + sz*pz) * inverse;
    if ( b1 < -BARYCENTRIC_TOLERANCE || b1 > 1.0 + BARYCENTRIC_TOLERANCE ) {
        return false;
    }
    double qx = sy*e1z - sz*e1y;
    double qy = sz*e1x - sx*e1z;
    double qz = sx*e1y - sy*e1x;
    double b2 = (d[0]*qx + d[1]*qy + d[2]*qz) * inverse;
    if ( b2 < -BARYCENTRIC_TOLERANCE ||
         b1 + b2 > 1.0 + BARYCENTRIC_TOLERANCE ) {
        return false;
    }
    double t = (e2x*qx + e2y*qy + e2z*qz) * inverse;
    if ( t <= VSDK::EPSILON ) {
        return false;
    }
    result[0] = t;
    result[1] = java::Math::max(0.0, b1);
    result[2] = java::Math::max(0.0, b2);
    return true;
}

/**
Stores a leaf triangle hit, refining its parameters on the exact
surface when the Newton iteration converges near the leaf.
*/
void _ParametricBiCubicPatchIntersector::acceptHit(
    const PatchNode* node, const double o[3], const double d[3],
    const double corners[12], int a, int b, int c,
    double t, double u, double v, PatchHit& best) const
{
    double refined[3] = { u, v, t };
    double du = node->uMax - node->uMin;
    double dv = node->vMax - node->vMin;
    if ( refineOnSurface(o, d, refined) &&
         refined[0] >= node->uMin - du && refined[0] <= node->uMax + du &&
         refined[1] >= node->vMin - dv && refined[1] <= node->vMax + dv ) {
        if ( refined[2] <= VSDK::EPSILON ) {
            // The exact surface root is the ray origin itself (i.e. a
            // shadow or reflected ray leaving the patch): the triangle
            // hit is only an approximation artifact and must be ignored
            return;
        }
        u = refined[0];
        v = refined[1];
        t = refined[2];
    }
    if ( t >= best.t ) {
        return;
    }

    best.t = t;
    best.u = clamp01(u);
    best.v = clamp01(v);
    best.pointX = o[0] + t * d[0];
    best.pointY = o[1] + t * d[1];
    best.pointZ = o[2] + t * d[2];

    double e1x = corners[3*b] - corners[3*a];
    double e1y = corners[3*b + 1] - corners[3*a + 1];
    double e1z = corners[3*b + 2] - corners[3*a + 2];
    double e2x = corners[3*c] - corners[3*a];
    double e2y = corners[3*c + 1] - corners[3*a + 1];
    double e2z = corners[3*c + 2] - corners[3*a + 2];
    best.triangleNx = e1y*e2z - e1z*e2y;
    best.triangleNy = e1z*e2x - e1x*e2z;
    best.triangleNz = e1x*e2y - e1y*e2x;
}

/**
Newton iteration over F(u, v, t) = S(u, v) - (o + t * d) = 0.
@param uvt initial guess as input, solution as output
@return true if the iteration converged
*/
bool _ParametricBiCubicPatchIntersector::refineOnSurface(
    const double o[3], const double d[3], double uvt[3]) const
{
    double s[9];
    for ( int iteration = 0; iteration < MAX_NEWTON_ITERATIONS;
          iteration++ ) {
        evaluate(uvt[0], uvt[1], s);
        double fx = s[0] - (o[0] + uvt[2] * d[0]);
        double fy = s[1] - (o[1] + uvt[2] * d[1]);
        double fz = s[2] - (o[2] + uvt[2] * d[2]);

        // Jacobian columns: dS/du, dS/dv, -d
        double determinant = determinant3x3(
            s[3], s[6], -d[0],
            s[4], s[7], -d[1],
            s[5], s[8], -d[2]);
        if ( std::abs(determinant) < 1.0e-30 ) {
            return false;
        }
        double deltaU = determinant3x3(
            fx, s[6], -d[0],
            fy, s[7], -d[1],
            fz, s[8], -d[2]) / determinant;
        double deltaV = determinant3x3(
            s[3], fx, -d[0],
            s[4], fy, -d[1],
            s[5], fz, -d[2]) / determinant;
        double deltaT = determinant3x3(
            s[3], s[6], fx,
            s[4], s[7], fy,
            s[5], s[8], fz) / determinant;
        uvt[0] -= deltaU;
        uvt[1] -= deltaV;
        uvt[2] -= deltaT;
        if ( std::isnan(uvt[0]) || std::isnan(uvt[1]) ||
             std::isnan(uvt[2]) ) {
            return false;
        }
        if ( std::abs(deltaU) + std::abs(deltaV) < NEWTON_TOLERANCE ) {
            return true;
        }
    }
    return false;
}

double _ParametricBiCubicPatchIntersector::determinant3x3(
    double a, double b, double c,
    double d, double e, double f,
    double g, double h, double i)
{
    return a * (e*i - f*h) - b * (d*i - f*g) + c * (d*h - e*g);
}

void _ParametricBiCubicPatchIntersector::bernstein(double t, double b[4],
                                                   double db[4])
{
    double s = 1.0 - t;
    b[0] = s * s * s;
    b[1] = 3.0 * t * s * s;
    b[2] = 3.0 * t * t * s;
    b[3] = t * t * t;
    db[0] = -3.0 * s * s;
    db[1] = 3.0 * s * s - 6.0 * t * s;
    db[2] = 6.0 * t * s - 3.0 * t * t;
    db[3] = 3.0 * t * t;
}

double _ParametricBiCubicPatchIntersector::clamp01(double value)
{
    if ( value < 0.0 ) {
        return 0.0;
    }
    if ( value > 1.0 ) {
        return 1.0;
    }
    return value;
}
