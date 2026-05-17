/****************************************************************************
 *                     poly.c
 *
 *  This module implements the code for general 3 variable polynomial shapes
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  4th - 6th order shapes and generously provided us these enhancements.
 *
 *****************************************************************************/

#include "geom/PolynomialShape.h"
#include "io/Parse.h"
#include "geom/Objects.h"
#include "processing/PolynomialSolver.h"
#include "common/VectorOps.h"
/* Basic form of a quartic equation
    a00*x^4+a01*x^3*y+a02*x^3*z+a03*x^3+a04*x^2*y^2+
    a05*x^2*y*z+a06*x^2*y+a07*x^2*z^2+a08*x^2*z+a09*x^2+
    a10*x*y^3+a11*x*y^2*z+a12*x*y^2+a13*x*y*z^2+a14*x*y*z+
    a15*x*y+a16*x*z^3+a17*x*z^2+a18*x*z+a19*x+a20*y^4+
    a21*y^3*z+a22*y^3+a23*y^2*z^2+a24*y^2*z+a25*y^2+a26*y*z^3+
    a27*y*z^2+a28*y*z+a29*y+a30*z^4+a31*z^3+a32*z^2+a33*z+a34
*/

static constexpr double COEFF_LIMIT = 1.0e-20;

static constexpr int LEFT_MARGIN = 0;
static constexpr int RIGHT_MARGIN = 72;

int binomial[11][12] = {{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 4, 6, 4, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0},
    {0, 1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0},
    {0, 1, 7, 21, 35, 35, 21, 7, 1, 0, 0, 0},
    {0, 1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0},
    {0, 1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0},
    {0, 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1}};

int factorials[MAX_ORDER + 1] = {1, 1, 2, 6, 24, 120, 720, 5040};
int termCounts[MAX_ORDER + 1] = {1, 4, 10, 20, 35, 56, 84, 120};

Methods Poly_Methods = {Composite::objectIntersect, PolynomialShape::allPolyIntersections, PolynomialShape::insidePoly,
    PolynomialShape::polyNormal, PolynomialShape::copyPoly, PolynomialShape::translatePoly, PolynomialShape::rotatePoly, PolynomialShape::scalePoly,
    PolynomialShape::invertPoly};

extern long rayPolyTests, rayPolyTestsSucceeded;

int
PolynomialShape::allPolyIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    PolynomialShape *shape = (PolynomialShape *)object;
    double depths[MAX_ORDER], len;
    Vector3D intersectionPoint;
    Vector3D dv;
    Intersection localElement;
    int cnt;
    int i;
    int j;
    int intersectionFound;
    Ray newRay;

    /* Transform the ray into the polynomial's space */
    if (shape->Transform != nullptr) {
        Transformation::MInverseTransformVector(
            &newRay.Initial, &ray->Initial, shape->Transform);
        Transformation::MInvTransVector(&newRay.Direction, &ray->Direction, shape->Transform);
    } else {
        newRay.Initial.x = ray->Initial.x;
        newRay.Initial.y = ray->Initial.y;
        newRay.Initial.z = ray->Initial.z;
        newRay.Direction.x = ray->Direction.x;
        newRay.Direction.y = ray->Direction.y;
        newRay.Direction.z = ray->Direction.z;
    }

    len = sqrt(newRay.Direction.x * newRay.Direction.x +
               newRay.Direction.y * newRay.Direction.y +
               newRay.Direction.z * newRay.Direction.z);
    if (len == 0.0) {
        return 0;
    }
    newRay.Direction.x /= len;
    newRay.Direction.y /= len;
    newRay.Direction.z /= len;

    intersectionFound = FALSE;
    rayPolyTests++;
    if (shape->Order == 4) {
        cnt = PolynomialShape::intersectQuartic(&newRay, shape, depths);
    } else {
        cnt = PolynomialShape::intersect(&newRay, shape->Order, shape->Coeffs, &depths[0]);
    }

    if (cnt > 0) {
        rayPolyTestsSucceeded++;
    }
    for (i = 0; i < cnt; i++) {
        if (depths[i] < 0) {
            goto l0;
        }
        for (j = 0; j < i; j++) {
            if (depths[i] == depths[j]) {
                goto l0;
            }
        }
        VectorOps::vScale(intersectionPoint, newRay.Direction, depths[j]);
        VectorOps::vAdd(intersectionPoint, intersectionPoint, newRay.Initial);
        /* Transform the point into world space */
        if (shape->Transform != nullptr) {
            Transformation::MTransformVector(
                &intersectionPoint, &intersectionPoint, shape->Transform);
        }

        VectorOps::vSub(dv, intersectionPoint, ray->Initial);
        VectorOps::vLength(len, dv);
        localElement.Depth = len;
        localElement.Object = shape->Parent_Object;
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;
    l0:;
    }
    return (intersectionFound);
}

/* Given the powers return the index into the polynomial */
int
PolynomialShape::roll(int order, int x, int y, int z)
{
    int xstart;
    int ystart;
    int zstart;
    xstart = binomial[order - x + 2][order - x];
    order = order - x;
    ystart = binomial[order - y + 1][order - y];
    order = order - y;
    zstart = binomial[order - z][order - z];
    return xstart + ystart + zstart;
}

/* Given the index into the polynomial, return the powers. */
void
PolynomialShape::unroll(int order, int index, int *x, int *y, int *z, int *w)
{
    int i;
    int torder;
    if (index == 0) {
        *x = order;
        *y = 0;
        *z = 0;
        *w = 0;
        return;
    }
    if (order == 1) {
        if (index == 1) {
            *x = 0;
            *y = 1;
            *z = 0;
            *w = 0;
            return;
        } else {
            *x = 0;
            *y = 0;
            *z = 3 - index;
            *w = order - (*x + *y + *z);
            return;
        }
    } else {
        for (i = 0; binomial[3 + i][i + 1] <= index; i++) {
            ;
        }
    }
    torder = order;
    *x = torder - i;
    index -= binomial[2 + i][i];
    torder = i;
    if (index == 0) {
        *y = torder;
        *z = 0;
        *w = 0;
        return;
    }
    if (torder == 1) {
        *y = 0;
        *z = 2 - index;
        *w = order - (*x + *y + *z);
        return;
    }
    for (i = 0; binomial[2 + i][i + 1] <= index; i++)
        ;
    *y = torder - i;
    index -= binomial[1 + i][i];
    torder = i;
    *z = torder - index;
    *w = order - (*x + *y + *z);
}

/* Intersection of a ray and an arbitrary polynomial function */
int
PolynomialShape::intersect(Ray *ray, int order, double *coeffs, double *depths)
{
    MATRIX q;
    double *a, t[MAX_ORDER + 1];
    int i;
    int j;
    /* Determine the coefficients of t^n, where the line is represented
        as (x,y,z) + (xx,yy,zz)*t.  */
    a = new double[termCounts[order]];
    if (a == nullptr) {
        printf("Cannot allocate memory for coefficients in poly PolynomialShape::intersect()\n");
        exit(1);
    }
    for (i = 0; i < termCounts[order]; i++) {
        a[i] = coeffs[i];
    }
    Transformation::MZero((MATRIX *)&q[0][0]);
    q[0][0] = ray->Direction.x;
    q[3][0] = ray->Initial.x;
    q[0][1] = ray->Direction.y;
    q[3][1] = ray->Initial.y;
    q[0][2] = ray->Direction.z;
    q[3][2] = ray->Initial.z;
    PolynomialShape::transform(order, a, (MATRIX *)&q[0][0]);
    /* The equation is now in terms of one variable.  Use numerical
        techniques to solve the polynomial that represents the intersections. */
    for (i = 0; i <= order; i++) {
        t[i] = a[binomial[3 + i][4] - 1];
        if (t[i] > -COEFF_LIMIT && t[i] < COEFF_LIMIT) {
            t[i] = 0.0;
        }
    }
    delete a;
    for (i = 0, j = order; i <= order; i++) {
        if (t[i] != 0.0) {
            break;
        }
        j -= 1;
    }
    if (j > 2) {
        return PolynomialSolver::polysolve(j, &t[i], depths);
    }
    if (j > 0) {
        return PolynomialSolver::solveQuadratic(&t[i], depths);
    }
    return 0;
}

double
PolynomialShape::inside(Vector3D *point, int order, double *coeffs)
{
    double x[MAX_ORDER + 1], y[MAX_ORDER + 1], z[MAX_ORDER + 1], result;
    int i;
    int k0;
    int k1;
    int k2;
    int k3;
    x[0] = 1.0;
    y[0] = 1.0;
    z[0] = 1.0;
    x[1] = point->x;
    y[1] = point->y;
    z[1] = point->z;
    for (i = 2; i <= MAX_ORDER; i++) {
        x[i] = x[1] * x[i - 1];
        y[i] = y[1] * y[i - 1];
        z[i] = z[1] * z[i - 1];
    }
    result = 0.0;
    for (i = 0; i < termCounts[order]; i++) {
        PolynomialShape::unroll(order, i, &k0, &k1, &k2, &k3);
        result += coeffs[i] * x[k0] * y[k1] * z[k2];
    }

    /* The Epsilon fudge factor is so that points really near the
        surface are considered inside the surface */
    return (result > -kEpsilon ? (result < kEpsilon ? 0.0 : result) : result);
}

/* Normal to a polynomial */
void
PolynomialShape::normalp(Vector3D *result, int order, double *coeffs, Vector3D *intersectionPoint)
{
    int i;
    int xp;
    int yp;
    int zp;
    int wp;
    double *a, x[MAX_ORDER + 1], y[MAX_ORDER + 1], z[MAX_ORDER + 1];
    x[0] = 1.0;
    y[0] = 1.0;
    z[0] = 1.0;
    x[1] = intersectionPoint->x;
    y[1] = intersectionPoint->y;
    z[1] = intersectionPoint->z;
    for (i = 2; i <= order; i++) {
        x[i] = intersectionPoint->x * x[i - 1];
        y[i] = intersectionPoint->y * y[i - 1];
        z[i] = intersectionPoint->z * z[i - 1];
    }
    a = coeffs;
    result->x = 0.0;
    result->y = 0.0;
    result->z = 0.0;
    for (i = 0; i < termCounts[order]; i++) {
        PolynomialShape::unroll(order, i, &xp, &yp, &zp, &wp);
        if (xp >= 1) {
            result->x += xp * a[i] * x[xp - 1] * y[yp] * z[zp];
        }
        if (yp >= 1) {
            result->y += yp * a[i] * x[xp] * y[yp - 1] * z[zp];
        }
        if (zp >= 1) {
            result->z += zp * a[i] * x[xp] * y[yp] * z[zp - 1];
        }
    }
    VTemp = sqrt(
        result->x * result->x + result->y * result->y + result->z * result->z);
    if (VTemp > 0.0) {
        result->x /= VTemp;
        result->y /= VTemp;
        result->z /= VTemp;
    } else {
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
    }
}

double
PolynomialShape::doPartialTerm(MATRIX *q, int row, int pwr, int i, int j, int k, int l)
{
    double result;
    int n;

    result = (double)(factorials[pwr] / (factorials[i] * factorials[j] *
                                         factorials[k] * factorials[l]));
    if (i > 0) {
        for (n = 0; n < i; n++) {
            result *= (*q)[0][row];
        }
    }
    if (j > 0) {
        for (n = 0; n < j; n++) {
            result *= (*q)[1][row];
        }
    }
    if (k > 0) {
        for (n = 0; n < k; n++) {
            result *= (*q)[2][row];
        }
    }
    if (l > 0) {
        for (n = 0; n < l; n++) {
            result *= (*q)[3][row];
        }
    }
    return result;
}

/* Using the transformation matrix q, transform the general polynomial
    equation given by a. */
void
PolynomialShape::transform(int order, double *coeffs, MATRIX *q)
{
    int termIndex;
    int partialIndex;
    int ip;
    int i;
    int i0;
    int i1;
    int i2;
    int i3;
    int jp;
    int j;
    int j0;
    int j1;
    int j2;
    int j3;
    int kp;
    int k;
    int k0;
    int k1;
    int k2;
    int k3;
    int wp;
    double *b, partialTerm;
    double tempx, tempy, tempz;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if ((*q)[i][j] > -COEFF_LIMIT && (*q)[i][j] < COEFF_LIMIT) {
                (*q)[i][j] = 0.0;
            }
        }
    }

    b = new double[termCounts[order]];
    if (b == nullptr) {
        printf("Cannot allocate memory for b in poly PolynomialShape::transform()\n");
        exit(1);
    }
    for (i = 0; i < termCounts[order]; i++) {
        b[i] = 0.0;
    }
    for (termIndex = 0; termIndex < termCounts[order]; termIndex++) {
        if (coeffs[termIndex] != 0.0) {
            PolynomialShape::unroll(order, termIndex, &ip, &jp, &kp, &wp);
            /* Step through terms in: (q[0][0]*x+q[0][1]*y+q[0][2]*z+q[0][3])^i
             */
            for (i = 0; i < termCounts[ip]; i++) {
                PolynomialShape::unroll(ip, i, &i0, &i1, &i2, &i3);
                tempx = PolynomialShape::doPartialTerm(q, 0, ip, i0, i1, i2, i3);
                if (tempx != 0.0) {

                    /* Step through terms in:
                                (q[1][0]*x+q[1][1]*y+q[1][2]*z+q[1][3])^j */
                    for (j = 0; j < termCounts[jp]; j++) {
                        PolynomialShape::unroll(jp, j, &j0, &j1, &j2, &j3);
                        tempy = PolynomialShape::doPartialTerm(q, 1, jp, j0, j1, j2, j3);
                        if (tempy != 0.0) {

                            /* Step through terms in:
                                        (q[2][0]*x+q[2][1]*y+q[2][2]*z+q[2][3])^k
                             */
                            for (k = 0; k < termCounts[kp]; k++) {
                                PolynomialShape::unroll(kp, k, &k0, &k1, &k2, &k3);
                                tempz = PolynomialShape::doPartialTerm(q, 2, kp, k0, k1, k2, k3);
                                if (tempz != 0.0) {
                                    /* Figure out it's index, and add into
                                     * result */
                                    partialIndex = PolynomialShape::roll(order, i0 + j0 + k0,
                                        i1 + j1 + k1, i2 + j2 + k2);
                                    partialTerm = coeffs[termIndex] * tempx *
                                                  tempy * tempz;
                                    b[partialIndex] += partialTerm;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < termCounts[order]; i++) {
        if (b[i] > -1.0e-4 && b[i] < 1.0e-4) {
            coeffs[i] = 0.0;
        } else {
            coeffs[i] = b[i];
        }
    }
    delete b;
}

/* Intersection of a ray and a quartic */
int
PolynomialShape::intersectQuartic(Ray *ray, PolynomialShape *shape, double *depths)
{
    double x, y, z, x2, y2, z2, x3, y3, z3, x4, y4, z4;
    double xx, yy, zz, xx2, yy2, zz2, xx3, yy3, zz3, xx4, yy4, zz4;
    double *a, t[5];
    double xZ, xZz, xxZ, xxZz, xY, xYy, xxY, xxYy, yZ, yZz, yyZ, yyZz, temp;

    x = ray->Initial.x;
    y = ray->Initial.y;
    z = ray->Initial.z;
    xx = ray->Direction.x;
    yy = ray->Direction.y;
    zz = ray->Direction.z;
    x2 = x * x;
    y2 = y * y;
    z2 = z * z;
    x3 = x * x2;
    y3 = y * y2;
    z3 = z * z2;
    x4 = x * x3;
    y4 = y * y3;
    z4 = z * z3;
    xx2 = xx * xx;
    yy2 = yy * yy;
    zz2 = zz * zz;
    xx3 = xx * xx2;
    yy3 = yy * yy2;
    zz3 = zz * zz2;
    xx4 = xx * xx3;
    yy4 = yy * yy3;
    zz4 = zz * zz3;
    a = shape->Coeffs;
    xZ = x * z;
    xZz = x * zz;
    xxZ = xx * z;
    xxZz = xx * zz;
    xY = x * y;
    xYy = x * yy;
    xxY = xx * y;
    xxYy = xx * yy;
    yZ = y * z;
    yZz = y * zz;
    yyZ = yy * z;
    yyZz = yy * zz;

    /*
        Determine the coefficients of t^n, where the line is represented
        as (x,y,z) + (xx,yy,zz)*t.
    */
    temp = a[0] * xx4;
    temp += a[1] * xx3 * yy;
    temp += a[2] * xx3 * zz;
    temp += a[4] * xx2 * yy2;
    temp += a[5] * xx2 * yyZz;
    temp += a[7] * xx2 * zz2;
    temp += a[10] * xx * yy3;
    temp += a[11] * xxZz * yy2;
    temp += a[13] * xxYy * zz2;
    temp += a[16] * xx * zz3;
    temp += a[20] * yy4;
    temp += a[21] * yy3 * zz;
    temp += a[23] * yy2 * zz2;
    temp += a[26] * yy * zz3;
    temp += a[30] * zz4;

    t[0] = temp;

    temp = 4 * a[0] * x * xx3;
    temp += a[1] * (3 * xx2 * xYy + xx3 * y);
    temp += a[2] * (3 * xx2 * xZz + xx3 * z);
    temp += a[3] * xx3;
    temp += a[4] * (2 * x * xx * yy2 + 2 * xx2 * y * yy);
    temp += a[5] * (xx2 * (yZz + yyZ) + 2 * xYy * xxZz);
    temp += a[6] * xx2 * yy;
    temp += a[7] * (2 * x * xx * zz2 + 2 * xx2 * z * zz);
    temp += a[8] * xx2 * zz;
    temp += a[10] * (x * yy3 + 3 * xxY * yy2);
    temp += a[11] * (xx * (2 * y * yyZz + yy2 * z) + xZz * yy2);
    temp += a[12] * xx * yy2;
    temp += a[13] * (xx * (y * zz2 + 2 * yyZ * zz) + xYy * zz2);
    temp += a[14] * xxYy * zz;
    temp += a[16] * (x * zz3 + 3 * xxZ * zz2);
    temp += a[17] * xx * zz2;
    temp += 4 * a[20] * y * yy3;
    temp += a[21] * (3 * yy2 * yZz + yy3 * z);
    temp += a[22] * yy3;
    temp += zz * (2 * a[23] * yy * (yZz + yyZ) + a[24] * yy2 +
                     zz * (a[26] * (yZz + 3 * yyZ) + a[27] * yy +
                              zz * (4 * a[30] * z + a[31])));
    t[1] = temp;

    temp = 6 * a[0] * x2 * xx2;
    temp += 3 * a[1] * x * xx * (xYy + xxY);
    temp += 3 * a[2] * x * xx * (xZz + xxZ);
    temp += 3 * a[3] * x * xx2;
    temp += a[4] * (x2 * yy2 + 4 * xYy * xxY + xx2 * y2);
    temp += a[5] * (x2 * yyZz + 2 * x * xx * (yZz + yyZ) + xx2 * yZ);
    temp += a[6] * (2 * x * xxYy + xx2 * y);
    temp += a[7] * (x2 * zz2 + 4 * xZz * xxZ + xx2 * z2);
    temp += a[8] * (2 * x * xxZz + xx2 * z);
    temp += a[9] * xx2;
    temp += a[10] * (3 * xY * yy2 + 3 * xxYy * y2);
    temp += a[11] * (xYy * (2 * yZz + yyZ) + xx * (y2 * zz + 2 * y * yyZ));
    temp += a[12] * (x * yy2 + 2 * xxY * yy);
    temp += a[13] * (xZz * (yZz + 2 * yyZ) + xx * (2 * yZ * zz + yy * z2));
    temp += a[14] * (xYy * zz + xx * (yZz + yyZ));
    temp += a[15] * xxYy;
    temp += a[16] * (3 * xZ * zz2 + 3 * xxZz * z2);
    temp += a[17] * (x * zz2 + 2 * xxZ * zz);
    temp += a[18] * xxZz;
    temp += 6 * a[20] * y2 * yy2;
    temp += 3 * a[21] * y * yy * (yZz + yyZ);
    temp += 3 * a[22] * y * yy2;
    temp += a[23] * (y2 * zz2 + 4 * yZz * yyZ + yy2 * z2);
    temp += a[24] * (2 * y * yyZz + yy2 * z);
    temp += a[25] * yy2;
    temp += zz * (3 * a[26] * z * (yZz + yyZ) + a[27] * (yZz + 2 * yyZ) +
                     a[28] * yy + 6 * a[30] * z2 * zz + 3 * a[31] * z * zz +
                     a[32] * zz);
    t[2] = temp;

    temp = 4 * a[0] * x3 * xx;
    temp += a[1] * x2 * (xYy + 3 * xxY);
    temp += a[2] * x2 * (xZz + 3 * xxZ);
    temp += 3 * a[3] * x2 * xx;
    temp += 2 * a[4] * xY * (xYy + xxY);
    temp += a[5] * x * (x * (yZz + yyZ) + 2 * xxY * z);
    temp += a[6] * x * (xYy + 2 * xxY);
    temp += 2 * a[7] * xZ * (xZz + xxZ);
    temp += a[8] * x * (xZz + 2 * xxZ);
    temp += 2 * a[9] * x * xx;
    temp += a[10] * (3 * xYy * y2 - xx * y3);
    temp += a[11] * (xY * (yZz + 2 * yyZ) + xxZ * y2);
    temp += a[12] * (2 * xY * yy + xx * y2);
    temp += a[13] * (xZ * (2 * yZz + yyZ) + xxY * z2);
    temp += a[14] * (x * (yZz + yyZ) + xxY * z);
    temp += a[15] * (xYy + xxY);
    temp += a[16] * (3 * xZz * z2 + xx * z3);
    temp += a[17] * (2 * xZ * zz + xx * z2);
    temp += a[18] * (xZz + xxZ);
    temp += a[19] * xx;
    temp += 4 * a[20] * y3 * yy;
    temp += a[21] * y2 * (yZz + 3 * yyZ);
    temp += 3 * a[22] * y2 * yy;
    temp += 2 * a[23] * yZ * (yZz + yyZ);
    temp += a[24] * y * (yZz + 2 * yyZ);
    temp += 2 * a[25] * y * yy;
    temp += a[26] * (3 * yZz * z2 + yy * z3);
    temp += a[27] * (2 * yZ * zz + yy * z2);
    temp += a[28] * (yZz + yyZ);
    temp += a[29] * yy;
    temp += zz * (4 * a[30] * z3 + 3 * a[31] * z2 + 2 * a[32] * z + a[33]);
    t[3] = temp;

    temp = a[0] * x4;
    temp += a[1] * x3 * y;
    temp += a[2] * x3 * z;
    temp += a[3] * x3;
    temp += a[4] * x2 * y2;
    temp += a[5] * x2 * yZ;
    temp += a[6] * x2 * y;
    temp += a[7] * x2 * z2;
    temp += a[8] * x2 * z;
    temp += a[9] * x2;
    temp += a[10] * x * y3;
    temp += a[11] * x * y2 * z;
    temp += a[12] * x * y2;
    temp += a[13] * xY * z2;
    temp += a[14] * xY * z;
    temp += a[15] * xY;
    temp += a[16] * x * z3;
    temp += a[17] * x * z2;
    temp += a[18] * xZ;
    temp += a[19] * x;
    temp += a[20] * y4;
    temp += a[21] * y3 * z;
    temp += a[22] * y3;
    temp += a[23] * y2 * z2;
    temp += a[24] * y2 * z;
    temp += a[25] * y2;
    temp += a[26] * y * z3;
    temp += a[27] * y * z2;
    temp += a[28] * yZ;
    temp += a[29] * y;
    temp += a[30] * z4;
    temp += a[31] * z3;
    temp += a[32] * z2;
    temp += a[33] * z;
    temp += a[34];
    t[4] = temp;

    if (shape->Sturm_Flag) {
        if (t[0] == 0.0) {
            if (t[1] == 0.0) {
                return PolynomialSolver::solveQuadratic(&t[2], depths);
            }
            return PolynomialSolver::polysolve(3, &t[1], depths);

        } else {
            return PolynomialSolver::polysolve(4, &t[0], depths);
        }
    } else {
        return PolynomialSolver::solveQuartic(&t[0], depths);
    }
}

/* Normal to a quartic */
void
PolynomialShape::quarticNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    PolynomialShape *shape = (PolynomialShape *)object;
    double *a, x, y, z, x2, y2, z2, x3, y3, z3;

    a = shape->Coeffs;
    x = intersectionPoint->x;
    y = intersectionPoint->y;
    z = intersectionPoint->z;
    x2 = x * x;
    y2 = y * y;
    z2 = z * z;
    x3 = x * x2;
    y3 = y * y2;
    z3 = z * z2;

    result->x =
        4 * a[0] * x3 + 3 * x2 * (a[1] * y + a[2] * z + a[3]) +
        2 * x *
            (a[4] * y2 + y * (a[5] * z + a[6]) + a[7] * z2 + a[8] * z + a[9]) +
        a[10] * y3 + y2 * (a[11] * z + a[12]) +
        y * (a[13] * z2 + a[14] * z + a[15]) + a[16] * z3 + a[17] * z2 +
        a[18] * z + a[19];

    result->y = a[1] * x3 + x2 * (2 * a[4] * y + a[5] * z + a[6]) +
                x * (3 * a[10] * y2 + 2 * y * (a[11] * z + a[12]) + a[13] * z2 +
                        a[14] * z + a[15]) +
                4 * a[20] * y3 + 3 * y2 * (a[21] * z + a[22]) +
                2 * y * (a[23] * z2 + a[24] * z + a[25]) + a[26] * z3 +
                a[27] * z2 + a[28] * z + a[29];

    result->z = a[2] * x3 + x2 * (a[5] * y + 2 * a[7] * z + a[8]) +
                x * (a[11] * y2 + y * (2 * a[13] * z + a[14]) + 3 * a[16] * z2 +
                        2 * a[17] * z + a[18]) +
                a[21] * y3 + y2 * (2 * a[23] * z + a[24]) +
                y * (3 * a[26] * z2 + 2 * a[27] * z + a[28]) + 4 * a[30] * z3 +
                3 * a[31] * z2 + 2 * a[32] * z + a[33];
    VTemp = sqrt(
        result->x * result->x + result->y * result->y + result->z * result->z);
    if (VTemp > 0.0) {
        result->x /= VTemp;
        result->y /= VTemp;
        result->z /= VTemp;
    } else {
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
    }
}

int
PolynomialShape::insidePoly(Vector3D *testPoint, SimpleBody *object)
{
    Vector3D newPoint;
    PolynomialShape *shape = (PolynomialShape *)object;
    double result;

    /* Transform the point into polynomial's space */
    if (shape->Transform != nullptr) {
        Transformation::MInverseTransformVector(&newPoint, testPoint, shape->Transform);
    } else {
        newPoint = *testPoint;
    }

    result = PolynomialShape::inside(&newPoint, shape->Order, shape->Coeffs);
    if (result < Small_Tolerance) {
        return ((int)(1 - shape->Inverted));
    }
    return ((int)shape->Inverted);
}

/* Normal to a polynomial */
void
PolynomialShape::polyNormal(Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    PolynomialShape *shape = (PolynomialShape *)object;
    Vector3D newPoint;

    /* Transform the point into the polynomials space */
    if (shape->Transform != nullptr) {
        Transformation::MInverseTransformVector(&newPoint, intersectionPoint, shape->Transform);
    } else {
        newPoint.x = intersectionPoint->x;
        newPoint.y = intersectionPoint->y;
        newPoint.z = intersectionPoint->z;
    }

    if (shape->Order == 4) {
        PolynomialShape::quarticNormal(result, object, &newPoint);
    } else {
        PolynomialShape::normalp(result, shape->Order, shape->Coeffs, &newPoint);
    }

    /* Transform back to world space */
    if (shape->Transform != nullptr) {
        Transformation::MTransNormal(result, result, shape->Transform);
    }
    VectorOps::vNormalize(*result, *result);
}

/* Make a copy of a polynomial object */
void *
PolynomialShape::copyPoly(SimpleBody *object)
{
    PolynomialShape *shape = (PolynomialShape *)object;
    PolynomialShape *newShape = SceneFactory::getPolyShape(shape->Order);
    int i;

    newShape->Shape_Texture = shape->Shape_Texture;
    newShape->Shape_Colour = shape->Shape_Colour;
    newShape->Next_Object = nullptr;
    newShape->Sturm_Flag = shape->Sturm_Flag;
    newShape->Inverted = shape->Inverted;

    /* Copy any associated transformation */
    if (shape->Transform != nullptr) {
        newShape->Transform = Transformation::getTransformation();
        memcpy(newShape->Transform, shape->Transform, sizeof(Transformation));
    }
    for (i = 0; i < termCounts[newShape->Order]; i++) {
        newShape->Coeffs[i] = shape->Coeffs[i];
    }

    /* Copy any associated texture */
    if (shape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = TextureParser::copyTexture(shape->Shape_Texture);
    }

    return (void *)(newShape);
}

void
PolynomialShape::translatePoly(SimpleBody *object, Vector3D *vector)
{
    Transformation transform;
    PolynomialShape *shape = (PolynomialShape *)object;
    if (shape->Transform == nullptr) {
        shape->Transform = Transformation::getTransformation();
    }
    Transformation::getTranslationTransformation(&transform, vector);
    Transformation::composeTransformations(shape->Transform, &transform);

    TextureUtils::translateTexture(&shape->Shape_Texture, vector);
}

void
PolynomialShape::rotatePoly(SimpleBody *object, Vector3D *vector)
{
    Transformation transform;
    PolynomialShape *shape = (PolynomialShape *)object;
    if (shape->Transform == nullptr) {
        shape->Transform = Transformation::getTransformation();
    }
    Transformation::getRotationTransformation(&transform, vector);
    Transformation::composeTransformations(shape->Transform, &transform);

    TextureUtils::rotateTexture(&shape->Shape_Texture, vector);
}

void
PolynomialShape::scalePoly(SimpleBody *object, Vector3D *vector)
{
    Transformation transform;
    PolynomialShape *shape = (PolynomialShape *)object;
    if (shape->Transform == nullptr) {
        shape->Transform = Transformation::getTransformation();
    }
    Transformation::getScalingTransformation(&transform, vector);
    Transformation::composeTransformations(shape->Transform, &transform);

    TextureUtils::scaleTexture(&shape->Shape_Texture, vector);
}

void
PolynomialShape::invertPoly(SimpleBody *object)
{
    ((PolynomialShape *)object)->Inverted = 1 - ((PolynomialShape *)object)->Inverted;
}
