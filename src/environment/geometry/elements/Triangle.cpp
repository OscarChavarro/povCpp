/**
This module implements primitives for triangles and smooth triangles.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/elements/Triangle.h"

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

    p3MinusP2 = triangle->p3.subtract(triangle->p2);
    x = java::Math::abs(p3MinusP2.x());
    y = java::Math::abs(p3MinusP2.y());
    z = java::Math::abs(p3MinusP2.z());

    switch (Triangle::max3Axis(x, y, z)) {
    case 1:
        triangle->vAxis = X_AXIS;
        triangle->baseDelta = p3MinusP2.x();
        break;

    case 2:
        triangle->vAxis = Y_AXIS;
        triangle->baseDelta = p3MinusP2.y();
        break;

    case 3:
        triangle->vAxis = Z_AXIS;
        triangle->baseDelta = p3MinusP2.z();
        break;
    }

    vTemp1 = triangle->p2.subtract(triangle->p3);
    vTemp1 = vTemp1.normalizedFast();
    vTemp2 = triangle->p1.subtract(triangle->p3);
    proj = vTemp2.dotProduct(vTemp1);
    vTemp1 = vTemp1.multiply(proj);
    triangle->perp = vTemp1.subtract(vTemp2);
    triangle->perp = triangle->perp.normalizedFast();
    uDenominator = vTemp2.dotProduct(triangle->perp);
    uDenominator = -1.0 / uDenominator;
    triangle->perp = triangle->perp.multiply(uDenominator);
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
        localElement.depth = depth;
        localElement.Object = nullptr;
        intersectionPoint = ray->direction.multiply(depth);
        intersectionPoint = intersectionPoint.add(ray->position);
        localElement.point = intersectionPoint;
        localElement.Shape = reinterpret_cast<SimpleBody *>(shape);
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

    Statistics::global().rayTriangleTests++;
    if (triangle->degenerateFlag) {
        return (false);
    }

    if (ray->isPrimaryRay) {
        if (!triangle->vpCached) {
            triangle->vpNormDotOrigin =
                triangle->normalVector.dotProduct(ray->position);
            triangle->vpNormDotOrigin += triangle->distance;
            triangle->vpNormDotOrigin *= -1.0;
            triangle->vpCached = true;
        }

        normalDotDirection = triangle->normalVector.dotProduct(ray->direction);
        if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
            (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
            return (false);
        }

        *depth = triangle->vpNormDotOrigin / normalDotDirection;
    } else {
        normalDotOrigin = triangle->normalVector.dotProduct(ray->position);
        normalDotOrigin += triangle->distance;
        normalDotOrigin *= -1.0;

        normalDotDirection = triangle->normalVector.dotProduct(ray->direction);
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
        s = ray->position.y() + *depth * ray->direction.y();
        t = ray->position.z() + *depth * ray->direction.z();

        if (((triangle->p2.y() - s) * (triangle->p2.z() - triangle->p1.z())) <
            ((triangle->p2.z() - t) * (triangle->p2.y() - triangle->p1.y()))) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if (((triangle->p3.y() - s) * (triangle->p3.z() - triangle->p2.z())) <
            ((triangle->p3.z() - t) * (triangle->p3.y() - triangle->p2.y()))) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if (((triangle->p1.y() - s) * (triangle->p1.z() - triangle->p3.z())) <
            ((triangle->p1.z() - t) * (triangle->p1.y() - triangle->p3.y()))) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            Statistics::global().rayTriangleTestsSucceeded++;
            return (true);
        }
        return (false);

    case Y_AXIS:
        s = ray->position.x() + *depth * ray->direction.x();
        t = ray->position.z() + *depth * ray->direction.z();

        if ((triangle->p2.x() - s) * (triangle->p2.z() - triangle->p1.z()) <
            (triangle->p2.z() - t) * (triangle->p2.x() - triangle->p1.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if ((triangle->p3.x() - s) * (triangle->p3.z() - triangle->p2.z()) <
            (triangle->p3.z() - t) * (triangle->p3.x() - triangle->p2.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if ((triangle->p1.x() - s) * (triangle->p1.z() - triangle->p3.z()) <
            (triangle->p1.z() - t) * (triangle->p1.x() - triangle->p3.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            Statistics::global().rayTriangleTestsSucceeded++;
            return (true);
        }
        return (false);

    case Z_AXIS:
        s = ray->position.x() + *depth * ray->direction.x();
        t = ray->position.y() + *depth * ray->direction.y();

        if ((triangle->p2.x() - s) * (triangle->p2.y() - triangle->p1.y()) <
            (triangle->p2.y() - t) * (triangle->p2.x() - triangle->p1.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if ((triangle->p3.x() - s) * (triangle->p3.y() - triangle->p2.y()) <
            (triangle->p3.y() - t) * (triangle->p3.x() - triangle->p2.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if ((triangle->p1.x() - s) * (triangle->p1.y() - triangle->p3.y()) <
            (triangle->p1.y() - t) * (triangle->p1.x() - triangle->p3.x())) {
            if ((int)triangle->inverted) {
                Statistics::global().rayTriangleTestsSucceeded++;
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            Statistics::global().rayTriangleTestsSucceeded++;
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
    Triangle *newShape;

    newShape = new Triangle;
    *newShape = *this;

    return (newShape);
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
