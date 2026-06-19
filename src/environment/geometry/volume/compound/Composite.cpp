#include <cstdio>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RendererConfiguration.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"

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
    Statistics &stats = *ray->getStatistics();

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(ray->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            Intersection _boundingHit;
            if (!boundingShape->intersect(ray, _boundingHit) &&
                !boundingShape->inside(&rayOrigin)) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->allIntersections(ray, localDepthQueue);
    }

    for (const Intersection& candidate : *localDepthQueue) {
        localIntersection = candidate;

        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];
            stats.incrementClippingRegionTests();
            if (!clippingShape->inside(&localIntersection.getPoint())) {
                intersectionFound = false;
                break;
            }
            stats.incrementClippingRegionTestsSucceeded();
        }

        if (intersectionFound) {
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    ray->getIntersectionQueuePool()->push(localDepthQueue);
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
    Statistics &stats = *ray->getStatistics();

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(ray->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            Intersection _boundingHit;
            if (!boundingShape->intersect(ray, _boundingHit) &&
                !boundingShape->inside(&rayOrigin)) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;
    this->getGeometry()->allIntersections(ray, localDepthQueue);

    for (const Intersection& candidate : *localDepthQueue) {
        localIntersection = candidate;

        localIntersection.setObject(this);
        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];

            stats.incrementClippingRegionTests();
            if (ray->getConfig()->hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Test (%.4f, %.4f, %.4f)\n", localIntersection.getPoint().x(),                     localIntersection.getPoint().y(), localIntersection.getPoint().z());
                    Logger::reportMessage("Composite", Logger::WARNING, "", _logMsg);
                }
            }
            if (!clippingShape->inside(&localIntersection.getPoint())) {
                if (ray->getConfig()->hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
                    Logger::reportMessage("Composite", Logger::WARNING, "", "not ok\n");
                }
                intersectionFound = false;
                break;
            }
            stats.incrementClippingRegionTestsSucceeded();
        }

        if (intersectionFound) {
            if (ray->getConfig()->hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
                Logger::reportMessage("Composite", Logger::WARNING, "", "ok\n");
            }
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    ray->getIntersectionQueuePool()->push(localDepthQueue);
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

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (!boundingShape->inside(point)) {
            return (false);
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (!clippingShape->inside(point)) {
            return (false);
        }
    }

    if (this->getGeometry()->inside(point)) {
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

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (!boundingShape->inside(point)) {
            return (false);
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (!clippingShape->inside(point)) {
            return (false);
        }
    }

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        if (localObject->inside(point)) {
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
    Material *copiedTexture =
        this->getObjectTexture() != nullptr ? this->getObjectTexture()->copy() : nullptr;
    java::ArrayList<TransformableElement*> copiedBoundingShapes(4);
    java::ArrayList<TransformableElement*> copiedClippingShapes(4);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        copiedBoundingShapes.add(copiedShape);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        copiedClippingShapes.add(copiedShape);
    }
    BoundedGeometry *newObject = new BoundedGeometry(
        (TransformableElement *)this->getGeometry()->copy(),
        copiedTexture,
        this->getObjectColor(),
        this->getNoShadowFlag(),
        copiedBoundingShapes,
        copiedClippingShapes);

    return ((void *)newObject);
}

void *
Composite::copy()
{
    TransformableElement *localShape;
    TransformableElement *copiedShape;
    BoundedGeometry *localObject;
    BoundedGeometry *copiedObject;
    Material *copiedTexture =
        this->getObjectTexture() != nullptr ? this->getObjectTexture()->copy() : nullptr;
    java::ArrayList<BoundedGeometry*> copiedSimpleBodies(4);
    java::ArrayList<TransformableElement*> copiedBoundingShapes(4);
    java::ArrayList<TransformableElement*> copiedClippingShapes(4);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        copiedObject = (BoundedGeometry *)localObject->copy();
        copiedSimpleBodies.add(copiedObject);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        copiedBoundingShapes.add(copiedShape);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        copiedClippingShapes.add(copiedShape);
    }
    Composite *newObject = new Composite(
        this->getGeometry() != nullptr ?
            (TransformableElement *)this->getGeometry()->copy() : nullptr,
        copiedTexture,
        this->getObjectColor(),
        this->getNoShadowFlag(),
        copiedBoundingShapes,
        copiedClippingShapes,
        copiedSimpleBodies);
    return ((void *)newObject);
}

void
BoundedGeometry::translate(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->translate(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->translate(vector);
    }

    this->getGeometry()->translate(vector);

    if (this->getObjectTexture() != nullptr) {
        this->getObjectTexture()->translate(vector);
    }
}

void
BoundedGeometry::rotate(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->rotate(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->rotate(vector);
    }

    this->getGeometry()->rotate(vector);

    if (this->getObjectTexture() != nullptr) {
        this->getObjectTexture()->rotate(vector);
    }
}

void
BoundedGeometry::scale(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->scale(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->scale(vector);
    }

    this->getGeometry()->scale(vector);

    if (this->getObjectTexture() != nullptr) {
        this->getObjectTexture()->scale(vector);
    }
}

void
Composite::translate(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->translate(vector);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->translate(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->translate(vector);
    }
}

void
Composite::rotate(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->rotate(vector);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->rotate(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->rotate(vector);
    }
}

void
Composite::scale(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->scale(vector);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->scale(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->scale(vector);
    }
}

void
BoundedGeometry::invert()
{
    TransformableElement *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invert();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invert();
    }
    this->getGeometry()->invert();
}

void
Composite::invert()
{
    BoundedGeometry *localObject;
    TransformableElement *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];
        localObject->invert();
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invert();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invert();
    }
}
