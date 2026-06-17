/**
This module implements smooth triangles.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/Triangle.h"

/**
Calculate the Phong-interpolated vector within the triangle
at the given intersection point. The math for this is a bit
bizarre:

 -            p1
 |          /|\ \
 |         / |perp\
 |        /  V  \    \
 |      /    |     \    \
u |     /____|_____PI___\
 |    /      |         \     \
 -  p2-----|--------|----p3
              Pbase     PIntersect
      |-------------------|
                          v

triangle->perp is a unit vector from p1 to Pbase. We calculate

u = (PI - p1) DOT perp / ((p3 - p1) DOT perp).

We then calculate where the line from p1 to PI intersects the line p2 to p3:
PIntersect = (PI - p1)/u.

We really only need one coordinate of PIntersect.  We then calculate v as:

    v = PIntersect.x / (p3.x - p2.x)
or    v = PIntersect.y / (p3.y - p2.y)
or    v = PIntersect.z / (p3.z - p2.z)

depending on which calculation will give us the best answers.

Once we have u and v, we can perform the normal interpolation as:

  NTemp1 = n1 + u(n2 - n1);
  NTemp2 = n1 + u(n3 - n1);
  Result = normalize (NTemp1 + v(NTemp2 - NTemp1))

As always, any values which are constant for the triangle are cached
in the triangle.
*/

static constexpr int X_AXIS = 0;
static constexpr int Y_AXIS = 1;
static constexpr int Z_AXIS = 2;

void
SmoothTriangle::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const SmoothTriangle *triangle = this;
    Vector3Dd piMinusP1;
    Vector3Dd nTemp1;
    Vector3Dd nTemp2;
    double u = 0.0;
    double v = 0.0;

    piMinusP1 = intersectionPoint->subtract(triangle->p1);
    u = piMinusP1.dotProduct(triangle->perp);
    if (u < 1.0e-9) {
        *result = triangle->n1;
        return;
    }

    // BaseDelta contains p3.x-p2.x,  p3.y-p2.y, or p3.z-p2.z depending on the
    // value of vAxis.

    switch (triangle->vAxis) {
    case X_AXIS:
        v = (piMinusP1.x() / u + triangle->p1.x() - triangle->p2.x()) /
            triangle->baseDelta;
        break;

    case Y_AXIS:
        v = (piMinusP1.y() / u + triangle->p1.y() - triangle->p2.y()) /
            triangle->baseDelta;
        break;

    case Z_AXIS:
        v = (piMinusP1.z() / u + triangle->p1.z() - triangle->p2.z()) /
            triangle->baseDelta;
        break;
    }

    nTemp1 = triangle->n2.subtract(triangle->n1);
    nTemp1 = nTemp1.multiply(u);
    nTemp1 = nTemp1.add(triangle->n1);
    nTemp2 = triangle->n3.subtract(triangle->n1);
    nTemp2 = nTemp2.multiply(u);
    nTemp2 = nTemp2.add(triangle->n1);
    *result = nTemp2.subtract(nTemp1);
    *result = (*result).multiply(v);
    *result = result->add(nTemp1);
    *result = (*result).normalizedFast();
}

void *
SmoothTriangle::copy()
{
    SmoothTriangle *newShape;

    newShape = new SmoothTriangle;
    *newShape = *this;

    return (newShape);
}

void
SmoothTriangle::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;
    SmoothTriangle * const triangle = this;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    triangle->normalVector = transformation.transpose().multiply(triangle->normalVector);
    triangle->p1 = transformation.transpose().multiply(triangle->p1);
    triangle->p2 = transformation.transpose().multiply(triangle->p2);
    triangle->p3 = transformation.transpose().multiply(triangle->p3);
    triangle->n1 = transformation.transpose().multiply(triangle->n1);
    triangle->n2 = transformation.transpose().multiply(triangle->n2);
    triangle->n3 = transformation.transpose().multiply(triangle->n3);
    Triangle::computeTriangle((Triangle *)triangle);
}

void
SmoothTriangle::translateGeometry(Vector3Dd *vector)
{
    SmoothTriangle * const triangle = this;
    Vector3Dd translation;

    translation = triangle->normalVector.multiply(*vector);
    triangle->distance -= translation.x() + translation.y() + translation.z();
    triangle->p1 = triangle->p1.add(*vector);
    triangle->p2 = triangle->p2.add(*vector);
    triangle->p3 = triangle->p3.add(*vector);
    Triangle::computeTriangle((Triangle *)triangle);
}

void
SmoothTriangle::scaleGeometry(Vector3Dd *vector)
{
    SmoothTriangle * const triangle = this;
    double length;

    triangle->normalVector = Vector3Dd(
        triangle->normalVector.x() / vector->x(),
        triangle->normalVector.y() / vector->y(),
        triangle->normalVector.z() / vector->z());

    length = triangle->normalVector.length();
    triangle->normalVector = triangle->normalVector.multiply(1.0 / length);
    triangle->distance /= length;

    triangle->p1 = triangle->p1.multiply(*vector);
    triangle->p2 = triangle->p2.multiply(*vector);
    triangle->p3 = triangle->p3.multiply(*vector);
    Triangle::computeTriangle((Triangle *)triangle);
}

void
SmoothTriangle::invertGeometry()
{
    this->inverted ^= true;
}

void
SmoothTriangle::swapVertexNormals()
{
    Vector3Dd temp = this->n2;
    this->n2 = this->n1;
    this->n1 = temp;
}

void
SmoothTriangle::finalizeComputation()
{
    Triangle::computeSmoothTriangle(this);
}
