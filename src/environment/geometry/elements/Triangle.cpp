#include "environment/scene/factory/ModelFactory.h"
/****************************************************************************
 *                     triangle.c
 *
 *  This module implements primitives for triangles and smooth triangles.
 *
 *****************************************************************************/

#include "environment/geometry/elements/Triangle.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/Composite.h"
Methods Triangle_Methods = {Composite::objectIntersect,
    Triangle::allTriangleIntersections, Triangle::insideTriangle,
    Triangle::triangleNormal, Triangle::copyTriangle,
    Triangle::translateTriangle, Triangle::rotateTriangle,
    Triangle::scaleTriangle, Triangle::invertTriangle};

Methods Smooth_Triangle_Methods = {Composite::objectIntersect,
    Triangle::allTriangleIntersections, Triangle::insideTriangle,
    SmoothTriangle::smoothTriangleNormal, SmoothTriangle::copySmoothTriangle,
    SmoothTriangle::translateSmoothTriangle,
    SmoothTriangle::rotateSmoothTriangle, SmoothTriangle::scaleSmoothTriangle,
    SmoothTriangle::invertSmoothTriangle};

extern RayWithSegments *vpRay;
extern long rayTriangleTests, rayTriangleTestsSucceeded;

int
Triangle::max3Axis(double x, double y, double z)
{
    return (x > y) ? ((x > z) ? 1 : 3) : ((y > z) ? 2 : 3);
}
static constexpr int X_AXIS = 0;
static constexpr int Y_AXIS = 1;
static constexpr int Z_AXIS = 2;
void
Triangle::findTriangleDominantAxis(Triangle *triangle)
{
    double x, y, z;

    x = fabs(triangle->Normal_Vector.x);
    y = fabs(triangle->Normal_Vector.y);
    z = fabs(triangle->Normal_Vector.z);
    switch (Triangle::max3Axis(x, y, z)) {
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

void
Triangle::computeSmoothTriangle(SmoothTriangle *triangle)
{
    Vector3Dd p3MinusP2;
    Vector3Dd vTemp1;
    Vector3Dd vTemp2;
    double x, y, z, uDenominator, proj;

    VectorOps::vSub(p3MinusP2, triangle->P3, triangle->P2);
    x = fabs(p3MinusP2.x);
    y = fabs(p3MinusP2.y);
    z = fabs(p3MinusP2.z);

    switch (Triangle::max3Axis(x, y, z)) {
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

    VectorOps::vSub(vTemp1, triangle->P2, triangle->P3);
    vTemp1.normalize();
    VectorOps::vSub(vTemp2, triangle->P1, triangle->P3);
    proj = vTemp2.dotProduct(vTemp1);
    vTemp1.scale(proj);
    VectorOps::vSub(triangle->Perp, vTemp1, vTemp2);
    triangle->Perp.normalize();
    uDenominator = vTemp2.dotProduct(triangle->Perp);
    uDenominator = -1.0 / uDenominator;
    triangle->Perp.scale(uDenominator);
}

int
Triangle::computeTriangle(Triangle *triangle)
{
    Vector3Dd v1;
    Vector3Dd v2;
    Vector3Dd temp;
    double length;

    VectorOps::vSub(v1, triangle->P1, triangle->P2);
    VectorOps::vSub(v2, triangle->P3, triangle->P2);
    triangle->Normal_Vector = v1.crossProduct(v2);
    length = triangle->Normal_Vector.length();
    /* Set up a flag so we can ignore degenerate triangles */
    if (length < 1.0e-9) {
        triangle->Degenerate_Flag = TRUE;
        return (0);
    }

    /* Normalize the normal vector. */
    triangle->Normal_Vector.scale(1.0 / length);

    triangle->Distance = triangle->Normal_Vector.dotProduct(triangle->P1);
    triangle->Distance *= -1.0;
    Triangle::findTriangleDominantAxis(triangle);

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
        Triangle::computeSmoothTriangle((SmoothTriangle *)triangle);
    }
    return (1);
}

int
Triangle::allTriangleIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    Triangle *shape = (Triangle *)object;
    double depth;
    Vector3Dd intersectionPoint;
    Intersection localElement;

    if (shape->Degenerate_Flag) {
        return (FALSE);
    }

    if (intersectTriangle(ray, shape, &depth)) {
        localElement.Depth = depth;
        localElement.Object = shape->Parent_Object;
        VectorOps::vScale(intersectionPoint, ray->direction, depth);
        intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        return (TRUE);
    }
    return (FALSE);
}

int
Triangle::intersectTriangle(
    RayWithSegments *ray, Triangle *triangle, double *depth)
{
    double normalDotOrigin, normalDotDirection;
    double s, t;

    rayTriangleTests++;
    if (triangle->Degenerate_Flag) {
        return (FALSE);
    }

    if (ray == vpRay) {
        if (!triangle->VPCached) {
            VectorOps::vDot(triangle->VPNormDotOrigin, triangle->Normal_Vector,
                ray->position);
            triangle->VPNormDotOrigin += triangle->Distance;
            triangle->VPNormDotOrigin *= -1.0;
            triangle->VPCached = TRUE;
        }

        normalDotDirection = triangle->Normal_Vector.dotProduct(ray->direction);
        if ((normalDotDirection < Small_Tolerance) &&
            (normalDotDirection > -Small_Tolerance)) {
            return (FALSE);
        }

        *depth = triangle->VPNormDotOrigin / normalDotDirection;
    } else {
        normalDotOrigin = triangle->Normal_Vector.dotProduct(ray->position);
        normalDotOrigin += triangle->Distance;
        normalDotOrigin *= -1.0;

        normalDotDirection = triangle->Normal_Vector.dotProduct(ray->direction);
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
        s = ray->position.y + *depth * ray->direction.y;
        t = ray->position.z + *depth * ray->direction.z;

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
        s = ray->position.x + *depth * ray->direction.x;
        t = ray->position.z + *depth * ray->direction.z;

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
        s = ray->position.x + *depth * ray->direction.x;
        t = ray->position.y + *depth * ray->direction.y;

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
Triangle::insideTriangle(Vector3Dd *testPoint, SimpleBody *object)
{
    return (FALSE);
}

void
Triangle::triangleNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Triangle *triangle = (Triangle *)object;

    *result = triangle->Normal_Vector;
}

void *
Triangle::copyTriangle(SimpleBody *object)
{
    Triangle *newShape;

    newShape = ModelFactory::getTriangleShape();
    *newShape = *((Triangle *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Triangle::translateTriangle(SimpleBody *object, Vector3Dd *vector)
{
    Triangle *triangle = (Triangle *)object;
    Vector3Dd translation;

    VectorOps::vEvaluate(translation, triangle->Normal_Vector, *vector);
    triangle->Distance -= translation.x + translation.y + translation.z;
    triangle->P1.add(*vector);
    triangle->P2.add(*vector);
    triangle->P3.add(*vector);
    TextureUtils::translateTexture(
        &((Triangle *)object)->Shape_Texture, vector);
}

void
Triangle::rotateTriangle(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Triangle *triangle = (Triangle *)object;

    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(
        &triangle->Normal_Vector, &triangle->Normal_Vector, &transformation);
    Transformation::MTransformVector(
        &triangle->P1, &triangle->P1, &transformation);
    Transformation::MTransformVector(
        &triangle->P2, &triangle->P2, &transformation);
    Transformation::MTransformVector(
        &triangle->P3, &triangle->P3, &transformation);
    Triangle::computeTriangle(triangle);

    TextureUtils::rotateTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
Triangle::scaleTriangle(SimpleBody *object, Vector3Dd *vector)
{
    Triangle *triangle = (Triangle *)object;
    double length;

    triangle->Normal_Vector.x = triangle->Normal_Vector.x / vector->x;
    triangle->Normal_Vector.y = triangle->Normal_Vector.y / vector->y;
    triangle->Normal_Vector.z = triangle->Normal_Vector.z / vector->z;

    length = triangle->Normal_Vector.length();
    triangle->Normal_Vector.scale(1.0 / length);
    triangle->Distance /= length;

    triangle->P1.evaluate(*vector);
    triangle->P2.evaluate(*vector);
    triangle->P3.evaluate(*vector);

    TextureUtils::scaleTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
Triangle::invertTriangle(SimpleBody *object)
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
SmoothTriangle::smoothTriangleNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;
    Vector3Dd piMinusP1;
    Vector3Dd nTemp1;
    Vector3Dd nTemp2;
    double u = 0.0, v = 0.0;

    VectorOps::vSub(piMinusP1, *intersectionPoint, triangle->P1);
    u = piMinusP1.dotProduct(triangle->Perp);
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

    VectorOps::vSub(nTemp1, triangle->N2, triangle->N1);
    nTemp1.scale(u);
    nTemp1.add(triangle->N1);
    VectorOps::vSub(nTemp2, triangle->N3, triangle->N1);
    nTemp2.scale(u);
    nTemp2.add(triangle->N1);
    VectorOps::vSub(*result, nTemp2, nTemp1);
    (*result).scale(v);
    (*result).add(nTemp1);
    (*result).normalize();
}

void *
SmoothTriangle::copySmoothTriangle(SimpleBody *object)
{
    SmoothTriangle *newShape;

    newShape = ModelFactory::getSmoothTriangleShape();
    *newShape = *((SmoothTriangle *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
SmoothTriangle::rotateSmoothTriangle(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    SmoothTriangle *triangle = (SmoothTriangle *)object;

    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(
        &triangle->Normal_Vector, &triangle->Normal_Vector, &transformation);
    Transformation::MTransformVector(
        &triangle->P1, &triangle->P1, &transformation);
    Transformation::MTransformVector(
        &triangle->P2, &triangle->P2, &transformation);
    Transformation::MTransformVector(
        &triangle->P3, &triangle->P3, &transformation);
    Transformation::MTransformVector(
        &triangle->N1, &triangle->N1, &transformation);
    Transformation::MTransformVector(
        &triangle->N2, &triangle->N2, &transformation);
    Transformation::MTransformVector(
        &triangle->N3, &triangle->N3, &transformation);
    Triangle::computeTriangle((Triangle *)triangle);

    TextureUtils::rotateTexture(&((Triangle *)object)->Shape_Texture, vector);
}

void
SmoothTriangle::translateSmoothTriangle(SimpleBody *object, Vector3Dd *vector)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;
    Vector3Dd translation;

    VectorOps::vEvaluate(translation, triangle->Normal_Vector, *vector);
    triangle->Distance -= translation.x + translation.y + translation.z;
    triangle->P1.add(*vector);
    triangle->P2.add(*vector);
    triangle->P3.add(*vector);
    Triangle::computeTriangle((Triangle *)triangle);

    TextureUtils::translateTexture(
        &((Triangle *)object)->Shape_Texture, vector);
}

void
SmoothTriangle::scaleSmoothTriangle(SimpleBody *object, Vector3Dd *vector)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;
    double length;

    triangle->Normal_Vector.x = triangle->Normal_Vector.x / vector->x;
    triangle->Normal_Vector.y = triangle->Normal_Vector.y / vector->y;
    triangle->Normal_Vector.z = triangle->Normal_Vector.z / vector->z;

    length = triangle->Normal_Vector.length();
    triangle->Normal_Vector.scale(1.0 / length);
    triangle->Distance /= length;

    triangle->P1.evaluate(*vector);
    triangle->P2.evaluate(*vector);
    triangle->P3.evaluate(*vector);
    Triangle::computeTriangle((Triangle *)triangle);

    TextureUtils::scaleTexture(
        &((SmoothTriangle *)object)->Shape_Texture, vector);
}

void
SmoothTriangle::invertSmoothTriangle(SimpleBody *object)
{
    SmoothTriangle *triangle = (SmoothTriangle *)object;

    triangle->Inverted ^= TRUE;
}
