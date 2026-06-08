/****************************************************************************
 *                         bezier.c
 *
 *  This module implements the code for Bezier bicubic patch shapes
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  bezier bicubic patches and generously provided us these enhancements.
 *
 *****************************************************************************/

#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "common/logger/Logger.h"
#include "environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicIntersection.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicSolver.h"
#undef EPSILON
static constexpr double EPSILON = 1.0e-10;

Methods ParametricBiCubicPatch::methodTable = {
    ParametricBiCubicSolver::allParametricBiCubicPatchIntersections,
    ParametricBiCubicPatch::insideBicubicPatch,
    ParametricBiCubicPatch::bicubicPatchNormal,
    ParametricBiCubicPatch::copyBicubicPatch,
    ParametricBiCubicPatch::translateBicubicPatch,
    ParametricBiCubicPatch::rotateBicubicPatch,
    ParametricBiCubicPatch::scaleBicubicPatch,
    ParametricBiCubicPatch::invertBicubicPatch};


int maxDepthReached;

ParametricPatchNode *
ParametricBiCubicPatch::createNewParametricPatchNode()
{
    ParametricPatchNode *node = new ParametricPatchNode();
    if (node == nullptr) {
        Logger::info("Failed to allocate Bezier node\n");
        exit(0);
    }
    node->Data_Ptr = nullptr;
    return node;
}

ParametricControlPoints *
ParametricBiCubicPatch::createParametricControlPointsBlock()
{
    ParametricControlPoints *vertices = new ParametricControlPoints();
    if (vertices == nullptr) {
        Logger::info("Failed to allocate Bezier vertices\n");
        exit(0);
    }
    return vertices;
}

ParametricPatchChild *
ParametricBiCubicPatch::createParametricPatchChildBlock()
{
    ParametricPatchChild *children = new ParametricPatchChild();
    if (children == nullptr) {
        Logger::info("Failed to allocate Bezier children\n");
        exit(0);
    }
    return children;
}

ParametricPatchNode *
ParametricBiCubicPatch::parametricTreeBuilder(
    ParametricBiCubicPatch *object, Vector3Dd (*patch)[4][4], int depth)
{
    Vector3Dd lowerLeft[4][4];
    Vector3Dd lowerRight[4][4];
    Vector3Dd upperLeft[4][4];
    Vector3Dd upperRight[4][4];
    ParametricPatchChild *children;
    ParametricControlPoints *vertices;
    ParametricPatchNode *node =
        ParametricBiCubicPatch::createNewParametricPatchNode();

    if (depth > maxDepthReached) {
        maxDepthReached = depth;
    }

    /* Build the bounding sphere for this subpatch */
    ParametricBiCubicPatch::parametricBoundingSphere(
        patch, &(node->Center), &(node->radiusSquared));

    /* If the patch is close to being flat, then just perform a ray-plane
        intersection test. */
    if (ParametricBiCubicPatch::flatEnough(object, patch)) {
        /* The patch is now flat enough to simply store the corners */
        node->nodeType = ParametricPatchConstants::PARAMETRIC_LEAF_NODE;
        vertices = ParametricBiCubicPatch::createParametricControlPointsBlock();
        vertices->Vertices[0] = (*patch)[0][0];
        vertices->Vertices[1] = (*patch)[0][3];
        vertices->Vertices[2] = (*patch)[3][3];
        vertices->Vertices[3] = (*patch)[3][0];
        node->Data_Ptr = (void *)vertices;
    } else if (depth >= object->uSteps) {
        if (depth >= object->vSteps) {
            /* We are at the max recursion depth. Just store corners. */
            node->nodeType = ParametricPatchConstants::PARAMETRIC_LEAF_NODE;
            vertices =
                ParametricBiCubicPatch::createParametricControlPointsBlock();
            vertices->Vertices[0] = (*patch)[0][0];
            vertices->Vertices[1] = (*patch)[0][3];
            vertices->Vertices[2] = (*patch)[3][3];
            vertices->Vertices[3] = (*patch)[3][0];
            node->Data_Ptr = (void *)vertices;
        } else {
            ParametricBiCubicPatch::parametricSplitUpDown(patch,
                (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])upperLeft);
            node->nodeType = ParametricPatchConstants::PARAMETRIC_INTERIOR_NODE;
            children =
                ParametricBiCubicPatch::createParametricPatchChildBlock();
            children->Children[0] =
                ParametricBiCubicPatch::parametricTreeBuilder(
                    object, (Vector3Dd(*)[4][4])lowerLeft, depth + 1);
            children->Children[1] =
                ParametricBiCubicPatch::parametricTreeBuilder(
                    object, (Vector3Dd(*)[4][4])upperLeft, depth + 1);
            node->Count = 2;
            node->Data_Ptr = (void *)children;
        }
    } else if (depth >= object->vSteps) {
        ParametricBiCubicPatch::parametricSplitLeftRight(patch,
            (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])lowerRight);
        node->nodeType = ParametricPatchConstants::PARAMETRIC_INTERIOR_NODE;
        children = ParametricBiCubicPatch::createParametricPatchChildBlock();
        children->Children[0] = ParametricBiCubicPatch::parametricTreeBuilder(
            object, (Vector3Dd(*)[4][4])lowerLeft, depth + 1);
        children->Children[1] = ParametricBiCubicPatch::parametricTreeBuilder(
            object, (Vector3Dd(*)[4][4])lowerRight, depth + 1);
        node->Count = 2;
        node->Data_Ptr = (void *)children;
    } else {
        ParametricBiCubicPatch::parametricSplitLeftRight(patch,
            (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])lowerRight);
        ParametricBiCubicPatch::parametricSplitUpDown(
            (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])lowerLeft,
            (Vector3Dd(*)[4][4])upperLeft);
        ParametricBiCubicPatch::parametricSplitUpDown(
            (Vector3Dd(*)[4][4])lowerRight, (Vector3Dd(*)[4][4])lowerRight,
            (Vector3Dd(*)[4][4])upperRight);
        node->nodeType = ParametricPatchConstants::PARAMETRIC_INTERIOR_NODE;
        children = ParametricBiCubicPatch::createParametricPatchChildBlock();
        children->Children[0] = ParametricBiCubicPatch::parametricTreeBuilder(
            object, (Vector3Dd(*)[4][4])lowerLeft, depth + 1);
        children->Children[1] = ParametricBiCubicPatch::parametricTreeBuilder(
            object, (Vector3Dd(*)[4][4])upperLeft, depth + 1);
        children->Children[2] = ParametricBiCubicPatch::parametricTreeBuilder(
            object, (Vector3Dd(*)[4][4])lowerRight, depth + 1);
        children->Children[3] = ParametricBiCubicPatch::parametricTreeBuilder(
            object, (Vector3Dd(*)[4][4])upperRight, depth + 1);
        node->Count = 4;
        node->Data_Ptr = (void *)children;
    }
    return node;
}

/* Evaluate a single coordinate point (u, v) on a bezier patch. */
void
ParametricBiCubicPatch::parametricValue(
    Vector3Dd *result, double u, double v, Vector3Dd (*controlPoints)[4][4])
{
    double u2;
    double u3;
    double v2;
    double v3;
    double uu1;
    double uu2;
    double uu3;
    double vv1;
    double vv2;
    double vv3;
    double t[4][4];
    int i;
    int j;

    u2 = u * u;
    u3 = u * u2;
    v2 = v * v;
    v3 = v * v2;
    uu1 = 1.0 - u;
    uu2 = uu1 * uu1;
    uu3 = uu1 * uu2;
    vv1 = 1.0 - v;
    vv2 = vv1 * vv1;
    vv3 = vv1 * vv2;
    t[0][0] = uu3 * vv3;
    t[0][1] = 3.0 * uu3 * v * vv2;
    t[0][2] = 3.0 * uu3 * v2 * vv1;
    t[0][3] = uu3 * v3;
    t[1][0] = 3.0 * u * uu2 * vv3;
    t[1][1] = 9.0 * u * uu2 * v * vv2;
    t[1][2] = 9.0 * u * uu2 * v2 * vv1;
    t[1][3] = 3.0 * u * uu2 * v3;
    t[2][0] = 3.0 * u2 * uu1 * vv3;
    t[2][1] = 9.0 * u2 * uu1 * v * vv2;
    t[2][2] = 9.0 * u2 * uu1 * v2 * vv1;
    t[2][3] = 3.0 * u2 * uu1 * v3;
    t[3][0] = u3 * vv3;
    t[3][1] = 3.0 * u3 * v * vv2;
    t[3][2] = 3.0 * u3 * v2 * vv1;
    t[3][3] = u3 * v3;

    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            rx += t[i][j] * (*controlPoints)[i][j].x();
            ry += t[i][j] * (*controlPoints)[i][j].y();
            rz += t[i][j] * (*controlPoints)[i][j].z();
        }
    }
    *result = Vector3Dd(rx, ry, rz);
}

/* Calculate the normal to a bezier patch for a particular axis, at
    a particular point on the patch.

    The normal at a point of a parametric surface z = f(u, v) is:

        (|[[dy/du, dy/dv],[dz/du, dz/dv]]|,
         |[[dz/du, dz/dv],[dx/du, dx/dv]]|,
         |[[dx/du, dx/dv],[dy/du, dy/dv]]|)

    The normal is undefined where the determinants vanish.
*/
void
ParametricBiCubicPatch::parametricPartial(
    Vector3Dd *result, double u, double v, ParametricBiCubicPatch *shape)
{
    Vector3Dd uVec;
    Vector3Dd vVec; /* Partial derivatives with respect to u, and v. */
    double u2;
    double u3;
    double v2;
    double v3;
    double t[4][4];
    double temp;
    int i;
    int j;

    u2 = u * u;
    u3 = u * u2;
    v2 = v * v;
    v3 = v * v2;

    /* Calculate the derivative with respect to u */
    t[0][0] = 3.0 * (v3 - 3.0 * v2 + 3.0 * v - 1.0) * (u2 - 2.0 * u + 1.0);
    t[0][1] = 9.0 * v * (v2 - 2.0 * v + 1.0) * (u2 - 2.0 * u + 1.0);
    t[0][2] = 9.0 * v2 * (v - 1.0) * (u2 - 2.0 * u + 1.0);
    t[0][3] = 3.0 * v3 * (u2 - 2.0 * u + 1.0);
    t[1][0] = 3.0 * (v3 - 3.0 * v2 + 3.0 * v - 1) * (3.0 * u2 - 4.0 * u + 1);
    t[1][1] = 9.0 * v * (v2 - 2.0 * v + 1.0) * (3.0 * u2 - 4.0 * u + 1.0);
    t[1][2] = 9.0 * v2 * (v - 1.0) * (3.0 * u2 - 4.0 * u + 1.0);
    t[1][3] = 3.0 * v3 * (3.0 * u2 - 4.0 * u + 1.0);
    t[2][0] = 3.0 * u * (v3 - 3.0 * v2 + 3.0 * v - 1.0) * (3.0 * u - 2.0);
    t[2][1] = 9.0 * u * v * (v2 - 2.0 * v + 1.0) * (3.0 * u - 2.0);
    t[2][2] = 9.0 * u * v2 * (v - 1.0) * (3.0 * u - 2.0);
    t[2][3] = 3.0 * u * v3 * (3.0 * u - 2.0);
    t[3][0] = 3.0 * u2 * (v3 - 3.0 * v2 + 3.0 * v - 1.0);
    t[3][1] = 9.0 * u2 * v * (v2 - 2.0 * v + 1.0);
    t[3][2] = 9.0 * u2 * v2 * (v - 1.0);
    t[3][3] = 3.0 * u2 * v3;
    double ux = 0.0;
    double uy = 0.0;
    double uz = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            ux += t[i][j] * shape->Control_Points[i][j].x();
            uy += t[i][j] * shape->Control_Points[i][j].y();
            uz += t[i][j] * shape->Control_Points[i][j].z();
        }
    }
    temp = ux * ux + uy * uy + uz * uz;
    if (temp < EPSILON) {
        /* Partial with respect to u is undefined. */
        *result = Vector3Dd(1.0, 0.0, 0.0);
        /* *Result = *n; */
        return;
    }
    temp = sqrt(temp);
    uVec = Vector3Dd(ux / temp, uy / temp, uz / temp);

    /* Calculate the derivative with respect to v */
    t[0][0] = 3.0 * (v2 - 2.0 * v + 1.0) * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[0][1] =
        3.0 * (3.0 * v2 - 4.0 * v + 1.0) * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[0][2] = 3.0 * v * (3.0 * v - 2.0) * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[0][3] = 3.0 * v2 * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[1][0] = 9.0 * u * (v2 - 2.0 * v + 1.0) * (u2 - 2.0 * u + 1.0);
    t[1][1] = 9.0 * u * (3.0 * v2 - 4.0 * v + 1.0) * (u2 - 2.0 * u + 1.0);
    t[1][2] = 9.0 * u * v * (3.0 * v - 2.0) * (u2 - 2.0 * u + 1.0);
    t[1][3] = 9.0 * u * v2 * (u2 - 2.0 * u + 1.0);
    t[2][0] = 9.0 * u2 * (v2 - 2.0 * v + 1.0) * (u - 1.0);
    t[2][1] = 9.0 * u2 * (3.0 * v2 - 4.0 * v + 1.0) * (u - 1.0);
    t[2][2] = 9.0 * u2 * v * (3.0 * v - 2.0) * (u - 1.0);
    t[2][3] = 9.0 * u2 * v2 * (u - 1.0);
    t[3][0] = 3.0 * u3 * (v2 - 2.0 * v + 1.0);
    t[3][1] = 3.0 * u3 * (3.0 * v2 - 4.0 * v + 1.0);
    t[3][2] = 3.0 * u3 * v * (3.0 * v - 2.0);
    t[3][3] = 3.0 * u3 * v2;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            vx += t[i][j] * shape->Control_Points[i][j].x();
            vy += t[i][j] * shape->Control_Points[i][j].y();
            vz += t[i][j] * shape->Control_Points[i][j].z();
        }
    }
    temp = vx * vx + vy * vy + vz * vz;
    if (temp < EPSILON) {
        /* Partial with respect to u is undefined. */
        *result = Vector3Dd(1.0, 0.0, 0.0);
        return;
    }
    temp = sqrt(temp);
    vVec = Vector3Dd(vx / temp, vy / temp, vz / temp);

    *result = uVec.crossProduct(vVec);
}

/* Find a sphere that contains all of the points in the list "vectors" */
void
ParametricBiCubicPatch::findAverage(
    int vectorCount, Vector3Dd *vectors, Vector3Dd *center, double *radius)
{
    double r0;
    double r1;
    double xc = 0;
    double yc = 0;
    double zc = 0;
    double x0;
    double y0;
    double z0;
    int i;
    for (i = 0; i < vectorCount; i++) {
        xc += vectors[i].x();
        yc += vectors[i].y();
        zc += vectors[i].z();
    }
    xc /= (double)vectorCount;
    yc /= (double)vectorCount;
    zc /= (double)vectorCount;
    r0 = 0.0;
    for (i = 0; i < vectorCount; i++) {
        x0 = vectors[i].x() - xc;
        y0 = vectors[i].y() - yc;
        z0 = vectors[i].z() - zc;
        r1 = x0 * x0 + y0 * y0 + z0 * z0;
        if (r1 > r0) {
            r0 = r1;
        }
    }
    *center = Vector3Dd(xc, yc, zc);
    *radius = r0;
}

/* Find a sphere that bounds all of the control points of a Bezier patch.
    The values returned are: the center of the bounding sphere, and the
    square of the radius of the bounding sphere. */
void
ParametricBiCubicPatch::parametricBoundingSphere(
    Vector3Dd (*patch)[4][4], Vector3Dd *center, double *radius)
{
    double r0;
    double r1;
    double xc = 0;
    double yc = 0;
    double zc = 0;
    double x0;
    double y0;
    double z0;
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            xc += (*patch)[i][j].x();
            yc += (*patch)[i][j].y();
            zc += (*patch)[i][j].z();
        }
    }
    xc /= 16.0;
    yc /= 16.0;
    zc /= 16.0;
    r0 = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            x0 = (*patch)[i][j].x() - xc;
            y0 = (*patch)[i][j].y() - yc;
            z0 = (*patch)[i][j].z() - zc;
            r1 = x0 * x0 + y0 * y0 + z0 * z0;
            if (r1 > r0) {
                r0 = r1;
            }
        }
    }
    *center = Vector3Dd(xc, yc, zc);
    *radius = r0;
}

/* Precompute grid points and normals for a bezier patch */
void
ParametricBiCubicPatch::precomputePatchValues(ParametricBiCubicPatch *shape)
{
    int i;
    int j;
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
    Vector3Dd controlPoints[16];
    Vector3Dd(*patchPtr)[4][4] = (Vector3Dd(*)[4][4])shape->Control_Points;

    /* Calculate the bounding sphere for the entire patch. */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            controlPoints[4 * i + j] = shape->Control_Points[i][j];
        }
    }
    ParametricBiCubicPatch::findAverage(16, &controlPoints[0],
        &shape->boundingSphereCenter, &shape->boundingSphereRadius);
    /* Shape->Node_Tree = NULL; */
    if (shape->patchType == 0 || shape->patchType == 2) {
        return;
    }
    if (shape->patchType == 3) {
        if (shape->Node_Tree != nullptr) {
            ParametricBiCubicPatch::parametricTreeDeleter(shape->Node_Tree);
        }
        shape->Node_Tree =
            ParametricBiCubicPatch::parametricTreeBuilder(shape, patchPtr, 0);
        return;
    }
    deltaU = 1.0 / (double)shape->uSteps;
    deltaV = 1.0 / (double)shape->vSteps;
    if (shape->Interpolated_Grid == nullptr) {
        shape->Interpolated_Grid = new Vector3Dd *[shape->uSteps + 1];
        if (shape->Interpolated_Grid == nullptr) {
            Logger::error("Failed to allocate Interpolated_Grid");
            exit(1);
        }
        for (i = 0; i <= shape->uSteps; i++) {
            shape->Interpolated_Grid[i] = new Vector3Dd[shape->vSteps + 1];
            if (shape->Interpolated_Grid == nullptr) {
                Logger::error(
                    "Failed to allocate component of Interpolated_Grid");
                exit(1);
            }
        }
        shape->Interpolated_Normals = new Vector3Dd *[shape->uSteps + 1];
        if (shape->Interpolated_Normals == nullptr) {
            Logger::error(
                "Failed to allocate Interpolated_Normals");
            exit(1);
        }
        for (i = 0; i <= shape->uSteps; i++) {
            shape->Interpolated_Normals[i] =
                new Vector3Dd[2 * (shape->vSteps + 1)];
            if (shape->Interpolated_Normals == nullptr) {
                Logger::error(
                    "Failed to allocate component of Interpolated_Normals");
                exit(1);
            }
        }

        if (shape->patchType == 4) {
            shape->Smooth_Normals = new Vector3Dd *[shape->uSteps + 1];
            if (shape->Smooth_Normals == nullptr) {
                Logger::error("Failed to allocate Smooth_Normals");
                exit(1);
            }
            for (i = 0; i <= shape->uSteps; i++) {
                shape->Smooth_Normals[i] = new Vector3Dd[shape->vSteps + 1];
                if (shape->Smooth_Normals == nullptr) {
                    Logger::error(
                        "Failed to allocate component of Smooth_Normals");
                    exit(1);
                }
            }
        }

        shape->Interpolated_D = new double *[shape->uSteps + 1];
        if (shape->Interpolated_D == nullptr) {
            Logger::error("Failed to allocate Interpolated_D");
            exit(1);
        }
        for (i = 0; i <= shape->uSteps; i++) {
            shape->Interpolated_D[i] = new double[2 * (shape->vSteps + 1)];
            if (shape->Interpolated_D == nullptr) {
                Logger::error(
                    "Failed to allocate component of Interpolated_D");
                exit(1);
            }
        }
    }

    /* Calculate the grid values for the given subdivision values. */
    for (i = 0; i <= shape->uSteps; i++) {
        u = (double)i / (double)shape->uSteps;
        for (j = 0; j < shape->vSteps; j++) {
            v = (double)j / (double)shape->vSteps;
            ParametricBiCubicPatch::parametricValue(
                &shape->Interpolated_Grid[i][j], u, v, patchPtr);
        }
    }

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

            shape->Interpolated_Grid[i][j] = v0;
            shape->Interpolated_Grid[i + 1][j] = v1;
            shape->Interpolated_Grid[i][j + 1] = v2;
            shape->Interpolated_Grid[i + 1][j + 1] = v3;
            if (shape->patchType == 1 || shape->patchType == 4) {
                /* Calculate the normals */
                if (ParametricBiCubicIntersection::subpatchNormal(
                        &v0, &v2, &v1, &n, &d)) {
                    shape->Interpolated_Normals[i][2 * j] = n;
                    shape->Interpolated_D[i][2 * j] = d;
                } else {
                    shape->Interpolated_Normals[i][2 * j] = Vector3Dd(0.0, 0.0, 0.0);
                    shape->Interpolated_D[i][2 * j] = 0.0;
                }

                if (ParametricBiCubicIntersection::subpatchNormal(
                        &v1, &v2, &v3, &n, &d)) {
                    shape->Interpolated_Normals[i][2 * j + 1] = n;
                    shape->Interpolated_D[i][2 * j + 1] = d;
                } else {
                    shape->Interpolated_Normals[i][2 * j + 1] =
                        Vector3Dd(0.0, 0.0, 0.0);
                    shape->Interpolated_D[i][2 * j + 1] = 0.0;
                }
            }
        }
    }

    if (shape->patchType == 4) {
        /* Calculate normals at the corners of the subpatches */
        for (i = 0; i <= shape->uSteps; i++) {
            u = (double)i / (double)shape->uSteps;
            for (j = 0; j <= shape->vSteps; j++) {
                v = (double)j / (double)shape->vSteps;
                ParametricBiCubicPatch::parametricPartial(
                    &shape->Smooth_Normals[i][j], u, v, shape);
            }
        }
    }
}

void
ParametricBiCubicPatch::parametricSubpatchIntersect(RayWithSegments *ray,
    ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4], double u0,
    double u1, double v0, int recursionDepth, int *depthCount, double *depths,
    double *uValues, double *vValues)
{
    int tcnt = shape->intersectionCount;
    Vector3Dd vv0;
    Vector3Dd vv1;
    Vector3Dd vv2;
    Vector3Dd vv3;
    Vector3Dd n;
    Vector3Dd ip;
    double d;
    double depth;

    if (tcnt + *depthCount >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    vv0 = (*patch)[0][0];
    vv1 = (*patch)[0][3];
    vv2 = (*patch)[3][3];
    vv3 = (*patch)[3][0];

    /* Triangulate this subpatch, then check for intersections in
        the triangles. */
    if (ParametricBiCubicIntersection::subpatchNormal(
            &vv0, &vv1, &vv2, &n, &d)) {
        if (ParametricBiCubicIntersection::intersectSubpatch(shape->patchType,
                ray, &vv0, &vv1, &vv2, &n, d, nullptr, nullptr, nullptr, &depth,
                &ip, &n)) {
            shape->Intersection_Point[tcnt + *depthCount] = ip;
            shape->normalVector[tcnt + *depthCount] = n;
            depths[*depthCount] = depth;
            *depthCount += 1;
        }
    }

    if (*depthCount + tcnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    if (ParametricBiCubicIntersection::subpatchNormal(
            &vv0, &vv2, &vv3, &n, &d)) {
        if (ParametricBiCubicIntersection::intersectSubpatch(shape->patchType,
                ray, &vv0, &vv2, &vv3, &n, d, nullptr, nullptr, nullptr, &depth,
                &ip, &n)) {
            shape->Intersection_Point[tcnt + *depthCount] = ip;
            shape->normalVector[tcnt + *depthCount] = n;
            depths[*depthCount] = depth;
            *depthCount += 1;
        }
    }
}

void
ParametricBiCubicPatch::parametricSplitLeftRight(Vector3Dd (*patch)[4][4],
    Vector3Dd (*leftPatch)[4][4], Vector3Dd (*rightPatch)[4][4])
{
    Vector3Dd temp1[4];
    Vector3Dd temp2[4];
    Vector3Dd half;
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        temp1[0] = (*patch)[i][0];
        temp1[1] = ((*patch)[i][0]).midpoint((*patch)[i][1]);
        half = ((*patch)[i][1]).midpoint((*patch)[i][2]);
        temp1[2] = temp1[1].midpoint(half);
        temp2[2] = ((*patch)[i][2]).midpoint((*patch)[i][3]);
        temp2[1] = half.midpoint(temp2[2]);
        temp1[3] = temp1[2].midpoint(temp2[1]);
        temp2[0] = temp1[3];
        temp2[3] = (*patch)[i][3];
        for (j = 0; j < 4; j++) {
            (*leftPatch)[i][j] = temp1[j];
            (*rightPatch)[i][j] = temp2[j];
        }
    }
}

void
ParametricBiCubicPatch::parametricSplitUpDown(Vector3Dd (*patch)[4][4],
    Vector3Dd (*topPatch)[4][4], Vector3Dd (*bottomPatch)[4][4])
{
    Vector3Dd temp1[4];
    Vector3Dd temp2[4];
    Vector3Dd half;
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        /* Split Left */
        temp1[0] = (*patch)[0][i];
        temp1[1] = ((*patch)[0][i]).midpoint((*patch)[1][i]);
        half = ((*patch)[1][i]).midpoint((*patch)[2][i]);
        temp1[2] = temp1[1].midpoint(half);
        temp2[2] = ((*patch)[2][i]).midpoint((*patch)[3][i]);
        temp2[1] = half.midpoint(temp2[2]);
        temp1[3] = temp1[2].midpoint(temp2[1]);
        temp2[0] = temp1[3];
        temp2[3] = (*patch)[3][i];
        for (j = 0; j < 4; j++) {
            (*bottomPatch)[j][i] = temp1[j];
            (*topPatch)[j][i] = temp2[j];
        }
    }
}

/* See how close to a plane a subpatch is, the patch must have at least
    three distinct vertices. A negative result from this function indicates
    that a degenerate value of some sort was encountered. */
double
ParametricBiCubicPatch::determineSubpatchFlatness(Vector3Dd (*patch)[4][4])
{
    Vector3Dd vertices[4];
    Vector3Dd n;
    Vector3Dd tempV;
    double d;
    double dist;
    double temp1;
    int i;
    int j;

    vertices[0] = (*patch)[0][0];
    vertices[1] = (*patch)[0][3];
    tempV = vertices[0].subtract(vertices[1]);
    temp1 = tempV.length();
    if (fabs(temp1) < EPSILON) {
        /* Degenerate in the V direction for U = 0. This is ok if the other
            two corners are distinct from the lower left corner - I'm sure there
            are cases where the corners coincide and the middle has good values,
            but that is somewhat pathalogical and won't be considered. */
        vertices[1] = (*patch)[3][3];
        tempV = vertices[0].subtract(vertices[1]);
        temp1 = tempV.length();
        if (fabs(temp1) < EPSILON) {
            return -1.0;
        }
        vertices[2] = (*patch)[3][0];
        tempV = vertices[0].subtract(vertices[1]);
        temp1 = tempV.length();
        if (fabs(temp1) < EPSILON) {
            return -1.0;
        }
        tempV = vertices[1].subtract(vertices[2]);
        temp1 = tempV.length();
        if (fabs(temp1) < EPSILON) {
            return -1.0;
        }
    } else {
        vertices[2] = (*patch)[3][0];
        tempV = vertices[0].subtract(vertices[1]);
        temp1 = tempV.length();
        if (fabs(temp1) < EPSILON) {
            vertices[2] = (*patch)[3][3];
            tempV = vertices[0].subtract(vertices[2]);
            temp1 = tempV.length();
            if (fabs(temp1) < EPSILON) {
                return -1.0;
            }
            tempV = vertices[1].subtract(vertices[2]);
            temp1 = tempV.length();
            if (fabs(temp1) < EPSILON) {
                return -1.0;
            }
        } else {
            tempV = vertices[1].subtract(vertices[2]);
            temp1 = tempV.length();
            if (fabs(temp1) < EPSILON) {
                return -1.0;
            }
        }
    }
    /* Now that a good set of candidate points has been found, find the
        plane equations for the patch */
    if (ParametricBiCubicIntersection::subpatchNormal(
            &vertices[0], &vertices[1], &vertices[2], &n, &d)) {
        /* Step through all vertices and see what the maximum distance from the
                 plane happens to be. */
        dist = 0.0;
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                temp1 = fabs(ParametricBiCubicIntersection::pointPlaneDistance(
                    &((*patch)[i][j]), &n, &d));
                if (temp1 > dist) {
                    dist = temp1;
                }
            }
        }
        return dist;
    }
    /* Logger::info("Subpatch normal failed in determine_subpatch_flatness\n"); */
    return -1.0;
}

int
ParametricBiCubicPatch::flatEnough(
    ParametricBiCubicPatch *object, Vector3Dd (*patch)[4][4])
{
    double dist;

    dist = ParametricBiCubicPatch::determineSubpatchFlatness(patch);
    if (dist < 0.0) {
        return 0;
    }
    if (dist < object->flatnessValue) {
        return 1;
    }
    return 0;
}

void
ParametricBiCubicPatch::parametricSubdivider(RayWithSegments *ray,
    ParametricBiCubicPatch *object, Vector3Dd (*patch)[4][4], double u0,
    double u1, double v0, double v1, int recursionDepth, int *depthCount,
    double *depths, double *uValues, double *vValues)
{
    Vector3Dd lowerLeft[4][4];
    Vector3Dd lowerRight[4][4];
    Vector3Dd upperLeft[4][4];
    Vector3Dd upperRight[4][4];
    Vector3Dd center;
    double ut;
    double vt;
    double radius;
    int tcnt = object->intersectionCount;

    /* Don't waste time if there are already too many intersections */
    if (tcnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    /* Make sure the ray passes through a sphere bounding the control points of
        the patch */
    ParametricBiCubicPatch::parametricBoundingSphere(patch, &center, &radius);
    if (!ParametricBiCubicIntersection::sphericalBoundsCheck(
            ray, &center, radius)) {
        return;
    }

    /* If the patch is close to being flat, then just perform a ray-plane
        intersection test. */
    if (ParametricBiCubicPatch::flatEnough(object, patch)) {
        ParametricBiCubicPatch::parametricSubpatchIntersect(ray, object, patch,
            u0, u1, v0, recursionDepth + 1, depthCount, depths, uValues,
            vValues);
    }

    if (recursionDepth >= object->uSteps) {
        if (recursionDepth >= object->vSteps) {
            ParametricBiCubicPatch::parametricSubpatchIntersect(ray, object,
                patch, u0, u1, v0, recursionDepth + 1, depthCount, depths,
                uValues, vValues);
        } else {
            ParametricBiCubicPatch::parametricSplitUpDown(patch,
                (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])upperLeft);
            vt = (v1 - v0) / 2.0;
            ParametricBiCubicPatch::parametricSubdivider(ray, object,
                (Vector3Dd(*)[4][4])lowerLeft, u0, u1, v0, vt,
                recursionDepth + 1, depthCount, depths, uValues, vValues);
            ParametricBiCubicPatch::parametricSubdivider(ray, object,
                (Vector3Dd(*)[4][4])upperLeft, u0, u1, vt, v1,
                recursionDepth + 1, depthCount, depths, uValues, vValues);
        }
    } else if (recursionDepth >= object->vSteps) {
        ParametricBiCubicPatch::parametricSplitLeftRight(patch,
            (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])lowerRight);
        ut = (u1 - u0) / 2.0;
        ParametricBiCubicPatch::parametricSubdivider(ray, object,
            (Vector3Dd(*)[4][4])lowerLeft, u0, ut, v0, v1, recursionDepth + 1,
            depthCount, depths, uValues, vValues);
        ParametricBiCubicPatch::parametricSubdivider(ray, object,
            (Vector3Dd(*)[4][4])lowerRight, ut, u1, v0, v1, recursionDepth + 1,
            depthCount, depths, uValues, vValues);
    } else {
        ut = (u1 - u0) / 2.0;
        vt = (v1 - v0) / 2.0;
        ParametricBiCubicPatch::parametricSplitLeftRight(patch,
            (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])lowerRight);
        ParametricBiCubicPatch::parametricSplitUpDown(
            (Vector3Dd(*)[4][4])lowerLeft, (Vector3Dd(*)[4][4])lowerLeft,
            (Vector3Dd(*)[4][4])upperLeft);
        ParametricBiCubicPatch::parametricSplitUpDown(
            (Vector3Dd(*)[4][4])lowerRight, (Vector3Dd(*)[4][4])lowerRight,
            (Vector3Dd(*)[4][4])upperRight);
        ParametricBiCubicPatch::parametricSubdivider(ray, object,
            (Vector3Dd(*)[4][4])lowerLeft, u0, ut, v0, vt, recursionDepth + 1,
            depthCount, depths, uValues, vValues);
        ParametricBiCubicPatch::parametricSubdivider(ray, object,
            (Vector3Dd(*)[4][4])upperLeft, u0, ut, vt, v1, recursionDepth + 1,
            depthCount, depths, uValues, vValues);
        ParametricBiCubicPatch::parametricSubdivider(ray, object,
            (Vector3Dd(*)[4][4])lowerRight, ut, u1, v0, vt, recursionDepth + 1,
            depthCount, depths, uValues, vValues);
        ParametricBiCubicPatch::parametricSubdivider(ray, object,
            (Vector3Dd(*)[4][4])upperRight, ut, u1, vt, v1, recursionDepth + 1,
            depthCount, depths, uValues, vValues);
    }
}

void
ParametricBiCubicPatch::parametricTreeDeleter(ParametricPatchNode *node)
{
    ParametricPatchChild *children;
    int i;

    /* If this is an interior node then continue the descent */
    if (node->nodeType == ParametricPatchConstants::PARAMETRIC_INTERIOR_NODE) {
        children = (ParametricPatchChild *)node->Data_Ptr;
        for (i = 0; i < node->Count; i++) {
            ParametricBiCubicPatch::parametricTreeDeleter(
                children->Children[i]);
        }
        delete (ParametricPatchChild *)children;
    } else if (node->nodeType == ParametricPatchConstants::PARAMETRIC_LEAF_NODE) {
        /* Free the memory used for the vertices. */
        delete (ParametricControlPoints *)node->Data_Ptr;
    }
    /* Free the memory used for the node. */
    delete node;
}

void
ParametricBiCubicPatch::parametricTreeWalker(RayWithSegments *ray,
    ParametricBiCubicPatch *shape, ParametricPatchNode *node, int depth,
    int *depthCount, double *depths)
{
    ParametricPatchChild *children;
    ParametricControlPoints *vertices;
    Vector3Dd n;
    Vector3Dd ip;
    Vector3Dd vv0;
    Vector3Dd vv1;
    Vector3Dd vv2;
    Vector3Dd vv3;
    double d;
    double hitDepth;
    int i;
    int tcnt = shape->intersectionCount;

    /* Don't waste time if there are already too many intersections */
    if (tcnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    /* Make sure the ray passes through a sphere bounding the control points of
        the patch */
    if (!ParametricBiCubicIntersection::sphericalBoundsCheck(
            ray, &(node->Center), node->radiusSquared)) {
        return;
    }

    /* If this is an interior node then continue the descent, else
        do a check against the vertices. */
    if (node->nodeType == ParametricPatchConstants::PARAMETRIC_INTERIOR_NODE) {
        children = (ParametricPatchChild *)node->Data_Ptr;
        for (i = 0; i < node->Count; i++) {
            ParametricBiCubicPatch::parametricTreeWalker(ray, shape,
                children->Children[i], depth + 1, depthCount, depths);
        }
    } else if (node->nodeType == ParametricPatchConstants::PARAMETRIC_LEAF_NODE) {
        vertices = (ParametricControlPoints *)node->Data_Ptr;
        vv0 = vertices->Vertices[0];
        vv1 = vertices->Vertices[1];
        vv2 = vertices->Vertices[2];
        vv3 = vertices->Vertices[3];

        /* Triangulate this subpatch, then check for intersections in
            the triangles. */
        if (ParametricBiCubicIntersection::subpatchNormal(
                &vv0, &vv1, &vv2, &n, &d)) {
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->patchType, ray, &vv0, &vv1, &vv2, &n, d, nullptr,
                    nullptr, nullptr, &hitDepth, &ip, &n)) {
                shape->Intersection_Point[tcnt + *depthCount] = ip;
                shape->normalVector[tcnt + *depthCount] = n;
                depths[*depthCount] = hitDepth;
                *depthCount += 1;
            }
        }

        if (*depthCount + tcnt >= ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS) {
            return;
        }

        if (ParametricBiCubicIntersection::subpatchNormal(
                &vv0, &vv2, &vv3, &n, &d)) {
            if (ParametricBiCubicIntersection::intersectSubpatch(
                    shape->patchType, ray, &vv0, &vv2, &vv3, &n, d, nullptr,
                    nullptr, nullptr, &hitDepth, &ip, &n)) {
                shape->Intersection_Point[tcnt + *depthCount] = ip;
                shape->normalVector[tcnt + *depthCount] = n;
                depths[*depthCount] = hitDepth;
                *depthCount += 1;
            }
        }
    } else {
        Logger::info("Bad Node type at depth %d\n", depth);
    }
}

/* A patch is not a solid, so an inside test doesn't make sense. */
int
ParametricBiCubicPatch::insideBicubicPatch(
    Vector3Dd *testPoint, SimpleBody *object)
{
    return 0;
}

void
ParametricBiCubicPatch::bicubicPatchNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    ParametricBiCubicPatch *patch = (ParametricBiCubicPatch *)object;
    int i;

    /* If all is going well, the normal was computed at the time the
       intersection was computed.  Look on the list of associated intersection
       points and normals */
    for (i = 0; i < patch->intersectionCount; i++) {
        if (intersectionPoint->x() == patch->Intersection_Point[i].x() &&
            intersectionPoint->y() == patch->Intersection_Point[i].y() &&
            intersectionPoint->z() == patch->Intersection_Point[i].z()) {
            *result = patch->normalVector[i];
            return;
        }
    }
    if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
        Logger::info("Bicubic patch normal for unknown intersection point\n");
        fflush(stdout);
    }
    *result = Vector3Dd(1.0, 0.0, 0.0);
}

void *
ParametricBiCubicPatch::copyBicubicPatch(SimpleBody *object)
{
    ParametricBiCubicPatch *newShape;

    newShape = new ParametricBiCubicPatch;
    *newShape = *((ParametricBiCubicPatch *)object);
    newShape->nextObject = nullptr;

    newShape->Interpolated_Grid = nullptr;
    ParametricBiCubicPatch::precomputePatchValues(newShape);
    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (void *)(newShape);
}

void
ParametricBiCubicPatch::translateBicubicPatch(
    SimpleBody *object, Vector3Dd *vector)
{
    ParametricBiCubicPatch *patch = (ParametricBiCubicPatch *)object;
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            patch->Control_Points[i][j] =
                patch->Control_Points[i][j].add(*vector);
        }
    }
    ParametricBiCubicPatch::precomputePatchValues(patch);
    TextureUtils::translateTexture(
        &((ParametricBiCubicPatch *)object)->Shape_Texture, vector);
}

void
ParametricBiCubicPatch::rotateBicubicPatch(
    SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;
    ParametricBiCubicPatch *patch = (ParametricBiCubicPatch *)object;
    int i;
    int j;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            patch->Control_Points[i][j] = transformation.transpose().multiply(
                patch->Control_Points[i][j]);
        }
    }
    ParametricBiCubicPatch::precomputePatchValues(patch);
    TextureUtils::rotateTexture(
        &((ParametricBiCubicPatch *)object)->Shape_Texture, vector);
}

void
ParametricBiCubicPatch::scaleBicubicPatch(SimpleBody *object, Vector3Dd *vector)
{
    ParametricBiCubicPatch *patch = (ParametricBiCubicPatch *)object;
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            patch->Control_Points[i][j] =
                patch->Control_Points[i][j].multiply(*vector);
        }
    }
    ParametricBiCubicPatch::precomputePatchValues(patch);
    TextureUtils::scaleTexture(
        &((ParametricBiCubicPatch *)object)->Shape_Texture, vector);
}

/* Inversion of a patch really doesn't make sense. */
void
ParametricBiCubicPatch::invertBicubicPatch(SimpleBody *object)
{
    ;
}
