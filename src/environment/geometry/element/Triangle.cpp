#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
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
    double x = java::Math::abs(triangle->normalVector.x());
    double y = java::Math::abs(triangle->normalVector.y());
    double z = java::Math::abs(triangle->normalVector.z());
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

int
Triangle::computeTriangle(Triangle *triangle)
{
    Vector3Dd v1;
    Vector3Dd v2;
    Vector3Dd temp;

    v1 = triangle->p1.subtract(triangle->p2);
    v2 = triangle->p3.subtract(triangle->p2);
    triangle->normalVector = v1.crossProduct(v2);
    const double length = triangle->normalVector.length();
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
Triangle::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    Triangle * const shape = this;
    double depth;
    Vector3Dd intersectionPoint;
    IntersectionCandidate localElement;

    if (shape->degenerateFlag) {
        return (false);
    }

    if (intersectTriangle(ray, shape, &depth)) {
        localElement.getIntersection().t = depth;
        intersectionPoint = ray->getDirection().multiply(depth);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(shape);
        localElement.getAttributes().setMaterial(materialOverride);
        depthQueue->offer(localElement);
        return (true);
    }
    return (false);
}

int
Triangle::doIntersectionForAllRayCrossingsAnnotated(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    Triangle * const shape = this;
    double depth;
    IntersectionCandidate localElement;

    if (shape->degenerateFlag) {
        return false;
    }

    if (!intersectTriangle(ray, shape, &depth)) {
        return false;
    }

    localElement.getIntersection().t = depth;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth).add(ray->getOrigin());
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(context.materialOverride);
    localElement.getAttributes().pushDetailOwner(context.detailOwner);
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context.materialUsesObjectLocalPoint);
    depthQueue->offer(localElement);
    return true;
}

int
Triangle::intersectTriangle(
    RayWithSegments *ray, Triangle *triangle, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;
    double s;
    double t;
    Statistics &stats = *ray->getStatistics();

    stats.getGeometryStatistics()->incrementRayTriangleTests();
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
        if ((normalDotDirection < Config::SMALL_TOLERANCE) &&
            (normalDotDirection > -Config::SMALL_TOLERANCE)) {
            return (false);
        }

        *depth = triangle->vpNormDotOrigin / normalDotDirection;
    } else {
        normalDotOrigin = triangle->normalVector.dotProduct(ray->getOrigin());
        normalDotOrigin += triangle->distance;
        normalDotOrigin *= -1.0;

        normalDotDirection = triangle->normalVector.dotProduct(ray->getDirection());
        if ((normalDotDirection < Config::SMALL_TOLERANCE) &&
            (normalDotDirection > -Config::SMALL_TOLERANCE)) {
            return (false);
        }

        *depth = normalDotOrigin / normalDotDirection;
    }

    if ((*depth < Config::SMALL_TOLERANCE) || (*depth > Config::MAX_DISTANCE)) {
        return (false);
    }

    switch (triangle->dominantAxis) {
    case X_AXIS:
        s = ray->getOrigin().y() + *depth * ray->getDirection().y();
        t = ray->getOrigin().z() + *depth * ray->getDirection().z();

        if (((triangle->p2.y() - s) * (triangle->p2.z() - triangle->p1.z())) <
            ((triangle->p2.z() - t) * (triangle->p2.y() - triangle->p1.y()))) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (((triangle->p3.y() - s) * (triangle->p3.z() - triangle->p2.z())) <
            ((triangle->p3.z() - t) * (triangle->p3.y() - triangle->p2.y()))) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (((triangle->p1.y() - s) * (triangle->p1.z() - triangle->p3.z())) <
            ((triangle->p1.z() - t) * (triangle->p1.y() - triangle->p3.y()))) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
            return (true);
        }
        return (false);

    case Y_AXIS:
        s = ray->getOrigin().x() + *depth * ray->getDirection().x();
        t = ray->getOrigin().z() + *depth * ray->getDirection().z();

        if ((triangle->p2.x() - s) * (triangle->p2.z() - triangle->p1.z()) <
            (triangle->p2.z() - t) * (triangle->p2.x() - triangle->p1.x())) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p3.x() - s) * (triangle->p3.z() - triangle->p2.z()) <
            (triangle->p3.z() - t) * (triangle->p3.x() - triangle->p2.x())) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p1.x() - s) * (triangle->p1.z() - triangle->p3.z()) <
            (triangle->p1.z() - t) * (triangle->p1.x() - triangle->p3.x())) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
            return (true);
        }
        return (false);

    case Z_AXIS:
        s = ray->getOrigin().x() + *depth * ray->getDirection().x();
        t = ray->getOrigin().y() + *depth * ray->getDirection().y();

        if ((triangle->p2.x() - s) * (triangle->p2.y() - triangle->p1.y()) <
            (triangle->p2.y() - t) * (triangle->p2.x() - triangle->p1.x())) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p3.x() - s) * (triangle->p3.y() - triangle->p2.y()) <
            (triangle->p3.y() - t) * (triangle->p3.x() - triangle->p2.x())) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if ((triangle->p1.x() - s) * (triangle->p1.y() - triangle->p3.y()) <
            (triangle->p1.y() - t) * (triangle->p1.x() - triangle->p3.x())) {
            if ((int)triangle->inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return (true);
            }
            return (false);
        }

        if (!(int)triangle->inverted) {
            stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
            return (true);
        }
        return (false);
    }
    return (false);
}

int
Triangle::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)point;
    (void)distanceTolerance;
    return OUTSIDE;
}

void
Triangle::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = normalVector;
}

void *
Triangle::copy()
{
    return new Triangle(*this);
}

void
Triangle::invertGeometry()
{
    this->inverted ^= true;
}

AxisAlignedBoundingBox
Triangle::getMinMax() const
{
    return AxisAlignedBoundingBox::empty()
        .expandedBy(p1)
        .expandedBy(p2)
        .expandedBy(p3);
}
