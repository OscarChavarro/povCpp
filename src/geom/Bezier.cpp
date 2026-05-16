/****************************************************************************
 *                         bezier.c
 *
 *  This module implements the code for Bezier bicubic patch shapes
 *
 *  This file was written by Alexander Enzmann.  He wrote the code for
 *  bezier bicubic patches and generously provided us these enhancements.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "geom/Bezier.h"
#include "geom/Geometry.h"
#include "geom/Objects.h"

/*===========================================================================*/

#undef EPSILON
#ifndef EPSILON
static constexpr double EPSILON = 1.0e-10;
#endif

Methods Bicubic_Patch_Methods = {objectIntersect,
    allBicubicPatchIntersections, insideBicubicPatch, bicubicPatchNormal,
    copyBicubicPatch, translateBicubicPatch, rotateBicubicPatch,
    scaleBicubicPatch, invertBicubicPatch};

extern long rayBicubicTests, rayBicubicTestsSucceeded;
extern Ray *vpRay;
extern int shadowTestFlag;

int maxDepthReached;

static void bezierValue(
    Vector3D *result, DBL u, DBL v, Vector3D (*controlPoints)[4][4]);
static void bezierPartial(Vector3D *result, DBL u, DBL v, BicubicPatch *shape);
static int subpatchNormal(
    Vector3D *v1, Vector3D *v2, Vector3D *v3, Vector3D *result, DBL *d);
static int intersectSubpatch(int patchType, Ray *ray, Vector3D *v1,
    Vector3D *v2, Vector3D *v3, Vector3D *n, DBL d, Vector3D *n1, Vector3D *n2,
    Vector3D *n3, DBL *depth, Vector3D *ip, Vector3D *ipNorm);
static void findAverage(
    int vectorCount, Vector3D *vectors, Vector3D *center, DBL *radius);
static int sphericalBoundsCheck(Ray *ray, Vector3D *center, DBL radius);
static int intersectBicubicPatch0(Ray *ray, BicubicPatch *shape, DBL *depths);
static int intersectBicubicPatch1(Ray *ray, BicubicPatch *shape, DBL *depths);
static int intersectBicubicPatch2(Ray *ray, BicubicPatch *shape, DBL *depths);
static int intersectBicubicPatch3(Ray *ray, BicubicPatch *shape, DBL *depths);
static int intersectBicubicPatch4(Ray *ray, BicubicPatch *shape, DBL *depths);
static DBL pointPlaneDistance(Vector3D *, Vector3D *, DBL *);
static DBL determineSubpatchFlatness(Vector3D (*)[4][4]);
static int flatEnough(BicubicPatch *, Vector3D (*)[4][4]);
static void bezierBoundingSphere(Vector3D (*)[4][4], Vector3D *, DBL *);
static void bezierSubpatchIntersect(Ray *, BicubicPatch *, Vector3D (*)[4][4],
    DBL, DBL, DBL, int, int *, DBL *, DBL *, DBL *);
static void bezierSplitLeftRight(
    Vector3D (*)[4][4], Vector3D (*)[4][4], Vector3D (*)[4][4]);
static void bezierSplitUpDown(
    Vector3D (*)[4][4], Vector3D (*)[4][4], Vector3D (*)[4][4]);
static void bezierSubdivider(Ray *, BicubicPatch *, Vector3D (*)[4][4], DBL,
    DBL, DBL, DBL, int, int *, DBL *, DBL *, DBL *);
static void bezierTreeDeleter(BezierNode *node);
static BezierNode *bezierTreeBuilder(BicubicPatch *, Vector3D (*)[4][4], int);
static void bezierTreeWalker(
    Ray *, BicubicPatch *, BezierNode *, int, int *, DBL *);
static BezierNode *createNewBezierNode();
static BezierVertices *createBezierVertexBlock();
static BezierChild *createBezierChildBlock();

static constexpr double SUBDIVISION_EPSILON = 0.001;
static constexpr int MAX_RECURSION_DEPTH = 20;

/*===========================================================================*/

static BezierNode *
createNewBezierNode()
{
    BezierNode *node = new BezierNode();
    if (node == nullptr) {
        printf("Failed to allocate Bezier node\n");
        exit(0);
    }
    node->Data_Ptr = nullptr;
    return node;
}

static BezierVertices *
createBezierVertexBlock()
{
    BezierVertices *vertices = new BezierVertices();
    if (vertices == nullptr) {
        printf("Failed to allocate Bezier vertices\n");
        exit(0);
    }
    return vertices;
}

static BezierChild *
createBezierChildBlock()
{
    BezierChild *children = new BezierChild();
    if (children == nullptr) {
        printf("Failed to allocate Bezier children\n");
        exit(0);
    }
    return children;
}

static BezierNode *
bezierTreeBuilder(BicubicPatch *object, Vector3D (*patch)[4][4], int depth)
{
    Vector3D lowerLeft[4][4];
    Vector3D lowerRight[4][4];
    Vector3D upperLeft[4][4];
    Vector3D upperRight[4][4];
    BezierChild *children;
    BezierVertices *vertices;
    BezierNode *node = createNewBezierNode();

    if (depth > maxDepthReached) {
        maxDepthReached = depth;
    }

    /* Build the bounding sphere for this subpatch */
    bezierBoundingSphere(patch, &(node->Center), &(node->Radius_Squared));

    /* If the patch is close to being flat, then just perform a ray-plane
        intersection test. */
    if (flatEnough(object, patch)) {
        /* The patch is now flat enough to simply store the corners */
        node->Node_Type = BEZIER_LEAF_NODE;
        vertices = createBezierVertexBlock();
        vertices->Vertices[0] = (*patch)[0][0];
        vertices->Vertices[1] = (*patch)[0][3];
        vertices->Vertices[2] = (*patch)[3][3];
        vertices->Vertices[3] = (*patch)[3][0];
        node->Data_Ptr = (void *)vertices;
    } else if (depth >= object->U_Steps) {
        if (depth >= object->V_Steps) {
            /* We are at the max recursion depth. Just store corners. */
            node->Node_Type = BEZIER_LEAF_NODE;
            vertices = createBezierVertexBlock();
            vertices->Vertices[0] = (*patch)[0][0];
            vertices->Vertices[1] = (*patch)[0][3];
            vertices->Vertices[2] = (*patch)[3][3];
            vertices->Vertices[3] = (*patch)[3][0];
            node->Data_Ptr = (void *)vertices;
        } else {
            bezierSplitUpDown(patch, (Vector3D(*)[4][4])lowerLeft,
                (Vector3D(*)[4][4])upperLeft);
            node->Node_Type = BEZIER_INTERIOR_NODE;
            children = createBezierChildBlock();
            children->Children[0] = bezierTreeBuilder(
                object, (Vector3D(*)[4][4])lowerLeft, depth + 1);
            children->Children[1] = bezierTreeBuilder(
                object, (Vector3D(*)[4][4])upperLeft, depth + 1);
            node->Count = 2;
            node->Data_Ptr = (void *)children;
        }
    } else if (depth >= object->V_Steps) {
        bezierSplitLeftRight(
            patch, (Vector3D(*)[4][4])lowerLeft, (Vector3D(*)[4][4])lowerRight);
        node->Node_Type = BEZIER_INTERIOR_NODE;
        children = createBezierChildBlock();
        children->Children[0] =
            bezierTreeBuilder(object, (Vector3D(*)[4][4])lowerLeft, depth + 1);
        children->Children[1] =
            bezierTreeBuilder(object, (Vector3D(*)[4][4])lowerRight, depth + 1);
        node->Count = 2;
        node->Data_Ptr = (void *)children;
    } else {
        bezierSplitLeftRight(
            patch, (Vector3D(*)[4][4])lowerLeft, (Vector3D(*)[4][4])lowerRight);
        bezierSplitUpDown((Vector3D(*)[4][4])lowerLeft,
            (Vector3D(*)[4][4])lowerLeft, (Vector3D(*)[4][4])upperLeft);
        bezierSplitUpDown((Vector3D(*)[4][4])lowerRight,
            (Vector3D(*)[4][4])lowerRight, (Vector3D(*)[4][4])upperRight);
        node->Node_Type = BEZIER_INTERIOR_NODE;
        children = createBezierChildBlock();
        children->Children[0] =
            bezierTreeBuilder(object, (Vector3D(*)[4][4])lowerLeft, depth + 1);
        children->Children[1] =
            bezierTreeBuilder(object, (Vector3D(*)[4][4])upperLeft, depth + 1);
        children->Children[2] =
            bezierTreeBuilder(object, (Vector3D(*)[4][4])lowerRight, depth + 1);
        children->Children[3] =
            bezierTreeBuilder(object, (Vector3D(*)[4][4])upperRight, depth + 1);
        node->Count = 4;
        node->Data_Ptr = (void *)children;
    }
    return node;
}

/* Evaluate a single coordinate point (u, v) on a bezier patch. */
static void
bezierValue(Vector3D *result, DBL u, DBL v, Vector3D (*controlPoints)[4][4])
{
    DBL u2, u3, v2, v3, uu1, uu2, uu3, vv1, vv2, vv3;
    DBL t[4][4];
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

    result->x = 0.0;
    result->y = 0.0;
    result->z = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result->x += t[i][j] * (*controlPoints)[i][j].x;
            result->y += t[i][j] * (*controlPoints)[i][j].y;
            result->z += t[i][j] * (*controlPoints)[i][j].z;
        }
    }
}

/* Calculate the normal to a bezier patch for a particular axis, at
    a particular point on the patch.

    The normal at a point of a parametric surface z = f(u, v) is:

        (|[[dy/du, dy/dv],[dz/du, dz/dv]]|,
         |[[dz/du, dz/dv],[dx/du, dx/dv]]|,
         |[[dx/du, dx/dv],[dy/du, dy/dv]]|)

    The normal is undefined where the determinants vanish.
*/
static void
bezierPartial(Vector3D *result, DBL u, DBL v, BicubicPatch *shape)
{
    Vector3D uVec;
    Vector3D vVec; /* Partial derivatives with respect to u, and v. */
    DBL u2, u3, v2, v3;
    DBL t[4][4], temp;
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
    uVec.x = 0.0;
    uVec.y = 0.0;
    uVec.z = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            uVec.x += t[i][j] * shape->Control_Points[i][j].x;
            uVec.y += t[i][j] * shape->Control_Points[i][j].y;
            uVec.z += t[i][j] * shape->Control_Points[i][j].z;
        }
    }
    temp = uVec.x * uVec.x + uVec.y * uVec.y + uVec.z * uVec.z;
    if (temp < EPSILON) {
        /* Partial with respect to u is undefined. */
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
        /* *Result = *n; */
        return;
    }
    temp = sqrt(temp);
    VInverseScale(uVec, uVec, temp);

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
    vVec.x = 0.0;
    vVec.y = 0.0;
    vVec.z = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            vVec.x += t[i][j] * shape->Control_Points[i][j].x;
            vVec.y += t[i][j] * shape->Control_Points[i][j].y;
            vVec.z += t[i][j] * shape->Control_Points[i][j].z;
        }
    }
    temp = vVec.x * vVec.x + vVec.y * vVec.y + vVec.z * vVec.z;
    if (temp < EPSILON) {
        /* Partial with respect to u is undefined. */
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
        return;
    }
    temp = sqrt(temp);
    VInverseScale(vVec, vVec, temp);

    VCross(*result, uVec, vVec);
}

/* Calculate the normal to a subpatch (triangle) return the vector
    <1.0 0.0 0.0> if the triangle is degenerate. */
static int
subpatchNormal(
    Vector3D *v1, Vector3D *v2, Vector3D *v3, Vector3D *result, DBL *d)
{
    Vector3D edge1;
    Vector3D edge2;
    DBL length;

    VSub(edge1, *v1, *v2);
    VSub(edge2, *v3, *v2);
    VCross(*result, edge1, edge2);
    VLength(length, *result);
    if (length < EPSILON) {
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
        *d = -1.0 * v1->x;
        return 0;
    }
    VInverseScale(*result, *result, length);
    VDot(*d, *result, *v1);
    *d = 0.0 - *d;
    return 1;
}

/* See if Ray intersects the triangular subpatch defined by the three
    points v1, v2, v3 and the normal to the triangle n. If so, Depth will
    be set to the distance along Ray at which the intersection occurs. */
static int
intersectSubpatch(int patchType, Ray *ray, Vector3D *v1, Vector3D *v2,
    Vector3D *v3, Vector3D *n, DBL d, Vector3D *n1, Vector3D *n2, Vector3D *n3,
    DBL *depth, Vector3D *ip, Vector3D *ipNorm)
{
    Vector3D tempV1;
    Vector3D tempV2;
    Vector3D perp;
    DBL s, t, proj, mu;
    DBL x1, y1, x2, y2, x3, y3;
    DBL x, y, z;
    int signHolder;
    int nextSign;
    int crossings;

    /* Calculate the point of intersection and the depth. */
    VDot(s, ray->Direction, *n);
    if (s == 0.0) {
        return 0;
    }
    VDot(t, ray->Initial, *n);
    *depth = 0.0 - (d + t) / s;
    if (*depth < Small_Tolerance) {
        return 0;
    }
    VScale(*ip, ray->Direction, *depth);
    VAdd(*ip, *ip, ray->Initial);

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
        VSub(tempV1, *v2, *v3);
        VNormalize(tempV1, tempV1);
        VSub(tempV2, *v1, *v3);
        VDot(proj, tempV2, tempV1);
        VScale(tempV1, tempV1, proj);
        VSub(perp, tempV1, tempV2);
        VNormalize(perp, perp);
        VDot(mu, tempV2, perp);
        mu = -1.0 / mu;
        VScale(perp, perp, mu);
        VSub(tempV1, *ip, *v1);
        VDot(s, tempV1, perp);
        if (s < EPSILON) {
            *ipNorm = *n1;
            return 1;
        }
        VSub(tempV1, *v3, *v2);
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
        VSub(tempV1, *n2, *n1);
        VScale(tempV1, tempV1, s);
        VAdd(tempV1, tempV1, *n1);
        VSub(tempV2, *n3, *n1);
        VScale(tempV2, tempV2, s);
        VAdd(tempV2, tempV2, *n1);
        VSub(*ipNorm, tempV2, tempV1);
        VScale(*ipNorm, *ipNorm, t);
        VAdd(*ipNorm, *ipNorm, tempV1);
        VNormalize(*ipNorm, *ipNorm);
        return 1;
    }
    return 0;
}

/* Find a sphere that contains all of the points in the list "vectors" */
static void
findAverage(int vectorCount, Vector3D *vectors, Vector3D *center, DBL *radius)
{
    DBL r0, r1, xc = 0, yc = 0, zc = 0;
    DBL x0, y0, z0;
    int i;
    for (i = 0; i < vectorCount; i++) {
        xc += vectors[i].x;
        yc += vectors[i].y;
        zc += vectors[i].z;
    }
    xc /= (DBL)vectorCount;
    yc /= (DBL)vectorCount;
    zc /= (DBL)vectorCount;
    r0 = 0.0;
    for (i = 0; i < vectorCount; i++) {
        x0 = vectors[i].x - xc;
        y0 = vectors[i].y - yc;
        z0 = vectors[i].z - zc;
        r1 = x0 * x0 + y0 * y0 + z0 * z0;
        if (r1 > r0) {
            r0 = r1;
        }
    }
    center->x = xc;
    center->y = yc;
    center->z = zc;
    *radius = r0;
}

static int
sphericalBoundsCheck(Ray *ray, Vector3D *center, DBL radius)
{
    DBL x, y, z, dist1, dist2;
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

/* Find a sphere that bounds all of the control points of a Bezier patch.
    The values returned are: the center of the bounding sphere, and the
    square of the radius of the bounding sphere. */
static void
bezierBoundingSphere(Vector3D (*patch)[4][4], Vector3D *center, DBL *radius)
{
    DBL r0, r1, xc = 0, yc = 0, zc = 0;
    DBL x0, y0, z0;
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            xc += (*patch)[i][j].x;
            yc += (*patch)[i][j].y;
            zc += (*patch)[i][j].z;
        }
    }
    xc /= 16.0;
    yc /= 16.0;
    zc /= 16.0;
    r0 = 0.0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            x0 = (*patch)[i][j].x - xc;
            y0 = (*patch)[i][j].y - yc;
            z0 = (*patch)[i][j].z - zc;
            r1 = x0 * x0 + y0 * y0 + z0 * z0;
            if (r1 > r0) {
                r0 = r1;
            }
        }
    }
    center->x = xc;
    center->y = yc;
    center->z = zc;
    *radius = r0;
}

/* Precompute grid points and normals for a bezier patch */
void
precomputePatchValues(BicubicPatch *shape)
{
    int i;
    int j;
    DBL d, u, v, deltaU, deltaV;
    Vector3D v0;
    Vector3D v1;
    Vector3D v2;
    Vector3D v3;
    Vector3D n;
    Vector3D controlPoints[16];
    Vector3D(*patchPtr)[4][4] = (Vector3D(*)[4][4])shape->Control_Points;

    /* Calculate the bounding sphere for the entire patch. */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            controlPoints[4 * i + j] = shape->Control_Points[i][j];
        }
    }
    findAverage(16, &controlPoints[0], &shape->Bounding_Sphere_Center,
        &shape->Bounding_Sphere_Radius);
    /* Shape->Node_Tree = NULL; */
    if (shape->Patch_Type == 0 || shape->Patch_Type == 2) {
        return;
    }
    if (shape->Patch_Type == 3) {
        if (shape->Node_Tree != nullptr) {
            bezierTreeDeleter(shape->Node_Tree);
        }
        shape->Node_Tree = bezierTreeBuilder(shape, patchPtr, 0);
        return;
    }
    deltaU = 1.0 / (DBL)shape->U_Steps;
    deltaV = 1.0 / (DBL)shape->V_Steps;
    if (shape->Interpolated_Grid == nullptr) {
        shape->Interpolated_Grid = new Vector3D *[shape->U_Steps + 1];
        if (shape->Interpolated_Grid == nullptr) {
            Error("Failed to allocate Interpolated_Grid");
        }
        for (i = 0; i <= shape->U_Steps; i++) {
            shape->Interpolated_Grid[i] = new Vector3D[shape->V_Steps + 1];
            if (shape->Interpolated_Grid == nullptr) {
                Error("Failed to allocate component of Interpolated_Grid");
            }
        }
        shape->Interpolated_Normals = new Vector3D *[shape->U_Steps + 1];
        if (shape->Interpolated_Normals == nullptr) {
            Error("Failed to allocate Interpolated_Normals");
        }
        for (i = 0; i <= shape->U_Steps; i++) {
            shape->Interpolated_Normals[i] =
                new Vector3D[2 * (shape->V_Steps + 1)];
            if (shape->Interpolated_Normals == nullptr) {
                Error("Failed to allocate component of Interpolated_Normals");
            }
        }

        if (shape->Patch_Type == 4) {
            shape->Smooth_Normals = new Vector3D *[shape->U_Steps + 1];
            if (shape->Smooth_Normals == nullptr) {
                Error("Failed to allocate Smooth_Normals");
            }
            for (i = 0; i <= shape->U_Steps; i++) {
                shape->Smooth_Normals[i] = new Vector3D[shape->V_Steps + 1];
                if (shape->Smooth_Normals == nullptr) {
                    Error("Failed to allocate component of Smooth_Normals");
                }
            }
        }

        shape->Interpolated_D = new DBL *[shape->U_Steps + 1];
        if (shape->Interpolated_D == nullptr) {
            Error("Failed to allocate Interpolated_D");
        }
        for (i = 0; i <= shape->U_Steps; i++) {
            shape->Interpolated_D[i] = new DBL[2 * (shape->V_Steps + 1)];
            if (shape->Interpolated_D == nullptr) {
                Error("Failed to allocate component of Interpolated_D");
            }
        }
    }

    /* Calculate the grid values for the given subdivision values. */
    for (i = 0; i <= shape->U_Steps; i++) {
        u = (DBL)i / (DBL)shape->U_Steps;
        for (j = 0; j < shape->V_Steps; j++) {
            v = (DBL)j / (DBL)shape->V_Steps;
            bezierValue(&shape->Interpolated_Grid[i][j], u, v, patchPtr);
        }
    }

    for (i = 0; i < shape->U_Steps; i++) {
        u = (DBL)i / (DBL)shape->U_Steps;
        for (j = 0; j < shape->V_Steps; j++) {
            v = (DBL)j / (DBL)shape->V_Steps;

            /* Calculate surface values for the current patch. */
            bezierValue(&v0, u, v, patchPtr);
            bezierValue(&v1, u + deltaU, v, patchPtr);
            bezierValue(&v2, u, v + deltaV, patchPtr);
            bezierValue(&v3, u + deltaU, v + deltaV, patchPtr);

            shape->Interpolated_Grid[i][j] = v0;
            shape->Interpolated_Grid[i + 1][j] = v1;
            shape->Interpolated_Grid[i][j + 1] = v2;
            shape->Interpolated_Grid[i + 1][j + 1] = v3;
            if (shape->Patch_Type == 1 || shape->Patch_Type == 4) {
                /* Calculate the normals */
                if (subpatchNormal(&v0, &v2, &v1, &n, &d)) {
                    shape->Interpolated_Normals[i][2 * j] = n;
                    shape->Interpolated_D[i][2 * j] = d;
                } else {
                    shape->Interpolated_Normals[i][2 * j].x = 0.0;
                    shape->Interpolated_Normals[i][2 * j].y = 0.0;
                    shape->Interpolated_Normals[i][2 * j].z = 0.0;
                    shape->Interpolated_D[i][2 * j] = 0.0;
                }

                if (subpatchNormal(&v1, &v2, &v3, &n, &d)) {
                    shape->Interpolated_Normals[i][2 * j + 1] = n;
                    shape->Interpolated_D[i][2 * j + 1] = d;
                } else {
                    shape->Interpolated_Normals[i][2 * j + 1].x = 0.0;
                    shape->Interpolated_Normals[i][2 * j + 1].y = 0.0;
                    shape->Interpolated_Normals[i][2 * j + 1].z = 0.0;
                    shape->Interpolated_D[i][2 * j + 1] = 0.0;
                }
            }
        }
    }

    if (shape->Patch_Type == 4) {
        /* Calculate normals at the corners of the subpatches */
        for (i = 0; i <= shape->U_Steps; i++) {
            u = (DBL)i / (DBL)shape->U_Steps;
            for (j = 0; j <= shape->V_Steps; j++) {
                v = (DBL)j / (DBL)shape->V_Steps;
                bezierPartial(&shape->Smooth_Normals[i][j], u, v, shape);
            }
        }
    }
}

/* Determine the distance from a point to a plane. */
static DBL
pointPlaneDistance(Vector3D *p, Vector3D *n, DBL *d)
{
    DBL temp1, temp2;

    VDot(temp1, *p, *n);
    temp1 += *d;
    VLength(temp2, *n);
    if (fabs(temp2) < EPSILON) {
        return 0;
    }
    temp1 /= temp2;
    return temp1;
}

static void
bezierSubpatchIntersect(Ray *ray, BicubicPatch *shape, Vector3D (*patch)[4][4],
    DBL u0, DBL u1, DBL v0, int recursionDepth, int *depthCount, DBL *depths,
    DBL *uValues, DBL *vValues)
{
    int tcnt = shape->Intersection_Count;
    Vector3D vv0;
    Vector3D vv1;
    Vector3D vv2;
    Vector3D vv3;
    Vector3D n;
    Vector3D ip;
    DBL d;
    DBL depth;

    if (tcnt + *depthCount >= MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    vv0 = (*patch)[0][0];
    vv1 = (*patch)[0][3];
    vv2 = (*patch)[3][3];
    vv3 = (*patch)[3][0];

    /* Triangulate this subpatch, then check for intersections in
        the triangles. */
    if (subpatchNormal(&vv0, &vv1, &vv2, &n, &d)) {
        if (intersectSubpatch(shape->Patch_Type, ray, &vv0, &vv1, &vv2, &n, d,
                nullptr, nullptr, nullptr, &depth, &ip, &n)) {
            shape->Intersection_Point[tcnt + *depthCount] = ip;
            shape->Normal_Vector[tcnt + *depthCount] = n;
            depths[*depthCount] = depth;
            *depthCount += 1;
        }
    }

    if (*depthCount + tcnt >= MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    if (subpatchNormal(&vv0, &vv2, &vv3, &n, &d)) {
        if (intersectSubpatch(shape->Patch_Type, ray, &vv0, &vv2, &vv3, &n, d,
                nullptr, nullptr, nullptr, &depth, &ip, &n)) {
            shape->Intersection_Point[tcnt + *depthCount] = ip;
            shape->Normal_Vector[tcnt + *depthCount] = n;
            depths[*depthCount] = depth;
            *depthCount += 1;
        }
    }
}

static void
bezierSplitLeftRight(Vector3D (*patch)[4][4], Vector3D (*leftPatch)[4][4],
    Vector3D (*rightPatch)[4][4])
{
    Vector3D temp1[4];
    Vector3D temp2[4];
    Vector3D half;
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        temp1[0] = (*patch)[i][0];
        VHalf(temp1[1], (*patch)[i][0], (*patch)[i][1]);
        VHalf(half, (*patch)[i][1], (*patch)[i][2]);
        VHalf(temp1[2], temp1[1], half);
        VHalf(temp2[2], (*patch)[i][2], (*patch)[i][3]);
        VHalf(temp2[1], half, temp2[2]);
        VHalf(temp1[3], temp1[2], temp2[1]);
        temp2[0] = temp1[3];
        temp2[3] = (*patch)[i][3];
        for (j = 0; j < 4; j++) {
            (*leftPatch)[i][j] = temp1[j];
            (*rightPatch)[i][j] = temp2[j];
        }
    }
}

static void
bezierSplitUpDown(Vector3D (*patch)[4][4], Vector3D (*topPatch)[4][4],
    Vector3D (*bottomPatch)[4][4])
{
    Vector3D temp1[4];
    Vector3D temp2[4];
    Vector3D half;
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        /* Split Left */
        temp1[0] = (*patch)[0][i];
        VHalf(temp1[1], (*patch)[0][i], (*patch)[1][i]);
        VHalf(half, (*patch)[1][i], (*patch)[2][i]);
        VHalf(temp1[2], temp1[1], half);
        VHalf(temp2[2], (*patch)[2][i], (*patch)[3][i]);
        VHalf(temp2[1], half, temp2[2]);
        VHalf(temp1[3], temp1[2], temp2[1]);
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
static DBL
determineSubpatchFlatness(Vector3D (*patch)[4][4])
{
    Vector3D vertices[4];
    Vector3D n;
    Vector3D tempV;
    DBL d, dist, temp1;
    int i;
    int j;

    vertices[0] = (*patch)[0][0];
    vertices[1] = (*patch)[0][3];
    VSub(tempV, vertices[0], vertices[1]);
    VLength(temp1, tempV);
    if (fabs(temp1) < EPSILON) {
        /* Degenerate in the V direction for U = 0. This is ok if the other
            two corners are distinct from the lower left corner - I'm sure there
            are cases where the corners coincide and the middle has good values,
            but that is somewhat pathalogical and won't be considered. */
        vertices[1] = (*patch)[3][3];
        VSub(tempV, vertices[0], vertices[1]);
        VLength(temp1, tempV);
        if (fabs(temp1) < EPSILON) {
            return -1.0;
        }
        vertices[2] = (*patch)[3][0];
        VSub(tempV, vertices[0], vertices[1]);
        VLength(temp1, tempV);
        if (fabs(temp1) < EPSILON) {
            return -1.0;
        }
        VSub(tempV, vertices[1], vertices[2]);
        VLength(temp1, tempV);
        if (fabs(temp1) < EPSILON) {
            return -1.0;
        }
    } else {
        vertices[2] = (*patch)[3][0];
        VSub(tempV, vertices[0], vertices[1]);
        VLength(temp1, tempV);
        if (fabs(temp1) < EPSILON) {
            vertices[2] = (*patch)[3][3];
            VSub(tempV, vertices[0], vertices[2]);
            VLength(temp1, tempV);
            if (fabs(temp1) < EPSILON) {
                return -1.0;
            }
            VSub(tempV, vertices[1], vertices[2]);
            VLength(temp1, tempV);
            if (fabs(temp1) < EPSILON) {
                return -1.0;
            }
        } else {
            VSub(tempV, vertices[1], vertices[2]);
            VLength(temp1, tempV);
            if (fabs(temp1) < EPSILON) {
                return -1.0;
            }
        }
    }
    /* Now that a good set of candidate points has been found, find the
        plane equations for the patch */
    if (subpatchNormal(&vertices[0], &vertices[1], &vertices[2], &n, &d)) {
        /* Step through all vertices and see what the maximum distance from the
                 plane happens to be. */
        dist = 0.0;
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                temp1 = fabs(pointPlaneDistance(&((*patch)[i][j]), &n, &d));
                if (temp1 > dist) {
                    dist = temp1;
                }
            }
        }
        return dist;
    }
    /* printf("Subpatch normal failed in determine_subpatch_flatness\n"); */
    return -1.0;
}

static int
flatEnough(BicubicPatch *object, Vector3D (*patch)[4][4])
{
    DBL dist;

    dist = determineSubpatchFlatness(patch);
    if (dist < 0.0) {
        return 0;
    }
    if (dist < object->Flatness_Value) {
        return 1;
    }
    return 0;
}

static void
bezierSubdivider(Ray *ray, BicubicPatch *object, Vector3D (*patch)[4][4],
    DBL u0, DBL u1, DBL v0, DBL v1, int recursionDepth, int *depthCount,
    DBL *depths, DBL *uValues, DBL *vValues)
{
    Vector3D lowerLeft[4][4];
    Vector3D lowerRight[4][4];
    Vector3D upperLeft[4][4];
    Vector3D upperRight[4][4];
    Vector3D center;
    DBL ut, vt, radius;
    int tcnt = object->Intersection_Count;

    /* Don't waste time if there are already too many intersections */
    if (tcnt >= MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    /* Make sure the ray passes through a sphere bounding the control points of
        the patch */
    bezierBoundingSphere(patch, &center, &radius);
    if (!sphericalBoundsCheck(ray, &center, radius)) {
        return;
    }

    /* If the patch is close to being flat, then just perform a ray-plane
        intersection test. */
    if (flatEnough(object, patch)) {
        bezierSubpatchIntersect(ray, object, patch, u0, u1, v0,
            recursionDepth + 1, depthCount, depths, uValues, vValues);
    }

    if (recursionDepth >= object->U_Steps) {
        if (recursionDepth >= object->V_Steps) {
            bezierSubpatchIntersect(ray, object, patch, u0, u1, v0,
                recursionDepth + 1, depthCount, depths, uValues, vValues);
        } else {
            bezierSplitUpDown(patch, (Vector3D(*)[4][4])lowerLeft,
                (Vector3D(*)[4][4])upperLeft);
            vt = (v1 - v0) / 2.0;
            bezierSubdivider(ray, object, (Vector3D(*)[4][4])lowerLeft, u0, u1,
                v0, vt, recursionDepth + 1, depthCount, depths, uValues,
                vValues);
            bezierSubdivider(ray, object, (Vector3D(*)[4][4])upperLeft, u0, u1,
                vt, v1, recursionDepth + 1, depthCount, depths, uValues,
                vValues);
        }
    } else if (recursionDepth >= object->V_Steps) {
        bezierSplitLeftRight(
            patch, (Vector3D(*)[4][4])lowerLeft, (Vector3D(*)[4][4])lowerRight);
        ut = (u1 - u0) / 2.0;
        bezierSubdivider(ray, object, (Vector3D(*)[4][4])lowerLeft, u0, ut, v0,
            v1, recursionDepth + 1, depthCount, depths, uValues, vValues);
        bezierSubdivider(ray, object, (Vector3D(*)[4][4])lowerRight, ut, u1, v0,
            v1, recursionDepth + 1, depthCount, depths, uValues, vValues);
    } else {
        ut = (u1 - u0) / 2.0;
        vt = (v1 - v0) / 2.0;
        bezierSplitLeftRight(
            patch, (Vector3D(*)[4][4])lowerLeft, (Vector3D(*)[4][4])lowerRight);
        bezierSplitUpDown((Vector3D(*)[4][4])lowerLeft,
            (Vector3D(*)[4][4])lowerLeft, (Vector3D(*)[4][4])upperLeft);
        bezierSplitUpDown((Vector3D(*)[4][4])lowerRight,
            (Vector3D(*)[4][4])lowerRight, (Vector3D(*)[4][4])upperRight);
        bezierSubdivider(ray, object, (Vector3D(*)[4][4])lowerLeft, u0, ut, v0,
            vt, recursionDepth + 1, depthCount, depths, uValues, vValues);
        bezierSubdivider(ray, object, (Vector3D(*)[4][4])upperLeft, u0, ut, vt,
            v1, recursionDepth + 1, depthCount, depths, uValues, vValues);
        bezierSubdivider(ray, object, (Vector3D(*)[4][4])lowerRight, ut, u1, v0,
            vt, recursionDepth + 1, depthCount, depths, uValues, vValues);
        bezierSubdivider(ray, object, (Vector3D(*)[4][4])upperRight, ut, u1, vt,
            v1, recursionDepth + 1, depthCount, depths, uValues, vValues);
    }
}

static void
bezierTreeDeleter(BezierNode *node)
{
    BezierChild *children;
    int i;

    /* If this is an interior node then continue the descent */
    if (node->Node_Type == BEZIER_INTERIOR_NODE) {
        children = (BezierChild *)node->Data_Ptr;
        for (i = 0; i < node->Count; i++) {
            bezierTreeDeleter(children->Children[i]);
        }
        delete (BezierChild *)children;
    } else if (node->Node_Type == BEZIER_LEAF_NODE) {
        /* Free the memory used for the vertices. */
        delete (BezierVertices *)node->Data_Ptr;
    }
    /* Free the memory used for the node. */
    delete node;
}

static void
bezierTreeWalker(Ray *ray, BicubicPatch *shape, BezierNode *node, int depth,
    int *depthCount, DBL *depths)
{
    BezierChild *children;
    BezierVertices *vertices;
    Vector3D n;
    Vector3D ip;
    Vector3D vv0;
    Vector3D vv1;
    Vector3D vv2;
    Vector3D vv3;
    DBL d;
    DBL hitDepth;
    int i;
    int tcnt = shape->Intersection_Count;

    /* Don't waste time if there are already too many intersections */
    if (tcnt >= MAX_BICUBIC_INTERSECTIONS) {
        return;
    }

    /* Make sure the ray passes through a sphere bounding the control points of
        the patch */
    if (!sphericalBoundsCheck(ray, &(node->Center), node->Radius_Squared)) {
        return;
    }

    /* If this is an interior node then continue the descent, else
        do a check against the vertices. */
    if (node->Node_Type == BEZIER_INTERIOR_NODE) {
        children = (BezierChild *)node->Data_Ptr;
        for (i = 0; i < node->Count; i++) {
            bezierTreeWalker(ray, shape, children->Children[i], depth + 1,
                depthCount, depths);
        }
    } else if (node->Node_Type == BEZIER_LEAF_NODE) {
        vertices = (BezierVertices *)node->Data_Ptr;
        vv0 = vertices->Vertices[0];
        vv1 = vertices->Vertices[1];
        vv2 = vertices->Vertices[2];
        vv3 = vertices->Vertices[3];

        /* Triangulate this subpatch, then check for intersections in
            the triangles. */
        if (subpatchNormal(&vv0, &vv1, &vv2, &n, &d)) {
            if (intersectSubpatch(shape->Patch_Type, ray, &vv0, &vv1, &vv2, &n,
                    d, nullptr, nullptr, nullptr, &hitDepth, &ip, &n)) {
                shape->Intersection_Point[tcnt + *depthCount] = ip;
                shape->Normal_Vector[tcnt + *depthCount] = n;
                depths[*depthCount] = hitDepth;
                *depthCount += 1;
            }
        }

        if (*depthCount + tcnt >= MAX_BICUBIC_INTERSECTIONS) {
            return;
        }

        if (subpatchNormal(&vv0, &vv2, &vv3, &n, &d)) {
            if (intersectSubpatch(shape->Patch_Type, ray, &vv0, &vv2, &vv3, &n,
                    d, nullptr, nullptr, nullptr, &hitDepth, &ip, &n)) {
                shape->Intersection_Point[tcnt + *depthCount] = ip;
                shape->Normal_Vector[tcnt + *depthCount] = n;
                depths[*depthCount] = hitDepth;
                *depthCount += 1;
            }
        }
    } else {
        printf("Bad Node type at depth %d\n", depth);
    }
}

/* Intersection of a ray and a bezier patch. */
/* Note: There is MUCH repeated work being done here. During the subdivision
    process, the values of surface points are not retained from one subpatch
    to the next, even though there are two points in common between one subpatch
    and the one adjacent to it.  An obvious optimization is to retain this
    surface information during the subdivision process.

    A second optimization is to make use of the fact that the surface never
    goes outside the convex hull generated by the control points.  By testing
    the ray against that hull first many unnecessary tests can be avoided.  The
    hull really should be generated at the time the patch is first defined -
    not at run time.
*/
static int
intersectBicubicPatch0(Ray *ray, BicubicPatch *shape, DBL *depths)
{
    int cnt = 0;
    int tcnt = shape->Intersection_Count;
    int i;
    int j;
    DBL depth, d, u, v, deltaU, deltaV;
    Vector3D v0;
    Vector3D v1;
    Vector3D v2;
    Vector3D v3;
    Vector3D n;
    Vector3D ip;
    Vector3D(*patchPtr)[4][4] = (Vector3D(*)[4][4])shape->Control_Points;

    if (!sphericalBoundsCheck(ray, &(shape->Bounding_Sphere_Center),
            shape->Bounding_Sphere_Radius)) {
        return 0;
    }

    deltaU = 1.0 / (DBL)shape->U_Steps;
    deltaV = 1.0 / (DBL)shape->V_Steps;

    /* Calculate the initial point */
    for (i = 0; i < shape->U_Steps; i++) {
        u = (DBL)i / (DBL)shape->U_Steps;
        for (j = 0; j < shape->V_Steps; j++) {
            v = (DBL)j / (DBL)shape->V_Steps;

            /* Calculate surface values for the current patch. */
            bezierValue(&v0, u, v, patchPtr);
            bezierValue(&v1, u + deltaU, v, patchPtr);
            bezierValue(&v2, u, v + deltaV, patchPtr);
            bezierValue(&v3, u + deltaU, v + deltaV, patchPtr);

            /* Triangulate this subpatch, then check for intersections in
                the triangles. */
            if (subpatchNormal(&v0, &v2, &v1, &n, &d)) {
                if (intersectSubpatch(shape->Patch_Type, ray, &v0, &v2, &v1, &n,
                        d, nullptr, nullptr, nullptr, &depth, &ip, &n)) {
                    shape->Intersection_Point[tcnt + cnt] = ip;
                    shape->Normal_Vector[tcnt + cnt] = n;
                    depths[cnt] = depth;
                    if (tcnt + ++cnt >= MAX_BICUBIC_INTERSECTIONS) {
                        /* Too many intersections. Stop looking for more. */
                        return cnt;
                    }
                }
            }
            if (subpatchNormal(&v1, &v2, &v3, &n, &d)) {
                if (intersectSubpatch(shape->Patch_Type, ray, &v1, &v2, &v3, &n,
                        d, nullptr, nullptr, nullptr, &depth, &ip, &n)) {
                    shape->Intersection_Point[tcnt + cnt] = ip;
                    shape->Normal_Vector[tcnt + cnt] = n;
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

static int
intersectBicubicPatch1(Ray *ray, BicubicPatch *shape, DBL *depths)
{
    int cnt = 0;
    int tcnt = shape->Intersection_Count;
    int i;
    int j;
    DBL depth, d, radius;
    Vector3D v[4];
    Vector3D n;
    Vector3D ip;
    Vector3D center;

    if (!sphericalBoundsCheck(ray, &(shape->Bounding_Sphere_Center),
            shape->Bounding_Sphere_Radius)) {
        return 0;
    }

    /* Calculate the initial point */
    for (i = 0; i < shape->U_Steps; i++) {
        for (j = 0; j < shape->V_Steps; j++) {

            /* Grab precomputed surface values for the current patch. */
            v[0] = shape->Interpolated_Grid[i][j];
            v[1] = shape->Interpolated_Grid[i + 1][j];
            v[2] = shape->Interpolated_Grid[i][j + 1];
            v[3] = shape->Interpolated_Grid[i + 1][j + 1];

            /* Check the ray against the bounding sphere for this subpatch */
            findAverage(4, &v[0], &center, &radius);
            if (!sphericalBoundsCheck(ray, &center, radius)) {
                continue;
            }

            n = shape->Interpolated_Normals[i][2 * j];
            if (n.x == 0.0 && n.y == 0.0 && n.z == 0.0) {
                goto l0;
            }
            d = shape->Interpolated_D[i][2 * j];

            /* Check for intersections in this subpatch. */
            if (intersectSubpatch(shape->Patch_Type, ray, &v[0], &v[2], &v[1],
                    &n, d, nullptr, nullptr, nullptr, &depth, &ip, &n)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->Normal_Vector[tcnt + cnt] = n;
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
            if (intersectSubpatch(shape->Patch_Type, ray, &v[1], &v[2], &v[3],
                    &n, d, nullptr, nullptr, nullptr, &depth, &ip, &n)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->Normal_Vector[tcnt + cnt] = n;
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

static int
intersectBicubicPatch2(Ray *ray, BicubicPatch *shape, DBL *depths)
{
    int cnt = 0;
    DBL uValues[MAX_BICUBIC_INTERSECTIONS];
    DBL vValues[MAX_BICUBIC_INTERSECTIONS];
    Vector3D(*patch)[4][4] = (Vector3D(*)[4][4])shape->Control_Points;

    bezierSubdivider(ray, shape, patch, 0.0, 1.0, 0.0, 1.0, 0, &cnt, depths,
        &uValues[0], &vValues[0]);
    return cnt;
}

static int
intersectBicubicPatch3(Ray *ray, BicubicPatch *shape, DBL *depths)
{
    int cnt = 0;
    bezierTreeWalker(ray, shape, shape->Node_Tree, 0, &cnt, depths);
    return cnt;
}

static int
intersectBicubicPatch4(Ray *ray, BicubicPatch *shape, DBL *depths)
{
    int cnt = 0;
    int tcnt = shape->Intersection_Count;
    int i;
    int j;
    DBL depth, d, t;
    Vector3D v0;
    Vector3D v1;
    Vector3D v2;
    Vector3D v3;
    Vector3D n;
    Vector3D n0;
    Vector3D n1;
    Vector3D n2;
    Vector3D n3;
    Vector3D ip;
    Vector3D ipNorm;

    if (!sphericalBoundsCheck(ray, &(shape->Bounding_Sphere_Center),
            shape->Bounding_Sphere_Radius)) {
        return 0;
    }

    /* Calculate the initial point */
    for (i = 0; i < shape->U_Steps; i++) {
        for (j = 0; j < shape->V_Steps; j++) {

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
            VDot(t, n0, n);
            if (t < 0)
                VScale(n0, n0, -1.0);
            VDot(t, n1, n);
            if (t < 0)
                VScale(n1, n1, -1.0);
            VDot(t, n2, n);
            if (t < 0)
                VScale(n2, n2, -1.0);

            /* Check for intersections in this subpatch. */
            if (intersectSubpatch(shape->Patch_Type, ray, &v0, &v2, &v1, &n, d,
                    &n0, &n2, &n1, &depth, &ip, &ipNorm)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->Normal_Vector[tcnt + cnt] = ipNorm;
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
            VDot(t, n1, n);
            if (t > 0)
                VScale(n1, n0, -1.0);
            VDot(t, n2, n);
            if (t > 0)
                VScale(n2, n1, -1.0);
            VDot(t, n3, n);
            if (t > 0)
                VScale(n3, n2, -1.0);

            if (intersectSubpatch(shape->Patch_Type, ray, &v1, &v2, &v3, &n, d,
                    &n1, &n2, &n3, &depth, &ip, &ipNorm)) {
                shape->Intersection_Point[tcnt + cnt] = ip;
                shape->Normal_Vector[tcnt + cnt] = ipNorm;
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
allBicubicPatchIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    BicubicPatch *shape = (BicubicPatch *)object;
    DBL depths[MAX_BICUBIC_INTERSECTIONS];
    Intersection localElement;
    int cnt = 0;
    int tcnt;
    int i;
    int intersectionFound;

    intersectionFound = 0;
    rayBicubicTests++;
    if (ray == vpRay) {
        shape->Intersection_Count = 0;
    }
    tcnt = shape->Intersection_Count;
    if (shape->Patch_Type == 0) {
        cnt = intersectBicubicPatch0(ray, shape, &depths[0]);
    } else if (shape->Patch_Type == 1) {
        cnt = intersectBicubicPatch1(ray, shape, &depths[0]);
    } else if (shape->Patch_Type == 2) {
        cnt = intersectBicubicPatch2(ray, shape, &depths[0]);
    } else if (shape->Patch_Type == 3) {
        cnt = intersectBicubicPatch3(ray, shape, &depths[0]);
    } else if (shape->Patch_Type == 4) {
        cnt = intersectBicubicPatch4(ray, shape, &depths[0]);
    } else {
        Error("Bad patch type\n");
    }
    if (cnt > 0) {
        rayBicubicTestsSucceeded++;
    }
    for (i = 0; i < cnt; i++) {
        if (!shadowTestFlag) {
            shape->Intersection_Count++;
        }
        localElement.Depth = depths[i];
        localElement.Object = shape->Parent_Object;
        localElement.Point = shape->Intersection_Point[tcnt + i];
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = 1;
    }
    return (intersectionFound);
}

/* A patch is not a solid, so an inside test doesn't make sense. */
int
insideBicubicPatch(Vector3D *testPoint, SimpleBody *object)
{
    return 0;
}

void
bicubicPatchNormal(
    Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    BicubicPatch *patch = (BicubicPatch *)object;
    int i;

    /* If all is going well, the normal was computed at the time the
       intersection was computed.  Look on the list of associated intersection
       points and normals */
    for (i = 0; i < patch->Intersection_Count; i++) {
        if (intersectionPoint->x == patch->Intersection_Point[i].x &&
            intersectionPoint->y == patch->Intersection_Point[i].y &&
            intersectionPoint->z == patch->Intersection_Point[i].z) {
            result->x = patch->Normal_Vector[i].x;
            result->y = patch->Normal_Vector[i].y;
            result->z = patch->Normal_Vector[i].z;
            return;
        }
    }
    if (Options & DEBUGGING) {
        printf("Bicubic patch normal for unknown intersection point\n");
        fflush(stdout);
    }
    result->x = 1.0;
    result->y = 0.0;
    result->z = 0.0;
}

void *
copyBicubicPatch(SimpleBody *object)
{
    BicubicPatch *newShape;

    newShape = getBicubicPatchShape();
    *newShape = *((BicubicPatch *)object);
    newShape->Next_Object = nullptr;

    newShape->Interpolated_Grid = nullptr;
    precomputePatchValues(newShape);
    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (void *)(newShape);
}

void
translateBicubicPatch(SimpleBody *object, Vector3D *vector)
{
    BicubicPatch *patch = (BicubicPatch *)object;
    int i;
    int j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            VAdd(patch->Control_Points[i][j], patch->Control_Points[i][j],
                *vector) precomputePatchValues(patch);
    translateTexture(&((BicubicPatch *)object)->Shape_Texture, vector);
}

void
rotateBicubicPatch(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    BicubicPatch *patch = (BicubicPatch *)object;
    int i;
    int j;

    getRotationTransformation(&transformation, vector);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            MTransformVector(&(patch->Control_Points[i][j]),
                &(patch->Control_Points[i][j]), &transformation);
        }
    }
    precomputePatchValues(patch);
    rotateTexture(&((BicubicPatch *)object)->Shape_Texture, vector);
}

void
scaleBicubicPatch(SimpleBody *object, Vector3D *vector)
{
    BicubicPatch *patch = (BicubicPatch *)object;
    int i;
    int j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            VEvaluate(patch->Control_Points[i][j], patch->Control_Points[i][j],
                *vector);
    precomputePatchValues(patch);
    scaleTexture(&((BicubicPatch *)object)->Shape_Texture, vector);
}

/* Inversion of a patch really doesn't make sense. */
void
invertBicubicPatch(SimpleBody *object)
{
    ;
}
