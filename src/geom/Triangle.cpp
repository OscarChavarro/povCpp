/****************************************************************************
 *                     triangle.c
 *
 *  This module implements primitives for triangles and smooth triangles.
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

#include "geom/Triangle.h"
#include "geom/Objects.h"

/*===========================================================================*/

Methods Triangle_Methods = {objectIntersect, allTriangleIntersections,
    insideTriangle, triangleNormal, copyTriangle, translateTriangle,
    rotateTriangle, scaleTriangle, invertTriangle};

Methods Smooth_Triangle_Methods = {objectIntersect, allTriangleIntersections,
    insideTriangle, smoothTriangleNormal, copySmoothTriangle,
    translateSmoothTriangle, rotateSmoothTriangle, scaleSmoothTriangle,
    invertSmoothTriangle};

extern Triangle *getTriangleShape();

extern Ray *vpRay;
extern long rayTriangleTests, rayTriangleTestsSucceeded;

#define max3(x, y, z) ((x > y) ? ((x > z) ? 1 : 3) : ((y > z) ? 2 : 3))
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

/*===========================================================================*/

static void
findTriangleDominantAxis(Triangle *triangle)
{
    DBL x, y, z;

    x = fabs(triangle->Normal_Vector.x);
    y = fabs(triangle->Normal_Vector.y);
    z = fabs(triangle->Normal_Vector.z);
    switch (max3(x, y, z)) {
    case 1:
        triangle->Dominant_Axis = X_AXIS;
        break;
    case 2:
        triangle->Dominant_Axis = Y_AXIS;
        break;
    case 3:
        triangle->Dominant_Axis = Z_AXIS;
        break;
    }
}

static void
computeSmoothTriangle(SmoothTriangle *triangle)
{
    Vector3D p3MinusP2;
    Vector3D vTemp1;
    Vector3D vTemp2;
    DBL x, y, z, uDenominator, proj;

    VSub(p3MinusP2, triangle->P3, triangle->P2);
    x = fabs(p3MinusP2.x);
    y = fabs(p3MinusP2.y);
    z = fabs(p3MinusP2.z);

    switch (max3(x, y, z)) {
    case 1:
        triangle->vAxis = X_AXIS;
        triangle->BaseDelta = p3MinusP2.x;
        break;

    case 2:
        triangle->vAxis = Y_AXIS;
        triangle->BaseDelta = p3MinusP2.y;
        break;

    case 3:
        triangle->vAxis = Z_AXIS;
        triangle->BaseDelta = p3MinusP2.z;
        break;
    }

    VSub(vTemp1, triangle->P2, triangle->P3);
    VNormalize(vTemp1, vTemp1);
    VSub(vTemp2, triangle->P1, triangle->P3);
    VDot(proj, vTemp2, vTemp1);
    VScale(vTemp1, vTemp1, proj);
    VSub(triangle->Perp, vTemp1, vTemp2);
    VNormalize(triangle->Perp, triangle->Perp);
    VDot(uDenominator, vTemp2, triangle->Perp);
    uDenominator = -1.0 / uDenominator;
    VScale(triangle->Perp, triangle->Perp, uDenominator);
}

int
computeTriangle(Triangle *triangle)
{
    Vector3D v1;
    Vector3D v2;
    Vector3D temp;
    DBL length;

    VSub(v1, triangle->P1, triangle->P2);
    VSub(v2, triangle->P3, triangle->P2);
    VCross(triangle->Normal_Vector, v1, v2);
    VLength(length, triangle->Normal_Vector);
    /* Set up a flag so we can ignore degenerate triangles */
    if (length < 1.0e-9) {
        triangle->Degenerate_Flag = TRUE;
        return (0);
    }

    /* Normalize the normal vector. */
    VScale(triangle->Normal_Vector, triangle->Normal_Vector, 1.0 / length);

    VDot(triangle->Distance, triangle->Normal_Vector, triangle->P1);
    triangle->Distance *= -1.0;
    findTriangleDominantAxis(triangle);

    switch (triangle->Dominant_Axis) {
    case X_AXIS:
        if ((triangle->P2.y - triangle->P3.y) *
                (triangle->P2.z - triangle->P1.z) <
            (triangle->P2.z - triangle->P3.z) *
                (triangle->P2.y - triangle->P1.y)) {

            temp = triangle->P2;
            triangle->P2 = triangle->P1;
            triangle->P1 = temp;
            if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
                temp = ((SmoothTriangle *)triangle)->N2;
                ((SmoothTriangle *)triangle)->N2 =
                    ((SmoothTriangle *)triangle)->N1;
                ((SmoothTriangle *)triangle)->N1 = temp;
            }
        }
        break;

    case Y_AXIS:
        if ((triangle->P2.x - triangle->P3.x) *
                (triangle->P2.z - triangle->P1.z) <
            (triangle->P2.z - triangle->P3.z) *
                (triangle->P2.x - triangle->P1.x)) {

            temp = triangle->P2;
            triangle->P2 = triangle->P1;
            triangle->P1 = temp;
            if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
                temp = ((SmoothTriangle *)triangle)->N2;
                ((SmoothTriangle *)triangle)->N2 =
                    ((SmoothTriangle *)triangle)->N1;
                ((SmoothTriangle *)triangle)->N1 = temp;
            }
        }
        break;

    case Z_AXIS:
        if ((triangle->P2.x - triangle->P3.x) *
                (triangle->P2.y - triangle->P1.y) <
            (triangle->P2.y - triangle->P3.y) *
                (triangle->P2.x - triangle->P1.x)) {

            temp = triangle->P2;
            triangle->P2 = triangle->P1;
            triangle->P1 = temp;
            if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
                temp = ((SmoothTriangle *)triangle)->N2;
                ((SmoothTriangle *)triangle)->N2 =
                    ((SmoothTriangle *)triangle)->N1;
                ((SmoothTriangle *)triangle)->N1 = temp;
            }
        }
        break;
    }

    if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
        computeSmoothTriangle((SmoothTriangle *)triangle);
    }
    return (1);
}

int
allTriangleIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    Triangle *shape = (Triangle *)object;
    DBL depth;
    Vector3D intersectionPoint;
    Intersection localElement;

    if (shape->Degenerate_Flag) {
        return (FALSE);
    }

    if (intersectTriangle(ray, shape, &depth)) {
        localElement.Depth = depth;
        localElement.Object = shape->Parent_Object;
        VScale(intersectionPoint, ray->Direction, depth);
        VAdd(intersectionPoint, intersectionPoint, ray->Initial);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        return (TRUE);
    }
    return (FALSE);
}

int
intersectTriangle(Ray *ray, Triangle *triangle, DBL *depth)
{
    DBL normalDotOrigin, normalDotDirection;
    DBL s, t;

    rayTriangleTests++;
    if (triangle->Degenerate_Flag) {
        return (FALSE);
    }

    if (ray == vpRay) {
        if (!triangle->VPCached) {
            VDot(triangle->VPNormDotOrigin, triangle->Normal_Vector,
                ray->Initial);
            triangle->VPNormDotOrigin += triangle->Distance;
            triangle->VPNormDotOrigin *= -1.0;
            triangle->VPCached = TRUE;
        }

        VDot(normalDotDirection, triangle->Normal_Vector, ray->Direction);
        if ((normalDotDirection < Small_Tolerance) &&
            (normalDotDirection > -Small_Tolerance)) {
            return (FALSE);
        }

        *depth = triangle->VPNormDotOrigin / normalDotDirection;
    } else {
        VDot(normalDotOrigin, triangle->Normal_Vector, ray->Initial);
        normalDotOrigin += triangle->Distance;
        normalDotOrigin *= -1.0;

        VDot(normalDotDirection, triangle->Normal_Vector, ray->Direction);
        if ((normalDotDirection < Small_Tolerance) &&
            (normalDotDirection > -Small_Tolerance)) {
            return (FALSE);
        }

        *depth = normalDotOrigin / normalDotDirection;
    }

    if ((*depth < Small_Tolerance) || (*depth > Max_Distance)) {
        return (FALSE);
    }

    switch (triangle->Dominant_Axis) {
    case X_AXIS:
        s = ray->Initial.y + *depth * ray->Direction.y;
        t = ray->Initial.z + *depth * ray->Direction.z;

        if (((triangle->P2.y - s) * (triangle->P2.z - triangle->P1.z)) <
            ((triangle->P2.z - t) * (triangle->P2.y - triangle->P1.y))) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (((triangle->P3.y - s) * (triangle->P3.z - triangle->P2.z)) <
            ((triangle->P3.z - t) * (triangle->P3.y - triangle->P2.y))) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (((triangle->P1.y - s) * (triangle->P1.z - triangle->P3.z)) <
            ((triangle->P1.z - t) * (triangle->P1.y - triangle->P3.y))) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (!(int)triangle->Inverted) {
            rayTriangleTestsSucceeded++;
            return (TRUE);
        }
        return (FALSE);

    case Y_AXIS:
        s = ray->Initial.x + *depth * ray->Direction.x;
        t = ray->Initial.z + *depth * ray->Direction.z;

        if ((triangle->P2.x - s) * (triangle->P2.z - triangle->P1.z) <
            (triangle->P2.z - t) * (triangle->P2.x - triangle->P1.x)) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P3.x - s) * (triangle->P3.z - triangle->P2.z) <
            (triangle->P3.z - t) * (triangle->P3.x - triangle->P2.x)) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P1.x - s) * (triangle->P1.z - triangle->P3.z) <
            (triangle->P1.z - t) * (triangle->P1.x - triangle->P3.x)) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (!(int)triangle->Inverted) {
            rayTriangleTestsSucceeded++;
            return (TRUE);
        }
        return (FALSE);

    case Z_AXIS:
        s = ray->Initial.x + *depth * ray->Direction.x;
        t = ray->Initial.y + *depth * ray->Direction.y;

        if ((triangle->P2.x - s) * (triangle->P2.y - triangle->P1.y) <
            (triangle->P2.y - t) * (triangle->P2.x - triangle->P1.x)) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P3.x - s) * (triangle->P3.y - triangle->P2.y) <
            (triangle->P3.y - t) * (triangle->P3.x - triangle->P2.x)) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P1.x - s) * (triangle->P1.y - triangle->P3.y) <
            (triangle->P1.y - t) * (triangle->P1.x - triangle->P3.x)) {
            if ((int)triangle->Inverted) {
                rayTriangleTestsSucceeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (!(int)triangle->Inverted) {
            rayTriangleTestsSucceeded++;
            return (TRUE);
        }
        return (FALSE);
    }
    return (FALSE);
}

int
insideTriangle(Vector3D *testPoint, SimpleBody *object)
{
    return (FALSE);
}

void
triangleNormal(
    Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    Triangle *triangle = (Triangle *)object;

    *result = triangle->Normal_Vector;
}

void *
copyTriangle(SimpleBody *object)
{
    Triangle *newShape;

    newShape = getTriangleShape();
    *newShape = *((Triangle *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
translateTriangle(SimpleBody *object, Vector3D *vector)
{
    Triangle *triangle = (Triangle *)object;
    Vector3D translation;

    VEvaluate(translation, triangle->Normal_Vector, *vector);
    triangle->Distance -= translation.x + translation.y + translation.z;
    VAdd(triangle->P1, triangle->P1, *vector)
        VAdd(triangle->P2, triangle->P2, *vector)
            VAdd(triangle->P3, triangle->P3, *vector)

                translateTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
rotateTriangle(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Triangle *triangle = (Triangle *)object;

    getRotationTransformation(&transformation, vector);
    MTransformVector(
        &triangle->Normal_Vector, &triangle->Normal_Vector, &transformation);
    MTransformVector(&triangle->P1, &triangle->P1, &transformation);
    MTransformVector(&triangle->P2, &triangle->P2, &transformation);
    MTransformVector(&triangle->P3, &triangle->P3, &transformation);
    computeTriangle(triangle);

    rotateTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
scaleTriangle(SimpleBody *object, Vector3D *vector)
{
    Triangle *triangle = (Triangle *)object;
    DBL length;

    triangle->Normal_Vector.x = triangle->Normal_Vector.x / vector->x;
    triangle->Normal_Vector.y = triangle->Normal_Vector.y / vector->y;
    triangle->Normal_Vector.z = triangle->Normal_Vector.z / vector->z;

    VLength(length, triangle->Normal_Vector);
    VScale(triangle->Normal_Vector, triangle->Normal_Vector, 1.0 / length);
    triangle->Distance /= length;

    VEvaluate(triangle->P1, triangle->P1, *vector);
    VEvaluate(triangle->P2, triangle->P2, *vector);
    VEvaluate(triangle->P3, triangle->P3, *vector);

    scaleTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
invertTriangle(SimpleBody *object)
{
    Triangle *triangle = (Triangle *)object;

    triangle->Inverted ^= TRUE;
}

/* Calculate the Phong-interpolated vector within the triangle
    at the given intersection point. The math for this is a bit
    bizarre:

     -            P1
     |          /|\ \
     |         / |Perp\
     |        /  V  \    \
     |      /    |     \    \
  u |     /____|_____PI___\
     |    /      |         \     \
     -  P2-----|--------|----P3
                  Pbase     PIntersect
          |-------------------|
                              v

    triangle->Perp is a unit vector from P1 to Pbase. We calculate

    u = (PI - P1) DOT Perp / ((P3 - P1) DOT Perp).

    We then calculate where the line from P1 to PI intersects the line P2 to P3:
    PIntersect = (PI - P1)/u.

    We really only need one coordinate of PIntersect.  We then calculate v as:

        v = PIntersect.x / (P3.x - P2.x)
 or    v = PIntersect.y / (P3.y - P2.y)
 or    v = PIntersect.z / (P3.z - P2.z)

    depending on which calculation will give us the best answers.

    Once we have u and v, we can perform the normal interpolation as:

      NTemp1 = N1 + u(N2 - N1);
      NTemp2 = N1 + u(N3 - N1);
      Result = normalize (NTemp1 + v(NTemp2 - NTemp1))

    As always, any values which are constant for the triangle are cached
    in the triangle.
*/

void
smoothTriangleNormal(
    Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;
    Vector3D piMinusP1;
    Vector3D nTemp1;
    Vector3D nTemp2;
    DBL u = 0.0, v = 0.0;

    VSub(piMinusP1, *intersectionPoint, triangle->P1);
    VDot(u, piMinusP1, triangle->Perp);
    if (u < 1.0e-9) {
        *result = triangle->N1;
        return;
    }

    /* BaseDelta contains P3.x-P2.x,  P3.y-P2.y, or P3.z-P2.z depending on the
        value of vAxis. */

    switch (triangle->vAxis) {
    case X_AXIS:
        v = (piMinusP1.x / u + triangle->P1.x - triangle->P2.x) /
            triangle->BaseDelta;
        break;

    case Y_AXIS:
        v = (piMinusP1.y / u + triangle->P1.y - triangle->P2.y) /
            triangle->BaseDelta;
        break;

    case Z_AXIS:
        v = (piMinusP1.z / u + triangle->P1.z - triangle->P2.z) /
            triangle->BaseDelta;
        break;
    }

    VSub(nTemp1, triangle->N2, triangle->N1);
    VScale(nTemp1, nTemp1, u);
    VAdd(nTemp1, nTemp1, triangle->N1);
    VSub(nTemp2, triangle->N3, triangle->N1);
    VScale(nTemp2, nTemp2, u);
    VAdd(nTemp2, nTemp2, triangle->N1);
    VSub(*result, nTemp2, nTemp1);
    VScale(*result, *result, v);
    VAdd(*result, *result, nTemp1);
    VNormalize(*result, *result);
}

void *
copySmoothTriangle(SimpleBody *object)
{
    SmoothTriangle *newShape;

    newShape = getSmoothTriangleShape();
    *newShape = *((SmoothTriangle *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
rotateSmoothTriangle(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    SmoothTriangle *triangle = (SmoothTriangle *)object;

    getRotationTransformation(&transformation, vector);
    MTransformVector(
        &triangle->Normal_Vector, &triangle->Normal_Vector, &transformation);
    MTransformVector(&triangle->P1, &triangle->P1, &transformation);
    MTransformVector(&triangle->P2, &triangle->P2, &transformation);
    MTransformVector(&triangle->P3, &triangle->P3, &transformation);
    MTransformVector(&triangle->N1, &triangle->N1, &transformation);
    MTransformVector(&triangle->N2, &triangle->N2, &transformation);
    MTransformVector(&triangle->N3, &triangle->N3, &transformation);
    computeTriangle((Triangle *)triangle);

    rotateTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
translateSmoothTriangle(SimpleBody *object, Vector3D *vector)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;
    Vector3D translation;

    VEvaluate(translation, triangle->Normal_Vector, *vector);
    triangle->Distance -= translation.x + translation.y + translation.z;
    VAdd(triangle->P1, triangle->P1, *vector)
        VAdd(triangle->P2, triangle->P2, *vector)
            VAdd(triangle->P3, triangle->P3, *vector)
                computeTriangle((Triangle *)triangle);

    translateTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
scaleSmoothTriangle(SimpleBody *object, Vector3D *vector)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;
    DBL length;

    triangle->Normal_Vector.x = triangle->Normal_Vector.x / vector->x;
    triangle->Normal_Vector.y = triangle->Normal_Vector.y / vector->y;
    triangle->Normal_Vector.z = triangle->Normal_Vector.z / vector->z;

    VLength(length, triangle->Normal_Vector);
    VScale(triangle->Normal_Vector, triangle->Normal_Vector, 1.0 / length);
    triangle->Distance /= length;

    VEvaluate(triangle->P1, triangle->P1, *vector);
    VEvaluate(triangle->P2, triangle->P2, *vector);
    VEvaluate(triangle->P3, triangle->P3, *vector);
    computeTriangle((Triangle *)triangle);

    scaleTexture(&((SmoothTriangle *)object)->Shape_Texture, vector);
}

void
invertSmoothTriangle(SimpleBody *object)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;

    triangle->Inverted ^= TRUE;
}
