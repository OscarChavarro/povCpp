#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Box.h"

Box::Box() :
    Box(Vector3Dd(-1.0, -1.0, -1.0), Vector3Dd(1.0, 1.0, 1.0), false)
{
}

Box::Box(const Vector3Dd &minBounds, const Vector3Dd &maxBounds, bool inverted) :
    bounds{minBounds, maxBounds},
    inverted(inverted)
{
}

Box::Box(const Box &other) :
    Box(other.bounds[0], other.bounds[1], other.inverted)
{
}

Box::~Box()
{
}


int
Box::closeTo(double x, double y)
{
    return java::Math::abs(x - y) < Config::INTERSECTION_EPSILON ? 1 : 0;
}

int
Box::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    IntersectionCandidate localElement;
    bool intersectionFound = false;
    Box * const shape = this;
    if (Box::intersectBoxx(ray, shape, &depth1, &depth2)) {
        localElement.getIntersection().t = depth1;
        intersectionPoint = ray->getDirection().multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(shape);
        localElement.getAttributes().setMaterial(materialOverride);
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.getIntersection().t = depth2;
            intersectionPoint = ray->getDirection().multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitGeometry(shape);
            localElement.getAttributes().setMaterial(materialOverride);
            depthQueue->offer(localElement);
            intersectionFound = true;
        }
    }
    return (intersectionFound);
}

int
Box::doIntersectionForAllRayCrossingsAnnotated(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    double depth1;
    double depth2;
    Box * const shape = this;
    if (!Box::intersectBoxx(ray, shape, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(context.materialOverride);
    localElement.getAttributes().pushDetailOwner(context.detailOwner);
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context.materialUsesObjectLocalPoint);

    localElement.getIntersection().t = depth1;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth1).add(ray->getOrigin());
    depthQueue->offer(localElement);

    if (depth2 != depth1) {
        localElement.getIntersection().t = depth2;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth2).add(ray->getOrigin());
        depthQueue->offer(localElement);
    }

    return true;
}

bool
Box::doIntersectionFirstHitNoQueue(
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    Box * const shape = this;
    if (!Box::intersectBoxx(ray, shape, &depth1, &depth2)) {
        return false;
    }

    const double nearestDepth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = nearestDepth;
    out.getIntersection().point =
        ray->getDirection().multiply(nearestDepth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(shape);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
Box::intersectBoxx(
    const RayWithSegments *ray, const Box *box, double *depth1, double *depth2)
{
    double t;
    double tmin;
    double tmax;
    Vector3Dd p;
    Vector3Dd d;
    Statistics &stats = *ray->getStatistics();

    stats.incrementRayBoxTests();

    p = Vector3Dd(ray->getOrigin().x(), ray->getOrigin().y(), ray->getOrigin().z());
    d = Vector3Dd(ray->getDirection().x(), ray->getDirection().y(), ray->getDirection().z());

    tmin = 0.0;
    tmax = HUGE_VAL;

    // Sides first
    if (d.x() < -Config::INTERSECTION_EPSILON) {
        t = (box->bounds[0].x() - p.x()) / d.x();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].x() - p.x()) / d.x();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.x() > Config::INTERSECTION_EPSILON) {
        t = (box->bounds[1].x() - p.x()) / d.x();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].x() - p.x()) / d.x();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.x() < box->bounds[0].x() || p.x() > box->bounds[1].x()) {
        return 0;
    }

    // Check Top/Bottom
    if (d.y() < -Config::INTERSECTION_EPSILON) {
        t = (box->bounds[0].y() - p.y()) / d.y();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].y() - p.y()) / d.y();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.y() > Config::INTERSECTION_EPSILON) {
        t = (box->bounds[1].y() - p.y()) / d.y();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].y() - p.y()) / d.y();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.y() < box->bounds[0].y() || p.y() > box->bounds[1].y()) {
        return 0;
    }

    // Now front/back
    if (d.z() < -Config::INTERSECTION_EPSILON) {
        t = (box->bounds[0].z() - p.z()) / d.z();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[1].z() - p.z()) / d.z();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (d.z() > Config::INTERSECTION_EPSILON) {
        t = (box->bounds[1].z() - p.z()) / d.z();
        if (t < tmin) {
            return 0;
        }
        if (t <= tmax) {
            tmax = t;
        }
        t = (box->bounds[0].z() - p.z()) / d.z();
        if (t >= tmin) {
            if (t > tmax) {
                return 0;
            }
            tmin = t;
        }
    } else if (p.z() < box->bounds[0].z() || p.z() > box->bounds[1].z()) {
        return 0;
    }

    *depth1 = tmin;
    *depth2 = tmax;

    if ((*depth1 < Config::SMALL_TOLERANCE) || (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
            return (false);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayBoxTestsSucceeded();
    return (true);
}

int
Box::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    (void)distanceTolerance;
    Vector3Dd newPoint;
    const Box *box = this;

    newPoint = testPoint;

    // Test to see if we are inside the box
    if (newPoint.x() < box->bounds[0].x() || newPoint.x() > box->bounds[1].x()) {
        return box->inverted ? INSIDE : OUTSIDE;
    }
    if (newPoint.y() < box->bounds[0].y() || newPoint.y() > box->bounds[1].y()) {
        return box->inverted ? INSIDE : OUTSIDE;
    }
    if (newPoint.z() < box->bounds[0].z() || newPoint.z() > box->bounds[1].z()) {
        return box->inverted ? INSIDE : OUTSIDE;
    }
    // Inside the box
    return box->inverted ? OUTSIDE : INSIDE;
}

void
Box::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    Vector3Dd newPoint;
    const Box *box = this;

    newPoint = Vector3Dd(
        intersectionPoint->x(), intersectionPoint->y(), intersectionPoint->z());

    *result = Vector3Dd(0.0, 0.0, 0.0);
    if (Box::closeTo(newPoint.x(), box->bounds[1].x())) {
        *result = result->withX(1.0);
    } else if (Box::closeTo(newPoint.x(), box->bounds[0].x())) {
        *result = result->withX(-1.0);
    } else if (Box::closeTo(newPoint.y(), box->bounds[1].y())) {
        *result = result->withY(1.0);
    } else if (Box::closeTo(newPoint.y(), box->bounds[0].y())) {
        *result = result->withY(-1.0);
    } else if (Box::closeTo(newPoint.z(), box->bounds[1].z())) {
        *result = result->withZ(1.0);
    } else if (closeTo(newPoint.z(), box->bounds[0].z())) {
        *result = result->withZ(-1.0);
    } else {
        // Bad result, should we do something with it?
        *result = result->withX(1.0);
    }
}

void *
Box::copy()
{
    return new Box(*this);
}

void
Box::invertGeometry()
{
    this->inverted = !this->inverted;
}

AxisAlignedBoundingBox
Box::getMinMax() const
{
    return AxisAlignedBoundingBox{bounds[0], bounds[1]};
}
