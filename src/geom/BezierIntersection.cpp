#include "geom/BezierIntersection.h"
#include "geom/GeometryOperations.h"
#include "common/VectorOps.h"
#undef EPSILON
static constexpr double EPSILON = 1.0e-10;

/* Calculate the normal to a subpatch (triangle) return the vector
    <1.0 0.0 0.0> if the triangle is degenerate. */
int
BezierIntersection::subpatchNormal(
    Vector3D *v1, Vector3D *v2, Vector3D *v3, Vector3D *result, double *d)
{
    Vector3D edge1;
    Vector3D edge2;
    double length;

    VectorOps::vSub(edge1, *v1, *v2);
    VectorOps::vSub(edge2, *v3, *v2);
    VectorOps::vCross(*result, edge1, edge2);
    VectorOps::vLength(length, *result);
    if (length < EPSILON) {
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
        *d = -1.0 * v1->x;
        return 0;
    }
    VectorOps::vInverseScale(*result, *result, length);
    VectorOps::vDot(*d, *result, *v1);
    *d = 0.0 - *d;
    return 1;
}

/* See if Ray intersects the triangular subpatch defined by the three
    points v1, v2, v3 and the normal to the triangle n. If so, Depth will
    be set to the distance along Ray at which the intersection occurs. */
int
BezierIntersection::intersectSubpatch(int patchType, Ray *ray, Vector3D *v1, Vector3D *v2,
    Vector3D *v3, Vector3D *n, double d, Vector3D *n1, Vector3D *n2, Vector3D *n3,
    double *depth, Vector3D *ip, Vector3D *ipNorm)
{
    Vector3D tempV1;
    Vector3D tempV2;
    Vector3D perp;
    double s, t, proj, mu;
    double x1, y1, x2, y2, x3, y3;
    double x, y, z;
    int signHolder;
    int nextSign;
    int crossings;

    /* Calculate the point of intersection and the depth. */
    VectorOps::vDot(s, ray->Direction, *n);
    if (s == 0.0) {
        return 0;
    }
    VectorOps::vDot(t, ray->Initial, *n);
    *depth = 0.0 - (d + t) / s;
    if (*depth < Small_Tolerance) {
        return 0;
    }
    VectorOps::vScale(*ip, ray->Direction, *depth);
    VectorOps::vAdd(*ip, *ip, ray->Initial);

    /* Map the intersection point and the triangle onto a plane. */
    x = fabs(n->x);
    y = fabs(n->y);
    z = fabs(n->z);
    if (x > y) {
        if (x > z) {
            x1 = v1->y - ip->y;
            y1 = v1->z - ip->z;
            x2 = v2->y - ip->y;
            y2 = v2->z - ip->z;
            x3 = v3->y - ip->y;
            y3 = v3->z - ip->z;
        } else {
            x1 = v1->x - ip->x;
            y1 = v1->y - ip->y;
            x2 = v2->x - ip->x;
            y2 = v2->y - ip->y;
            x3 = v3->x - ip->x;
            y3 = v3->y - ip->y;
        }
    } else if (y > z) {
        x1 = v1->x - ip->x;
        y1 = v1->z - ip->z;
        x2 = v2->x - ip->x;
        y2 = v2->z - ip->z;
        x3 = v3->x - ip->x;
        y3 = v3->z - ip->z;
    } else {
        x1 = v1->x - ip->x;
        y1 = v1->y - ip->y;
        x2 = v2->x - ip->x;
        y2 = v2->y - ip->y;
        x3 = v3->x - ip->x;
        y3 = v3->y - ip->y;
    }

    /* Determine crossing count */
    crossings = 0;
    if (y1 < 0) {
        signHolder = -1;
    } else {
        signHolder = 1;
    }

    /* Start of winding tests, test the segment from v1 to v2 */
    if (y2 < 0) {
        nextSign = -1;
    } else {
        nextSign = 1;
    }
    if (signHolder != nextSign) {
        if (x1 > 0.0) {
            if (x2 > 0.0) {
                crossings++;
            } else if ((x1 - y1 * (x2 - x1) / (y2 - y1)) > 0.0) {
                crossings++;
            }
        } else if (x2 > 0.0) {
            if ((x1 - y1 * (x2 - x1) / (y2 - y1)) > 0.0) {
                crossings++;
            }
        }
        signHolder = nextSign;
    }

    /* Test the segment from v2 to v3 */
    if (y3 < 0) {
        nextSign = -1;
    } else {
        nextSign = 1;
    }
    if (signHolder != nextSign) {
        if (x2 > 0.0) {
            if (x3 > 0.0) {
                crossings++;
            } else if ((x2 - y2 * (x3 - x2) / (y3 - y2)) > 0.0) {
                crossings++;
            }
        } else if (x3 > 0.0) {
            if ((x2 - y2 * (x3 - x2) / (y3 - y2)) > 0.0) {
                crossings++;
            }
        }
        signHolder = nextSign;
    }

    /* Test the segment from v3 to v1 */
    if (y1 < 0) {
        nextSign = -1;
    } else {
        nextSign = 1;
    }
    if (signHolder != nextSign) {
        if (x3 > 0.0) {
            if (x1 > 0.0) {
                crossings++;
            } else if ((x3 - y3 * (x1 - x3) / (y1 - y3)) > 0.0) {
                crossings++;
            }
        } else if (x1 > 0.0) {
            if ((x3 - y3 * (x1 - x3) / (y1 - y3)) > 0.0) {
                crossings++;
            }
        }
        signHolder = nextSign;
    }
    if (crossings != 1) {
        return 0;
    }

    if (patchType == 0 || patchType == 1 || patchType == 2 || patchType == 3) {
        *ipNorm = *n;
        return 1;
    }
    if (patchType == 4) {
        VectorOps::vSub(tempV1, *v2, *v3);
        VectorOps::vNormalize(tempV1, tempV1);
        VectorOps::vSub(tempV2, *v1, *v3);
        VectorOps::vDot(proj, tempV2, tempV1);
        VectorOps::vScale(tempV1, tempV1, proj);
        VectorOps::vSub(perp, tempV1, tempV2);
        VectorOps::vNormalize(perp, perp);
        VectorOps::vDot(mu, tempV2, perp);
        mu = -1.0 / mu;
        VectorOps::vScale(perp, perp, mu);
        VectorOps::vSub(tempV1, *ip, *v1);
        VectorOps::vDot(s, tempV1, perp);
        if (s < EPSILON) {
            *ipNorm = *n1;
            return 1;
        }
        VectorOps::vSub(tempV1, *v3, *v2);
        x = fabs(tempV1.x);
        y = fabs(tempV1.y);
        z = fabs(tempV1.z);
        if (x > y) {
            if (x > z) {
                t = (tempV1.x / s + v1->x - v2->x) / tempV1.x;
            } else {
                t = (tempV1.z / s + v1->z - v2->z) / tempV1.z;
            }
        } else if (y > z) {
            t = (tempV1.y / s + v1->y - v2->y) / tempV1.y;
        } else {
            t = (tempV1.z / s + v1->z - v2->z) / tempV1.z;
        }
        VectorOps::vSub(tempV1, *n2, *n1);
        VectorOps::vScale(tempV1, tempV1, s);
        VectorOps::vAdd(tempV1, tempV1, *n1);
        VectorOps::vSub(tempV2, *n3, *n1);
        VectorOps::vScale(tempV2, tempV2, s);
        VectorOps::vAdd(tempV2, tempV2, *n1);
        VectorOps::vSub(*ipNorm, tempV2, tempV1);
        VectorOps::vScale(*ipNorm, *ipNorm, t);
        VectorOps::vAdd(*ipNorm, *ipNorm, tempV1);
        VectorOps::vNormalize(*ipNorm, *ipNorm);
        return 1;
    }
    return 0;
}

int
BezierIntersection::sphericalBoundsCheck(Ray *ray, Vector3D *center, double radius)
{
    double x, y, z, dist1, dist2;
    x = center->x - ray->Initial.x;
    y = center->y - ray->Initial.y;
    z = center->z - ray->Initial.z;
    dist1 = x * x + y * y + z * z;
    if (dist1 < radius) {
        /* ray starts inside sphere - assume it intersects. */
        return 1;
    }
    dist2 = x * ray->Direction.x + y * ray->Direction.y + z * ray->Direction.z;
    dist2 = dist2 * dist2;
    if (dist2 > 0 && (dist1 - dist2 < radius)) {
        return 1;
    }

    return 0;
}

/* Determine the distance from a point to a plane. */
double
BezierIntersection::pointPlaneDistance(Vector3D *p, Vector3D *n, double *d)
{
    double temp1, temp2;

    VectorOps::vDot(temp1, *p, *n);
    temp1 += *d;
    VectorOps::vLength(temp2, *n);
    if (fabs(temp2) < EPSILON) {
        return 0;
    }
    temp1 /= temp2;
    return temp1;
}
