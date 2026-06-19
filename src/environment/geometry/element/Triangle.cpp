/**
This module implements primitives for triangles and smooth triangles.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/element/Triangle.h"

Triangle::Triangle() :
    normalVector(0.0, 1.0, 0.0),
    distance(0.0),
    vpNormDotOrigin(0.0),
    vpCached(false),
    dominantAxis(0),
    inverted(false),
    vAxis(0),
    p1(0.0, 0.0, 0.0),
    p2(1.0, 0.0, 0.0),
    p3(0.0, 1.0, 0.0),
    degenerateFlag(false)
{
}

Triangle::Triangle(const Vector3Dd &p1, const Vector3Dd &p2, const Vector3Dd &p3,
    bool inverted) :
    normalVector(0.0, 1.0, 0.0),
    distance(0.0),
    vpNormDotOrigin(0.0),
    vpCached(false),
    dominantAxis(0),
    inverted(inverted),
    vAxis(0),
    p1(p1),
    p2(p2),
    p3(p3),
    degenerateFlag(false)
{
    Triangle::computeTriangle(this);
}

Triangle::Triangle(const Vector3Dd &normalVector, double distance,
    double vpNormDotOrigin, bool vpCached, unsigned int dominantAxis,
    bool inverted, unsigned int vAxis, const Vector3Dd &p1,
    const Vector3Dd &p2, const Vector3Dd &p3, bool degenerateFlag) :
    normalVector(normalVector),
    distance(distance),
    vpNormDotOrigin(vpNormDotOrigin),
    vpCached(vpCached),
    dominantAxis(dominantAxis),
    inverted(inverted),
    vAxis(vAxis),
    p1(p1),
    p2(p2),
    p3(p3),
    degenerateFlag(degenerateFlag)
{
}

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
    double x;
    double y;
    double z;

    x = java::Math::abs(triangle->normalVector.x());
    y = java::Math::abs(triangle->normalVector.y());
    z = java::Math::abs(triangle->normalVector.z());
    switch (Triangle::max3Axis(x, y, z)) {
    case 1:
        triangle->dominantAxis = X_AXIS;
        break;
    case 2:
        triangle->dominantAxis = Y_AXIS;
        break;
    case 3:
        triangle->dominantAxis = Z_AXIS;
        break;
    }
}

void
Triangle::computeSmoothTriangle(SmoothTriangle *triangle)
{
    Vector3Dd p3MinusP2;
    Vector3Dd vTemp1;
    Vector3Dd vTemp2;
    double x;
    double y;
    double z;
    double uDenominator;
    double proj;

    p3MinusP2 = triangle->getP3().subtract(triangle->getP2());
    x = java::Math::abs(p3MinusP2.x());
    y = java::Math::abs(p3MinusP2.y());
    z = java::Math::abs(p3MinusP2.z());

    switch (Triangle::max3Axis(x, y, z)) {
    case 1:
        triangle->setVAxis(X_AXIS);
        triangle->setBaseDelta(p3MinusP2.x());
        break;

    case 2:
        triangle->setVAxis(Y_AXIS);
        triangle->setBaseDelta(p3MinusP2.y());
        break;

    case 3:
        triangle->setVAxis(Z_AXIS);
        triangle->setBaseDelta(p3MinusP2.z());
        break;
    }

    vTemp1 = triangle->getP2().subtract(triangle->getP3());
    vTemp1 = vTemp1.normalizedFast();
    vTemp2 = triangle->getP1().subtract(triangle->getP3());
    proj = vTemp2.dotProduct(vTemp1);
    vTemp1 = vTemp1.multiply(proj);
    triangle->getPerp() = vTemp1.subtract(vTemp2);
    triangle->getPerp() = triangle->getPerp().normalizedFast();
    uDenominator = vTemp2.dotProduct(triangle->getPerp());
    uDenominator = -1.0 / uDenominator;
    triangle->getPerp() = triangle->getPerp().multiply(uDenominator);
}

int
Triangle::computeTriangle(Triangle *triangle)
{
    Vector3Dd v1;
    Vector3Dd v2;
    Vector3Dd temp;
    double length;

    v1 = triangle->p1.subtract(triangle->p2);
    v2 = triangle->p3.subtract(triangle->p2);
    triangle->normalVector = v1.crossProduct(v2);
    length = triangle->normalVector.length();
    // Set up a flag so we can ignore degenerate triangles
    if (length < 1.0e-9) {
        triangle->degenerateFlag = true;
        return (0);
    }

    // Normalize the normal vector
    triangle->normalVector = triangle->normalVector.multiply(1.0 / length);

    triangle->distance = triangle->normalVector.dotProduct(triangle->p1);
    triangle->distance *= -1.0;
    Triangle::findTriangleDominantAxis(triangle);

    switch (triangle->dominantAxis) {
    case X_AXIS:
        if ((triangle->p2.y() - triangle->p3.y()) *
                (triangle->p2.z() - triangle->p1.z()) <
            (triangle->p2.z() - triangle->p3.z()) *
                (triangle->p2.y() - triangle->p1.y())) {

            temp = triangle->p2;
            triangle->p2 = triangle->p1;
            triangle->p1 = temp;
            triangle->swapVertexNormals();
        }
        break;

    case Y_AXIS:
        if ((triangle->p2.x() - triangle->p3.x()) *
                (triangle->p2.z() - triangle->p1.z()) <
            (triangle->p2.z() - triangle->p3.z()) *
                (triangle->p2.x() - triangle->p1.x())) {

            temp = triangle->p2;
            triangle->p2 = triangle->p1;
            triangle->p1 = temp;
            triangle->swapVertexNormals();
        }
        break;

    case Z_AXIS:
        if ((triangle->p2.x() - triangle->p3.x()) *
                (triangle->p2.y() - triangle->p1.y()) <
            (triangle->p2.y() - triangle->p3.y()) *
                (triangle->p2.x() - triangle->p1.x())) {

            temp = triangle->p2;
            triangle->p2 = triangle->p1;
            triangle->p1 = temp;
            triangle->swapVertexNormals();
        }
        break;
    }

    triangle->finalizeComputation();
    return (1);
}

int
Triangle::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    Triangle * const shape = this;
    double depth;
    Vector3Dd intersectionPoint;
    Intersection localElement;

    if (shape->degenerateFlag) {
        return (false);
    }

    if (intersectTriangle(ray, shape, &depth)) {
        localElement.setDepth(depth);
        localElement.setObject(nullptr);
        intersectionPoint = ray->getDirection().multiply(depth);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.setPoint(intersectionPoint);
        localElement.setShape(reinterpret_cast<SimpleBody *>(shape));
        depthQueue->offer(localElement);
        return (true);
    }
    return (false);
}

int
Triangle::intersectTriangle(
    RayWithSegments *ray, Triangle *triangle, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;
    double s;
    double t;

    Statistics::global().incrementRayTriangleTests();
    if (triangle->degenerateFlag) {
        return (false);
    }

    if (ray->isPrimaryRayEnabled()) {
        if (!triangle->vpCached) {
            triangle->vpNormDotOrigin =
                triangle->normalVector.dotProduct(ray->getOrigin());
            triangle->vpNormDotOrigin += triangle->distance;
            triangle->vpNormDotOrigin *= -1.0;
            triangle->vpCached = true;
        }

        normalDotDirection = triangle->normalVector.dotProduct(ray->getDirection());
        if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
            (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
            return (false);
        }

        *depth = triangle->vpNormDotOrigin / normalDotDirection;
    } else {
        normalDotOrigin = triangle->normalVector.dotProduct(ray->getOrigin());
        normalDotOrigin += triangle->distance;
        normalDotOrigin *= -1.0;

        normalDotDirection = triangle->normalVector.dotProduct(ray->getDirection());
        if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
            (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
            return (false);
        }

        *depth = normalDotOrigin / normalDotDirection;
    }

    if ((*depth < GeometryConstants::Small_Tolerance) || (*depth > GeometryConstants::Max_Distance)) {
        return (false);
    }

    switch (triangle->dominantAxis) {
    case X_AXIS:
        s = ray->getOrigin().y() + *depth * ray->getDirection().y();
        t = ray->getOrigin().z() + *depth * ray->getDirection().z();

        if (((triangle->p2.y() - s) * (triangle->p2.z() - triangle->p1.z())) <
            ((triangle->p2.z() - t) * (triangle->p2.y() - triangle->p1.y()))) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (((triangle->p3.y() - s) * (triangle->p3.z() - triangle->p2.z())) <
            ((triangle->p3.z() - t) * (triangle->p3.y() - triangle->p2.y()))) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (((triangle->p1.y() - s) * (triangle->p1.z() - triangle->p3.z())) <
            ((triangle->p1.z() - t) * (triangle->p1.y() - triangle->p3.y()))) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            Statistics::global().incrementRayTriangleTestsSucceeded();
            return (true);
        }
        return (false);

    case Y_AXIS:
        s = ray->getOrigin().x() + *depth * ray->getDirection().x();
        t = ray->getOrigin().z() + *depth * ray->getDirection().z();

        if ((triangle->p2.x() - s) * (triangle->p2.z() - triangle->p1.z()) <
            (triangle->p2.z() - t) * (triangle->p2.x() - triangle->p1.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p3.x() - s) * (triangle->p3.z() - triangle->p2.z()) <
            (triangle->p3.z() - t) * (triangle->p3.x() - triangle->p2.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p1.x() - s) * (triangle->p1.z() - triangle->p3.z()) <
            (triangle->p1.z() - t) * (triangle->p1.x() - triangle->p3.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            Statistics::global().incrementRayTriangleTestsSucceeded();
            return (true);
        }
        return (false);

    case Z_AXIS:
        s = ray->getOrigin().x() + *depth * ray->getDirection().x();
        t = ray->getOrigin().y() + *depth * ray->getDirection().y();

        if ((triangle->p2.x() - s) * (triangle->p2.y() - triangle->p1.y()) <
            (triangle->p2.y() - t) * (triangle->p2.x() - triangle->p1.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p3.x() - s) * (triangle->p3.y() - triangle->p2.y()) <
            (triangle->p3.y() - t) * (triangle->p3.x() - triangle->p2.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p1.x() - s) * (triangle->p1.y() - triangle->p3.y()) <
            (triangle->p1.y() - t) * (triangle->p1.x() - triangle->p3.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            Statistics::global().incrementRayTriangleTestsSucceeded();
            return (true);
        }
        return (false);
    }
    return (false);
}

int
Triangle::inside(Vector3Dd *point)
{
    return (false);
}

void
Triangle::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const Triangle *triangle = this;

    *result = triangle->normalVector;
}

void *
Triangle::copy()
{
    return new Triangle(normalVector, distance, vpNormDotOrigin, vpCached,
        dominantAxis, inverted, vAxis, p1, p2, p3, degenerateFlag);
}

void
Triangle::translateGeometry(Vector3Dd *vector)
{
    Triangle * const triangle = this;
    Vector3Dd translation;

    translation = triangle->normalVector.multiply(*vector);
    triangle->distance -= translation.x() + translation.y() + translation.z();
    triangle->p1 = triangle->p1.add(*vector);
    triangle->p2 = triangle->p2.add(*vector);
    triangle->p3 = triangle->p3.add(*vector);
}

void
Triangle::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;
    Triangle * const triangle = this;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    triangle->normalVector = transformation.transpose().multiply(triangle->normalVector);
    triangle->p1 = transformation.transpose().multiply(triangle->p1);
    triangle->p2 = transformation.transpose().multiply(triangle->p2);
    triangle->p3 = transformation.transpose().multiply(triangle->p3);
    Triangle::computeTriangle(triangle);
}

void
Triangle::scaleGeometry(Vector3Dd *vector)
{
    Triangle * const triangle = this;
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
}

void
Triangle::invertGeometry()
{
    this->inverted ^= true;
}

#include "java/util/PriorityQueue.txx"
