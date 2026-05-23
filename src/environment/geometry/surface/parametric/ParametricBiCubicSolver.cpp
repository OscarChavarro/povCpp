#include "environment/geometry/surface/parametric/ParametricBiCubicSolver.h"
#include "common/Statistics.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicIntersection.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"

extern RayWithSegments *vpRay;

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch0(
    RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    int tcnt = shape->intersectionCount;
    int i;
    int j;
    double depth;
    double d;
    double u;
    double v;
    double deltaU;
    double deltaV;
    Vector3Dd v0;
    Vector3Dd v1;
    Vector3Dd v2;
    Vector3Dd v3;
    Vector3Dd n;
    Vector3Dd ip;
    Vector3Dd(*patchPtr)[4][4] = (Vector3Dd(*)[4][4])shape->Control_Points;

    if (!ParametricBiCubicIntersection::sphericalBoundsCheck(ray,
            &(shape->boundingSphereCenter), shape->boundingSphereRadius)) {
        return 0;
    }

    deltaU = 1.0 / (double)shape->uSteps;
    deltaV = 1.0 / (double)shape->vSteps;

    /* Calculate the initial point */
    for (i = 0; i < shape->uSteps; i++) {
        u = (double)i / (double)shape->uSteps;
        for (j = 0; j < shape->vSteps; j++) {
            v = (double)j / (double)shape->vSteps;

            /* Calculate surface values for the current patch. */
            ParametricBiCubicPatch::parametricValue(&v0, u, v, patchPtr);
            ParametricBiCubicPatch::parametricValue(
                &v1, u + deltaU, v, patchPtr);
            ParametricBiCubicPatch::parametricValue(
                &v2, u, v + deltaV, patchPtr);
            ParametricBiCubicPatch::parametricValue(
                &v3, u + deltaU, v + deltaV, patchPtr);

            /* Triangulate this subpatch, then check for intersections in
                the triangles. */
            if (ParametricBiCubicIntersection::subpatchNormal(
                    &v0, &v2, &v1, &n, &d)) {
                if (ParametricBiCubicIntersection::intersectSubpatch(
                        shape->patchType, ray, &v0, &v2, &v1, &n, d, nullptr,
                        nullptr, nullptr, &depth, &ip, &n)) {
                    shape->Intersection_Point[tcnt + cnt] = ip;
                    shape->normalVector[tcnt + cnt] = n;
                    depths[cnt] = depth;
                    if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                        /* Too many intersections. Stop looking for more. */
                        return cnt;
                    }
                }
            }
            if (ParametricBiCubicIntersection::subpatchNormal(
                    &v1, &v2, &v3, &n, &d)) {
                if (ParametricBiCubicIntersection::intersectSubpatch(
                        shape->patchType, ray, &v1, &v2, &v3, &n, d, nullptr,
                        nullptr, nullptr, &depth, &ip, &n)) {
                    shape->Intersection_Point[tcnt + cnt] = ip;
                    shape->normalVector[tcnt + cnt] = n;
                    depths[cnt] = depth;
                    if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                        /* Too many intersections. Stop looking for more. */
                        return cnt;
                    }
                }
            }
        }
    }
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch1(
    RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    int tcnt = shape->intersectionCount;
    int i;
    int j;
    double depth;
    double d;
    double radius;
    Vector3Dd v[4];
    Vector3Dd n;
    Vector3Dd ip;
    Vector3Dd center;

    if (!ParametricBiCubicIntersection::sphericalBoundsCheck(ray,
            &(shape->boundingSphereCenter), shape->boundingSphereRadius)) {
        return 0;
    }

    /* Calculate the initial point */
    for (i = 0; i < shape->uSteps; i++) {
        for (j = 0; j < shape->vSteps; j++) {

            /* Grab precomputed surface values for the current patch. */
            v[0] = shape->Interpolated_Grid[i][j];
            v[1] = shape->Interpolated_Grid[i + 1][j];
            v[2] = shape->Interpolated_Grid[i][j + 1];
            v[3] = shape->Interpolated_Grid[i + 1][j + 1];

            /* Check the ray against the bounding sphere for this subpatch */
            ParametricBiCubicPatch::findAverage(4, &v[0], &center, &radius);
            if (!ParametricBiCubicIntersection::sphericalBoundsCheck(
                    ray, &center, radius)) {
                continue;
            }

            n = shape->Interpolated_Normals[i][2 * j];
            if (n.x == 0.0 && n.y == 0.0 && n.z == 0.0) {
                goto l0;
            }
            d = shape->Interpolated_D[i][2 * j];

            /* Check for intersections in this subpatch. */
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->patchType, ray, &v[0], &v[2], &v[1], &n, d, nullptr,
                    nullptr, nullptr, &depth, &ip, &n)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->normalVector[tcnt + cnt] = n;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                    /* Too many intersections. Stop looking for more. */
                    return cnt;
                }
            }
        l0:
            n = shape->Interpolated_Normals[i][2 * j + 1];
            if (n.x == 0.0 && n.y == 0.0 && n.z == 0.0) {
                continue;
            }
            d = shape->Interpolated_D[i][2 * j + 1];
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->patchType, ray, &v[1], &v[2], &v[3], &n, d, nullptr,
                    nullptr, nullptr, &depth, &ip, &n)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->normalVector[tcnt + cnt] = n;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                    /* Too many intersections. Stop looking for more. */
                    return cnt;
                }
            }
        }
    }
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch2(
    RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    double uValues[MAX_BICUBIC_INTERSECTIONS];
    double vValues[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd(*patch)[4][4] = (Vector3Dd(*)[4][4])shape->Control_Points;

    ParametricBiCubicPatch::parametricSubdivider(ray, shape, patch, 0.0, 1.0,
        0.0, 1.0, 0, &cnt, depths, &uValues[0], &vValues[0]);
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch3(
    RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    ParametricBiCubicPatch::parametricTreeWalker(
        ray, shape, shape->Node_Tree, 0, &cnt, depths);
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch4(
    RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    int tcnt = shape->intersectionCount;
    int i;
    int j;
    double depth;
    double d;
    double t;
    Vector3Dd v0;
    Vector3Dd v1;
    Vector3Dd v2;
    Vector3Dd v3;
    Vector3Dd n;
    Vector3Dd n0;
    Vector3Dd n1;
    Vector3Dd n2;
    Vector3Dd n3;
    Vector3Dd ip;
    Vector3Dd ipNorm;

    if (!ParametricBiCubicIntersection::sphericalBoundsCheck(ray,
            &(shape->boundingSphereCenter), shape->boundingSphereRadius)) {
        return 0;
    }

    /* Calculate the initial point */
    for (i = 0; i < shape->uSteps; i++) {
        for (j = 0; j < shape->vSteps; j++) {

            /* Grab precomputed surface values for the current patch. */
            v0 = shape->Interpolated_Grid[i][j];
            v1 = shape->Interpolated_Grid[i + 1][j];
            v2 = shape->Interpolated_Grid[i][j + 1];
            v3 = shape->Interpolated_Grid[i + 1][j + 1];

            n0 = shape->Smooth_Normals[i][j];
            n1 = shape->Smooth_Normals[i + 1][j];
            n2 = shape->Smooth_Normals[i][j + 1];
            n3 = shape->Smooth_Normals[i + 1][j + 1];

            n = shape->Interpolated_Normals[i][2 * j];
            if (n.x == 0.0 && n.y == 0.0 && n.z == 0.0) {
                goto l0;
            }
            d = shape->Interpolated_D[i][2 * j];

            /* Make sure the smooth normals point in the same direction as the
             * normal */
            t = n0.dotProduct(n);
            if (t < 0) {
                n0.scale(-1.0);
            }
            t = n1.dotProduct(n);
            if (t < 0) {
                n1.scale(-1.0);
            }
            t = n2.dotProduct(n);
            if (t < 0) {
                n2.scale(-1.0);
            }

            /* Check for intersections in this subpatch. */
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->patchType, ray, &v0, &v2, &v1, &n, d, &n0, &n2, &n1,
                    &depth, &ip, &ipNorm)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->normalVector[tcnt + cnt] = ipNorm;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                    /* Too many intersections. Stop looking for more. */
                    return cnt;
                }
            }
        l0:
            n = shape->Interpolated_Normals[i][2 * j + 1];
            if (n.x == 0.0 && n.y == 0.0 && n.z == 0.0) {
                continue;
            }
            d = shape->Interpolated_D[i][2 * j + 1];

            /* Make sure the smooth normals point in the same direction as the
             * normal */
            t = n1.dotProduct(n);
            if (t > 0) {
                VectorOps::vScale(n1, n0, -1.0);
            }
            t = n2.dotProduct(n);
            if (t > 0) {
                VectorOps::vScale(n2, n1, -1.0);
            }
            t = n3.dotProduct(n);
            if (t > 0) {
                VectorOps::vScale(n3, n2, -1.0);
            }

            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->patchType, ray, &v1, &v2, &v3, &n, d, &n1, &n2, &n3,
                    &depth, &ip, &ipNorm)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->normalVector[tcnt + cnt] = ipNorm;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                    /* Too many intersections. Stop looking for more. */
                    return cnt;
                }
            }
        }
    }
    return cnt;
}

int
ParametricBiCubicSolver::allParametricBiCubicPatchIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    ParametricBiCubicPatch *shape = (ParametricBiCubicPatch *)object;
    double depths[MAX_BICUBIC_INTERSECTIONS];
    Intersection localElement;
    int cnt = 0;
    int tcnt;
    int i;
    int intersectionFound;

    intersectionFound = 0;
    globalStatistics.rayBicubicTests++;
    if (ray == vpRay) {
        shape->intersectionCount = 0;
    }
    tcnt = shape->intersectionCount;
    if (shape->patchType == 0) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch0(
            ray, shape, &depths[0]);
    } else if (shape->patchType == 1) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch1(
            ray, shape, &depths[0]);
    } else if (shape->patchType == 2) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch2(
            ray, shape, &depths[0]);
    } else if (shape->patchType == 3) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch3(
            ray, shape, &depths[0]);
    } else if (shape->patchType == 4) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch4(
            ray, shape, &depths[0]);
    } else {
        Logger::error("Bad patch type\n");
        exit(1);
    }
    if (cnt > 0) {
        globalStatistics.rayBicubicTestsSucceeded++;
    }
    for (i = 0; i < cnt; i++) {
        if (!ray->isShadowRay) {
            shape->intersectionCount++;
        }
        localElement.Depth = depths[i];
        localElement.Object = nullptr;
        localElement.Point = shape->Intersection_Point[tcnt + i];
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = 1;
    }
    return (intersectionFound);
}
