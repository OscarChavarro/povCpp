#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "common/statistics/Statistics.h"

#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicIntersection.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicSolver.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"


int
ParametricBiCubicSolver::intersectParametricBiCubicPatch0(
    const RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    const int tcnt = shape->getIntersectionCount();
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
    Vector3Dd(* const patchPtr)[4][4] =
        (Vector3Dd(*)[4][4])shape->getControlPoints();

    if (!ParametricBiCubicIntersection::sphericalBoundsCheck(ray,
            &(shape->getBoundingSphereCenter()),
            shape->getBoundingSphereRadius())) {
        return 0;
    }

    deltaU = 1.0 / (double)shape->getUSteps();
    deltaV = 1.0 / (double)shape->getVSteps();

    // Calculate the initial point
    for (i = 0; i < shape->getUSteps(); i++) {
        u = (double)i / (double)shape->getUSteps();
        for (j = 0; j < shape->getVSteps(); j++) {
            v = (double)j / (double)shape->getVSteps();

            // Calculate surface values for the current patch
            ParametricBiCubicPatch::parametricValue(&v0, u, v, patchPtr);
            ParametricBiCubicPatch::parametricValue(
                &v1, u + deltaU, v, patchPtr);
            ParametricBiCubicPatch::parametricValue(
                &v2, u, v + deltaV, patchPtr);
            ParametricBiCubicPatch::parametricValue(
                &v3, u + deltaU, v + deltaV, patchPtr);

            // Triangulate this subpatch, then check for intersections in
            // the triangles
            if (ParametricBiCubicIntersection::subpatchNormal(
                    &v0, &v2, &v1, &n, &d)) {
                if (ParametricBiCubicIntersection::intersectSubpatch(
                        shape->getPatchType(), ray, &v0, &v2, &v1, &n, d, nullptr,
                        nullptr, nullptr, &depth, &ip, &n)) {
                    shape->getIntersectionPointAt(tcnt + cnt) = ip;
                    shape->getNormalVectorAt(tcnt + cnt) = n;
                    depths[cnt] = depth;
                    if (tcnt + ++cnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
                        // Too many intersections. Stop looking for more
                        return cnt;
                    }
                }
            }
            if (ParametricBiCubicIntersection::subpatchNormal(
                    &v1, &v2, &v3, &n, &d)) {
                if (ParametricBiCubicIntersection::intersectSubpatch(
                        shape->getPatchType(), ray, &v1, &v2, &v3, &n, d, nullptr,
                        nullptr, nullptr, &depth, &ip, &n)) {
                    shape->getIntersectionPointAt(tcnt + cnt) = ip;
                    shape->getNormalVectorAt(tcnt + cnt) = n;
                    depths[cnt] = depth;
                    if (tcnt + ++cnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
                        // Too many intersections. Stop looking for more
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
    const RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    const int tcnt = shape->getIntersectionCount();
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
            &(shape->getBoundingSphereCenter()),
            shape->getBoundingSphereRadius())) {
        return 0;
    }

    // Calculate the initial point
    for (i = 0; i < shape->getUSteps(); i++) {
        for (j = 0; j < shape->getVSteps(); j++) {
            Vector3Dd **interpolatedGrid = shape->getInterpolatedGrid();
            Vector3Dd **interpolatedNormals = shape->getInterpolatedNormals();
            double **interpolatedD = shape->getInterpolatedD();

            // Grab precomputed surface values for the current patch
            v[0] = interpolatedGrid[i][j];
            v[1] = interpolatedGrid[i + 1][j];
            v[2] = interpolatedGrid[i][j + 1];
            v[3] = interpolatedGrid[i + 1][j + 1];

            // Check the ray against the bounding sphere for this subpatch
            ParametricBiCubicPatch::findAverage(4, &v[0], &center, &radius);
            if (!ParametricBiCubicIntersection::sphericalBoundsCheck(
                    ray, &center, radius)) {
                continue;
            }

            n = interpolatedNormals[i][2 * j];
            if (n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) {
                goto l0;
            }
            d = interpolatedD[i][2 * j];

            // Check for intersections in this subpatch
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->getPatchType(), ray, &v[0], &v[2], &v[1], &n, d, nullptr,
                    nullptr, nullptr, &depth, &ip, &n)) {
                shape->getIntersectionPointAt(tcnt + cnt) = ip;
                shape->getNormalVectorAt(tcnt + cnt) = n;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
                    // Too many intersections. Stop looking for more
                    return cnt;
                }
            }
        l0:
            n = interpolatedNormals[i][2 * j + 1];
            if (n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) {
                continue;
            }
            d = interpolatedD[i][2 * j + 1];
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->getPatchType(), ray, &v[1], &v[2], &v[3], &n, d, nullptr,
                    nullptr, nullptr, &depth, &ip, &n)) {
                shape->getIntersectionPointAt(tcnt + cnt) = ip;
                shape->getNormalVectorAt(tcnt + cnt) = n;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
                    // Too many intersections. Stop looking for more
                    return cnt;
                }
            }
        }
    }
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch2(
    const RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    double uValues[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    double vValues[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd(* const patch)[4][4] =
        (Vector3Dd(*)[4][4])shape->getControlPoints();

    ParametricBiCubicPatch::parametricSubdivider(ray, shape, patch, 0.0, 1.0,
        0.0, 1.0, 0, &cnt, depths, &uValues[0], &vValues[0]);
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch3(
    const RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    ParametricBiCubicPatch::parametricTreeWalker(
        ray, shape, shape->getNodeTree(), 0, &cnt, depths);
    return cnt;
}

int
ParametricBiCubicSolver::intersectParametricBiCubicPatch4(
    const RayWithSegments *ray, ParametricBiCubicPatch *shape, double *depths)
{
    int cnt = 0;
    const int tcnt = shape->getIntersectionCount();
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
            &(shape->getBoundingSphereCenter()),
            shape->getBoundingSphereRadius())) {
        return 0;
    }

    // Calculate the initial point
    for (i = 0; i < shape->getUSteps(); i++) {
        for (j = 0; j < shape->getVSteps(); j++) {
            Vector3Dd **interpolatedGrid = shape->getInterpolatedGrid();
            Vector3Dd **smoothNormals = shape->getSmoothNormals();
            Vector3Dd **interpolatedNormals = shape->getInterpolatedNormals();
            double **interpolatedD = shape->getInterpolatedD();

            // Grab precomputed surface values for the current patch
            v0 = interpolatedGrid[i][j];
            v1 = interpolatedGrid[i + 1][j];
            v2 = interpolatedGrid[i][j + 1];
            v3 = interpolatedGrid[i + 1][j + 1];

            n0 = smoothNormals[i][j];
            n1 = smoothNormals[i + 1][j];
            n2 = smoothNormals[i][j + 1];
            n3 = smoothNormals[i + 1][j + 1];

            n = interpolatedNormals[i][2 * j];
            if (n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) {
                goto l0;
            }
            d = interpolatedD[i][2 * j];

            // Make sure the smooth normals point in the same direction as the
            // normal
            t = n0.dotProduct(n);
            if (t < 0) {
                n0 = n0.multiply(-1.0);
            }
            t = n1.dotProduct(n);
            if (t < 0) {
                n1 = n1.multiply(-1.0);
            }
            t = n2.dotProduct(n);
            if (t < 0) {
                n2 = n2.multiply(-1.0);
            }

            // Check for intersections in this subpatch
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->getPatchType(), ray, &v0, &v2, &v1, &n, d, &n0, &n2, &n1,
                    &depth, &ip, &ipNorm)) {
                shape->getIntersectionPointAt(tcnt + cnt) = ip;
                shape->getNormalVectorAt(tcnt + cnt) = ipNorm;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
                    // Too many intersections. Stop looking for more
                    return cnt;
                }
            }
        l0:
            n = interpolatedNormals[i][2 * j + 1];
            if (n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) {
                continue;
            }
            d = interpolatedD[i][2 * j + 1];

            // Make sure the smooth normals point in the same direction as the
            // normal
            t = n1.dotProduct(n);
            if (t > 0) {
                n1 = n0.multiply(-1.0);
            }
            t = n2.dotProduct(n);
            if (t > 0) {
                n2 = n1.multiply(-1.0);
            }
            t = n3.dotProduct(n);
            if (t > 0) {
                n3 = n2.multiply(-1.0);
            }

            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->getPatchType(), ray, &v1, &v2, &v3, &n, d, &n1, &n2, &n3,
                    &depth, &ip, &ipNorm)) {
                shape->getIntersectionPointAt(tcnt + cnt) = ip;
                shape->getNormalVectorAt(tcnt + cnt) = ipNorm;
                depths[cnt] = depth;
                if (tcnt + ++cnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
                    // Too many intersections. Stop looking for more
                    return cnt;
                }
            }
        }
    }
    return cnt;
}

int
ParametricBiCubicSolver::allParametricBiCubicPatchIntersections(
    BoundedGeometry *object, RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    ParametricBiCubicPatch * const shape = (ParametricBiCubicPatch *)object;
    double depths[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    Intersection localElement;
    int cnt = 0;
    int tcnt;
    int i;
    int intersectionFound = 0;
    Statistics &stats = *ray->getStatistics();
    stats.incrementRayBicubicTests();
    if (ray->isPrimaryRayEnabled()) {
        shape->setIntersectionCount(0);
    }
    tcnt = shape->getIntersectionCount();
    if (shape->getPatchType() == 0) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch0(
            ray, shape, &depths[0]);
    } else if (shape->getPatchType() == 1) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch1(
            ray, shape, &depths[0]);
    } else if (shape->getPatchType() == 2) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch2(
            ray, shape, &depths[0]);
    } else if (shape->getPatchType() == 3) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch3(
            ray, shape, &depths[0]);
    } else if (shape->getPatchType() == 4) {
        cnt = ParametricBiCubicSolver::intersectParametricBiCubicPatch4(
            ray, shape, &depths[0]);
    } else {
        Logger::reportMessage("ParametricBiCubicSolver", Logger::FATAL_ERROR, "", "Bad patch type\n");
    }
    if (cnt > 0) {
        stats.incrementRayBicubicTestsSucceeded();
    }
    for (i = 0; i < cnt; i++) {
        if (!ray->isShadowRayEnabled()) {
            shape->incrementIntersectionCount();
        }
        localElement.setT(depths[i]);
        localElement.setBoundedGeometry(nullptr);
        localElement.setPoint(shape->getIntersectionPointAt(tcnt + i));
        localElement.setSimpleBody(reinterpret_cast<SimpleBody *>(shape));
        depthQueue->offer(localElement);
        intersectionFound = 1;
    }
    return (intersectionFound);
}
#include "java/util/PriorityQueue.txx"
