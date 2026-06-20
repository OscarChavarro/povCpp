#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/surface/InfinitePlane.h"

InfinitePlane::InfinitePlane() :
    InfinitePlane(Vector3Dd(0.0, 1.0, 0.0), 0.0)
{
}

InfinitePlane::InfinitePlane(const Vector3Dd &normalVector, double distance) :
    InfinitePlane(normalVector, distance, 0.0, false)
{
}

InfinitePlane::InfinitePlane(const Vector3Dd &normalVector, double distance,
    double vpNormDotOrigin, bool vpCached) :
    normalVector(normalVector),
    distance(distance),
    vpNormDotOrigin(vpNormDotOrigin),
    vpCached(vpCached)
{
}

int
InfinitePlane::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return allIntersectionsForOwner(
        ray, depthQueue, reinterpret_cast<SimpleBody *>(this));
}

int
InfinitePlane::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    InfinitePlane * const shape = this;
    double depth;
    Vector3Dd intersectionPoint;
    Intersection localElement;

    if (InfinitePlane::intersectPlane(ray, shape, &depth)) {
        if (depth > GeometryConstants::Small_Tolerance) {
            localElement.setT(depth);
            localElement.setBoundedGeometry(nullptr);
            intersectionPoint = ray->getDirection().multiply(depth);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.setPoint(intersectionPoint);
            localElement.setOwnerSimpleBody(owner);
            depthQueue->offer(localElement);
            return (true);
        }
    }

    return (false);
}

int
InfinitePlane::intersectPlane(
    RayWithSegments *ray, InfinitePlane *plane, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;
    Statistics &stats = *ray->getStatistics();

    stats.incrementRayPlaneTests();
    if (ray->isPrimaryRayEnabled()) {
        if (!plane->vpCached) {
            plane->vpNormDotOrigin =
                plane->normalVector.dotProduct(ray->getOrigin());
            plane->vpNormDotOrigin += plane->distance;
            plane->vpNormDotOrigin *= -1.0;
            plane->vpCached = true;
        }

        normalDotDirection = plane->normalVector.dotProduct(ray->getDirection());
        if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
            (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
            return (false);
        }

        *depth = plane->vpNormDotOrigin / normalDotDirection;
        if ((*depth >= GeometryConstants::Small_Tolerance) && (*depth <= GeometryConstants::Max_Distance)) {
            stats.incrementRayPlaneTestsSucceeded();
            return (true);
        }
        return (false);
    }
    normalDotOrigin = plane->normalVector.dotProduct(ray->getOrigin());
    normalDotOrigin += plane->distance;
    normalDotOrigin *= -1.0;

    normalDotDirection = plane->normalVector.dotProduct(ray->getDirection());
    if ((normalDotDirection < GeometryConstants::Small_Tolerance) &&
        (normalDotDirection > -GeometryConstants::Small_Tolerance)) {
        return (false);
    }

    *depth = normalDotOrigin / normalDotDirection;
    if ((*depth >= GeometryConstants::Small_Tolerance) && (*depth <= GeometryConstants::Max_Distance)) {
        stats.incrementRayPlaneTestsSucceeded();
        return (true);
    }
    return (false);
}

int
InfinitePlane::inside(Vector3Dd *testPoint)
{
    const InfinitePlane *plane = this;

    double temp = (*testPoint).dotProduct(plane->normalVector);
    return ((temp + plane->distance) <= GeometryConstants::Small_Tolerance);
}

void
InfinitePlane::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = normalVector;
}

void *
InfinitePlane::copy()
{
    return new InfinitePlane(*this);
}

void
InfinitePlane::translateGeometry(Vector3Dd *vector)
{
    InfinitePlane * const plane = this;

    Vector3Dd translation = plane->normalVector.multiply(*vector);
    plane->distance -= translation.x() + translation.y() + translation.z();
}

void
InfinitePlane::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->normalVector = transformation.transpose().multiply(this->normalVector);
}

void
InfinitePlane::scaleGeometry(Vector3Dd *vector)
{
    InfinitePlane * const plane = this;

    plane->normalVector = Vector3Dd(
        plane->normalVector.x() / vector->x(),
        plane->normalVector.y() / vector->y(),
        plane->normalVector.z() / vector->z());

    const double length = plane->normalVector.length();
    plane->normalVector = plane->normalVector.multiply(1.0 / length);
    plane->distance /= length;
}

void
InfinitePlane::invertGeometry()
{
    InfinitePlane * const plane = this;

    plane->normalVector = plane->normalVector.multiply(-1.0);
    plane->distance *= -1.0;
}
