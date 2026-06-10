#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicIntersection.h"
#undef EPSILON
static constexpr double EPSILON = 1.0e-10;

/* Calculate the normal to a subpatch (triangle) return the vector
    <1.0 0.0 0.0> if the triangle is degenerate. */
int
ParametricBiCubicIntersection::subpatchNormal(
    Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3, Vector3Dd *result, double *d)
{
    Vector3Dd edge1;
    Vector3Dd edge2;
    double length;

    edge1 = v1->subtract(*v2);
    edge2 = v3->subtract(*v2);
    *result = edge1.crossProduct(edge2);
    length = (*result).length();
    if (length < EPSILON) {
        *result = Vector3Dd(1.0, 0.0, 0.0);
        *d = -1.0 * v1->x();
        return 0;
    }
    *result = Vector3Dd((*result).x() / length, (*result).y() / length, (*result).z() / length);
    *d = (*result).dotProduct(*v1);
    *d = 0.0 - *d;
    return 1;
}

/* See if Ray intersects the triangular subpatch defined by the three
    points v1, v2, v3 and the normal to the triangle n. If so, Depth will
    be set to the distance along Ray at which the intersection occurs. */
int
ParametricBiCubicIntersection::intersectSubpatch(int patchType,
    RayWithSegments *ray, Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3,
    Vector3Dd *n, double d, Vector3Dd *n1, Vector3Dd *n2, Vector3Dd *n3,
    double *depth, Vector3Dd *ip, Vector3Dd *ipNorm)
{
    Vector3Dd tempV1;
    Vector3Dd tempV2;
    Vector3Dd perp;
    double s;
    double t;
    double proj;
    double mu;
    double x1;
    double y1;
    double x2;
    double y2;
    double x3;
    double y3;
    double x;
    double y;
    double z;
    int signHolder;
    int nextSign;
    int crossings;

    /* Calculate the point of intersection and the depth. */
    s = ray->direction.dotProduct(*n);
    if (s == 0.0) {
        return 0;
    }
    t = ray->position.dotProduct(*n);
    *depth = 0.0 - (d + t) / s;
    if (*depth < GeometryConstants::Small_Tolerance) {
        return 0;
    }
    *ip = ray->direction.multiply(*depth);
    *ip = ip->add(ray->position);

    /* Map the intersection point and the triangle onto a plane. */
    x = fabs(n->x());
    y = fabs(n->y());
    z = fabs(n->z());
    if (x > y) {
        if (x > z) {
            x1 = v1->y() - ip->y();
            y1 = v1->z() - ip->z();
            x2 = v2->y() - ip->y();
            y2 = v2->z() - ip->z();
            x3 = v3->y() - ip->y();
            y3 = v3->z() - ip->z();
        } else {
            x1 = v1->x() - ip->x();
            y1 = v1->y() - ip->y();
            x2 = v2->x() - ip->x();
            y2 = v2->y() - ip->y();
            x3 = v3->x() - ip->x();
            y3 = v3->y() - ip->y();
        }
    } else if (y > z) {
        x1 = v1->x() - ip->x();
        y1 = v1->z() - ip->z();
        x2 = v2->x() - ip->x();
        y2 = v2->z() - ip->z();
        x3 = v3->x() - ip->x();
        y3 = v3->z() - ip->z();
    } else {
        x1 = v1->x() - ip->x();
        y1 = v1->y() - ip->y();
        x2 = v2->x() - ip->x();
        y2 = v2->y() - ip->y();
        x3 = v3->x() - ip->x();
        y3 = v3->y() - ip->y();
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
        tempV1 = v2->subtract(*v3);
        tempV1 = tempV1.normalizedFast();
        tempV2 = v1->subtract(*v3);
        proj = tempV2.dotProduct(tempV1);
        tempV1 = tempV1.multiply(proj);
        perp = tempV1.subtract(tempV2);
        perp = perp.normalizedFast();
        mu = tempV2.dotProduct(perp);
        mu = -1.0 / mu;
        perp = perp.multiply(mu);
        tempV1 = ip->subtract(*v1);
        s = tempV1.dotProduct(perp);
        if (s < EPSILON) {
            *ipNorm = *n1;
            return 1;
        }
        tempV1 = v3->subtract(*v2);
        x = fabs(tempV1.x());
        y = fabs(tempV1.y());
        z = fabs(tempV1.z());
        if (x > y) {
            if (x > z) {
                t = (tempV1.x() / s + v1->x() - v2->x()) / tempV1.x();
            } else {
                t = (tempV1.z() / s + v1->z() - v2->z()) / tempV1.z();
            }
        } else if (y > z) {
            t = (tempV1.y() / s + v1->y() - v2->y()) / tempV1.y();
        } else {
            t = (tempV1.z() / s + v1->z() - v2->z()) / tempV1.z();
        }
        tempV1 = n2->subtract(*n1);
        tempV1 = tempV1.multiply(s);
        tempV1 = tempV1.add(*n1);
        tempV2 = n3->subtract(*n1);
        tempV2 = tempV2.multiply(s);
        tempV2 = tempV2.add(*n1);
        *ipNorm = tempV2.subtract(tempV1);
        *ipNorm = (*ipNorm).multiply(t);
        *ipNorm = ipNorm->add(tempV1);
        *ipNorm = (*ipNorm).normalizedFast();
        return 1;
    }
    return 0;
}

int
ParametricBiCubicIntersection::sphericalBoundsCheck(
    RayWithSegments *ray, Vector3Dd *center, double radius)
{
    double x;
    double y;
    double z;
    double dist1;
    double dist2;
    x = center->x() - ray->position.x();
    y = center->y() - ray->position.y();
    z = center->z() - ray->position.z();
    dist1 = x * x + y * y + z * z;
    if (dist1 < radius) {
        /* ray starts inside sphere - assume it intersects. */
        return 1;
    }
    dist2 = x * ray->direction.x() + y * ray->direction.y() + z * ray->direction.z();
    dist2 = dist2 * dist2;
    if (dist2 > 0 && (dist1 - dist2 < radius)) {
        return 1;
    }

    return 0;
}

/* Determine the distance from a point to a plane. */
double
ParametricBiCubicIntersection::pointPlaneDistance(
    Vector3Dd *p, Vector3Dd *n, double *d)
{
    double temp1;
    double temp2;

    temp1 = (*p).dotProduct(*n);
    temp1 += *d;
    temp2 = (*n).length();
    if (fabs(temp2) < EPSILON) {
        return 0;
    }
    temp1 /= temp2;
    return temp1;
}
