#include <cstdio>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "common/dataStructures/PriorityQueuePool.txx"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RendererConfiguration.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"

BoundedGeometry *
BoundedGeometry::createBasicObject(Material *objectTexture)
{
    BoundedGeometry *newObject;

    if ((newObject = new BoundedGeometry()) == nullptr) {
        Logger::reportMessage("Composite", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newObject->geometry = nullptr;
    newObject->objectTexture = objectTexture;
    newObject->objectColor = nullptr;
    newObject->noShadowFlag = false;
    return newObject;
}

int
Composite::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    TransformableElement *boundingShape;
    TransformableElement *clippingShape;
    Intersection localIntersection;
    BoundedGeometry *localObject;
    java::PriorityQueue<Intersection> *localDepthQueue;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        boundingShape = this->boundingShapes[i];

        Statistics::global().boundingRegionTests++;
        {
            Intersection _boundingHit;
            if (!GeometryOperations::intersect(boundingShape, ray, _boundingHit) &&
                !GeometryOperations::inside(&ray->origin, boundingShape)) {
                return (false);
            }
        }
        Statistics::global().boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);
    anyIntersectionFound = false;

    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        GeometryOperations::allIntersections(localObject, ray, localDepthQueue);
    }

    for (const Intersection& candidate : *localDepthQueue) {
        localIntersection = candidate;

        intersectionFound = true;

        for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
            clippingShape = this->clippingShapes[i];
            Statistics::global().clippingRegionTests++;
            if (!GeometryOperations::inside(
                    &localIntersection.point, clippingShape)) {
                intersectionFound = false;
                break;
            }
            Statistics::global().clippingRegionTestsSucceeded++;
        }

        if (intersectionFound) {
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    PriorityQueuePool<Intersection>::pqPush(localDepthQueue);
    return (anyIntersectionFound);
}

int
BoundedGeometry::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    Intersection localIntersection;
    TransformableElement *boundingShape;
    TransformableElement *clippingShape;
    java::PriorityQueue<Intersection> *localDepthQueue;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        boundingShape = this->boundingShapes[i];

        Statistics::global().boundingRegionTests++;
        {
            Intersection _boundingHit;
            if (!GeometryOperations::intersect(boundingShape, ray, _boundingHit) &&
                !GeometryOperations::inside(&ray->origin, boundingShape)) {
                return (false);
            }
        }
        Statistics::global().boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);
    anyIntersectionFound = false;
    GeometryOperations::allIntersections(
        this->geometry, ray, localDepthQueue);

    for (const Intersection& candidate : *localDepthQueue) {
        localIntersection = candidate;

        localIntersection.Object = this;
        intersectionFound = true;

        for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
            clippingShape = this->clippingShapes[i];

            Statistics::global().clippingRegionTests++;
            if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Test (%.4f, %.4f, %.4f)\n", localIntersection.point.x(),                     localIntersection.point.y(), localIntersection.point.z());
                    Logger::reportMessage("Composite", Logger::WARNING, "", _logMsg);
                }
            }
            if (!GeometryOperations::inside(
                    &localIntersection.point, clippingShape)) {
                if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
                    Logger::reportMessage("Composite", Logger::WARNING, "", "not ok\n");
                }
                intersectionFound = false;
                break;
            }
            Statistics::global().clippingRegionTestsSucceeded++;
        }

        if (intersectionFound) {
            if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
                Logger::reportMessage("Composite", Logger::WARNING, "", "ok\n");
            }
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    PriorityQueuePool<Intersection>::pqPush(localDepthQueue);
    return (anyIntersectionFound);
}

int
BoundedGeometry::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    (void)owner;
    return allIntersections(ray, depthQueue);
}

int
BoundedGeometry::inside(Vector3Dd *point)
{
    TransformableElement *boundingShape;
    TransformableElement *clippingShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        boundingShape = this->boundingShapes[i];

        if (!GeometryOperations::inside(
                point, boundingShape)) {
            return (false);
        }
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        clippingShape = this->clippingShapes[i];

        if (!GeometryOperations::inside(
                point, clippingShape)) {
            return (false);
        }
    }

    if (GeometryOperations::inside(point, this->geometry)) {
        return (true);
    }
    return (false);
}

int
Composite::inside(Vector3Dd *point)
{
    TransformableElement *boundingShape;
    TransformableElement *clippingShape;
    BoundedGeometry *localObject;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        boundingShape = this->boundingShapes[i];

        if (!GeometryOperations::inside(
                point, boundingShape)) {
            return (false);
        }
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        clippingShape = this->clippingShapes[i];

        if (!GeometryOperations::inside(
                point, clippingShape)) {
            return (false);
        }
    }

    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        if (GeometryOperations::inside(point, localObject)) {
            return (true);
        }
    }

    return (false);
}

void *
BoundedGeometry::copy()
{
    TransformableElement *localShape;
    TransformableElement *copiedShape;
    BoundedGeometry *newObject;

    newObject = BoundedGeometry::createBasicObject(this->objectTexture);
    *newObject = *this;
    newObject->boundingShapes.clear();
    newObject->clippingShapes.clear();
    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        copiedShape =
            (TransformableElement *)GeometryOperations::copy(localShape);
        newObject->boundingShapes.add(copiedShape);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        copiedShape =
            (TransformableElement *)GeometryOperations::copy(localShape);
        newObject->clippingShapes.add(copiedShape);
    }

    newObject->geometry =
        (TransformableElement *)GeometryOperations::copy(this->geometry);

    if (newObject->objectTexture != nullptr) {
        newObject->objectTexture = newObject->objectTexture->copy();
    }

    return ((void *)newObject);
}

void *
Composite::copy()
{
    Composite *newObject;
    TransformableElement *localShape;
    TransformableElement *copiedShape;
    BoundedGeometry *localObject;
    BoundedGeometry *copiedObject;

    newObject = new Composite;
    *newObject = *this;
    newObject->simpleBodies.clear();
    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        copiedObject = (BoundedGeometry *)GeometryOperations::copy(localObject);
        newObject->simpleBodies.add(copiedObject);
    }

    newObject->boundingShapes.clear();
    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        copiedShape =
            (TransformableElement *)GeometryOperations::copy(localShape);
        newObject->boundingShapes.add(copiedShape);
    }
    newObject->clippingShapes.clear();
    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        copiedShape =
            (TransformableElement *)GeometryOperations::copy(localShape);
        newObject->clippingShapes.add(copiedShape);
    }
    return ((void *)newObject);
}

void
BoundedGeometry::translate(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::translate(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::translate(localShape, vector);
    }

    GeometryOperations::translate(this->geometry, vector);

    if (this->objectTexture != nullptr) {
        this->objectTexture->translate(vector);
    }
}

void
BoundedGeometry::rotate(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::rotate(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::rotate(localShape, vector);
    }

    GeometryOperations::rotate(this->geometry, vector);

    if (this->objectTexture != nullptr) {
        this->objectTexture->rotate(vector);
    }
}

void
BoundedGeometry::scale(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::scale(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::scale(localShape, vector);
    }

    GeometryOperations::scale(this->geometry, vector);

    if (this->objectTexture != nullptr) {
        this->objectTexture->scale(vector);
    }
}

void
Composite::translate(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        GeometryOperations::translate(localObject, vector);
    }

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::translate(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::translate(localShape, vector);
    }
}

void
Composite::rotate(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        GeometryOperations::rotate(localObject, vector);
    }

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::rotate(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::rotate(localShape, vector);
    }
}

void
Composite::scale(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        GeometryOperations::scale(localObject, vector);
    }

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::scale(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::scale(localShape, vector);
    }
}

void
BoundedGeometry::invert()
{
    TransformableElement *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];
        GeometryOperations::invert(localShape);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];
        GeometryOperations::invert(localShape);
    }
    GeometryOperations::invert(this->geometry);
}

void
Composite::invert()
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];
        GeometryOperations::invert(localObject);
    }

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];
        GeometryOperations::invert(localShape);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];
        GeometryOperations::invert(localShape);
    }
}
