/**
This module implements smooth triangles.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/Triangle.h"

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

    piMinusP1 = intersectionPoint->subtract(triangle->getP1());
    u = piMinusP1.dotProduct(triangle->getPerp());
    if (u < 1.0e-9) {
        *result = triangle->getN1();
        return;
    }

    // BaseDelta contains p3.x-p2.x,  p3.y-p2.y, or p3.z-p2.z depending on the
    // value of vAxis.

    switch (triangle->getVAxis()) {
    case X_AXIS:
        v = (piMinusP1.x() / u + triangle->getP1().x() - triangle->getP2().x()) /
            triangle->getBaseDelta();
        break;

    case Y_AXIS:
        v = (piMinusP1.y() / u + triangle->getP1().y() - triangle->getP2().y()) /
            triangle->getBaseDelta();
        break;

    case Z_AXIS:
        v = (piMinusP1.z() / u + triangle->getP1().z() - triangle->getP2().z()) /
            triangle->getBaseDelta();
        break;
    }

    nTemp1 = triangle->getN2().subtract(triangle->getN1());
    nTemp1 = nTemp1.multiply(u);
    nTemp1 = nTemp1.add(triangle->getN1());
    nTemp2 = triangle->getN3().subtract(triangle->getN1());
    nTemp2 = nTemp2.multiply(u);
    nTemp2 = nTemp2.add(triangle->getN1());
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
    triangle->getNormalVector() =
        transformation.transpose().multiply(triangle->getNormalVector());
    triangle->getP1() = transformation.transpose().multiply(triangle->getP1());
    triangle->getP2() = transformation.transpose().multiply(triangle->getP2());
    triangle->getP3() = transformation.transpose().multiply(triangle->getP3());
    triangle->getN1() = transformation.transpose().multiply(triangle->getN1());
    triangle->getN2() = transformation.transpose().multiply(triangle->getN2());
    triangle->getN3() = transformation.transpose().multiply(triangle->getN3());
    Triangle::computeTriangle((Triangle *)triangle);
}

void
SmoothTriangle::translateGeometry(Vector3Dd *vector)
{
    SmoothTriangle * const triangle = this;
    Vector3Dd translation;

    translation = triangle->getNormalVector().multiply(*vector);
    triangle->setDistance(
        triangle->getDistance() - translation.x() - translation.y() - translation.z());
    triangle->getP1() = triangle->getP1().add(*vector);
    triangle->getP2() = triangle->getP2().add(*vector);
    triangle->getP3() = triangle->getP3().add(*vector);
    Triangle::computeTriangle((Triangle *)triangle);
}

void
SmoothTriangle::scaleGeometry(Vector3Dd *vector)
{
    SmoothTriangle * const triangle = this;
    double length;

    triangle->getNormalVector() = Vector3Dd(
        triangle->getNormalVector().x() / vector->x(),
        triangle->getNormalVector().y() / vector->y(),
        triangle->getNormalVector().z() / vector->z());

    length = triangle->getNormalVector().length();
    triangle->getNormalVector() = triangle->getNormalVector().multiply(1.0 / length);
    triangle->setDistance(triangle->getDistance() / length);

    triangle->getP1() = triangle->getP1().multiply(*vector);
    triangle->getP2() = triangle->getP2().multiply(*vector);
    triangle->getP3() = triangle->getP3().multiply(*vector);
    Triangle::computeTriangle((Triangle *)triangle);
}

void
SmoothTriangle::invertGeometry()
{
    this->toggleInverted();
}

void
SmoothTriangle::swapVertexNormals()
{
    Vector3Dd temp = this->getN2();
    this->getN2() = this->getN1();
    this->getN1() = temp;
}

void
SmoothTriangle::finalizeComputation()
{
    Triangle::computeSmoothTriangle(this);
}
