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
    Box(nullptr, nullptr, minBounds, maxBounds, inverted)
{
}

Box::Box(Matrix4x4d *transformation, Matrix4x4d *transformationInverse,
    const Vector3Dd &minBounds, const Vector3Dd &maxBounds, bool inverted) :
    transformation(nullptr),
    transformationInverse(nullptr),
    bounds{minBounds, maxBounds},
    inverted(inverted)
{
    if (transformation != nullptr) {
        this->transformation = new Matrix4x4d(*transformation);
        this->transformationInverse = new Matrix4x4d(*transformationInverse);
    }
}

Box::Box(const Box &other) :
    Box(other.transformation, other.transformationInverse, other.bounds[0],
        other.bounds[1], other.inverted)
{
}

Box::~Box()
{
    delete transformation;
    delete transformationInverse;
}


int
Box::closeTo(double x, double y)
{
    return java::Math::abs(x - y) < Config::INTERSECTION_EPSILON ? 1 : 0;
}

int
Box::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    return allIntersectionsForMaterial(ray, depthQueue, nullptr);
}

int
Box::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *material)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    IntersectionCandidate localElement;
    bool intersectionFound = false;
    Box * const shape = this;
    if (Box::intersectBoxx(ray, shape, &depth1, &depth2)) {
        localElement.getIntersection().setT(depth1);
        intersectionPoint = ray->getDirection().multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.getIntersection().setPoint(intersectionPoint);
        localElement.getAttributes().setHitGeometry(shape);
        localElement.getAttributes().setMaterial(material);
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.getIntersection().setT(depth2);
            intersectionPoint = ray->getDirection().multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().setPoint(intersectionPoint);
            localElement.getAttributes().setHitGeometry(shape);
            localElement.getAttributes().setMaterial(material);
            depthQueue->offer(localElement);
            intersectionFound = true;
        }
    }
    return (intersectionFound);
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

    // Transform the point into the boxes space
    if (box->transformation != nullptr) {
        p = box->transformationInverse->transformPoint(ray->getOrigin());
        d = box->transformationInverse->transformDirection(ray->getDirection());
    } else {
        p = Vector3Dd(ray->getOrigin().x(), ray->getOrigin().y(), ray->getOrigin().z());
        d = Vector3Dd(ray->getDirection().x(), ray->getDirection().y(), ray->getDirection().z());
    }

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

    // Logger::info("Box intersects: %g, %g\n", *Depth1, *Depth2);
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
Box::inside(Vector3Dd *testPoint)
{
    Vector3Dd newPoint;
    const Box *box = this;

    // Transform the point into the boxes space
    if (box->transformation != nullptr) {
        newPoint = box->transformationInverse->transformPoint(*testPoint);
    } else {
        newPoint = *testPoint;
    }

    // Test to see if we are inside the box
    if (newPoint.x() < box->bounds[0].x() || newPoint.x() > box->bounds[1].x()) {
        return ((int)box->inverted);
    }
    if (newPoint.y() < box->bounds[0].y() || newPoint.y() > box->bounds[1].y()) {
        return ((int)box->inverted);
    }
    if (newPoint.z() < box->bounds[0].z() || newPoint.z() > box->bounds[1].z()) {
        return ((int)box->inverted);
    }
    // Inside the box
    return 1 - box->inverted;
}

void
Box::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    Vector3Dd newPoint;
    const Box *box = this;

    // Transform the point into the boxes space
    if (box->transformation != nullptr) {
        newPoint = box->transformationInverse->transformPoint(*intersectionPoint);
    } else {
        newPoint = Vector3Dd(
            intersectionPoint->x(), intersectionPoint->y(), intersectionPoint->z());
    }

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

    // Transform the point into the boxes space
    if (box->transformation != nullptr) {
        *result = box->transformationInverse->withoutTranslation().multiply(*result);
        *result = (*result).normalizedFast();
    }
}

void *
Box::copy()
{
    return new Box(*this);
}

void
Box::translateGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Box * const box = this;
    if (box->transformation == nullptr) {
        box->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        box->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *box->transformation = box->transformation->multiply(deltaTransformation);
    *box->transformationInverse =
        deltaTransformationInverse.multiply(*box->transformationInverse);
}

void
Box::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Box * const box = this;
    if (box->transformation == nullptr) {
        box->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        box->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *box->transformation = box->transformation->multiply(deltaTransformation);
    *box->transformationInverse =
        deltaTransformationInverse.multiply(*box->transformationInverse);
}

void
Box::scaleGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    Box * const box = this;
    if (box->transformation == nullptr) {
        box->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        box->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *box->transformation = box->transformation->multiply(deltaTransformation);
    *box->transformationInverse =
        deltaTransformationInverse.multiply(*box->transformationInverse);
}

void
Box::invertGeometry()
{
    this->inverted = !this->inverted;
}
