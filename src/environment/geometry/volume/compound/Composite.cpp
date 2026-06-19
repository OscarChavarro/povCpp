#include <cstdio>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "common/dataStructures/PriorityQueuePool.txx"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RendererConfiguration.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"

BoundedGeometry *
BoundedGeometry::createBasicObject(Material *objectTexture)
{
    BoundedGeometry *newObject = new BoundedGeometry(
        nullptr, objectTexture, nullptr, false);
    if (newObject == nullptr) {
        Logger::reportMessage("Composite", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }
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

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(ray->getOrigin());

        Statistics::global().incrementBoundingRegionTests();
        {
            Intersection _boundingHit;
            if (!boundingShape->intersect(ray, _boundingHit) &&
                !boundingShape->inside(&rayOrigin)) {
                return (false);
            }
        }
        Statistics::global().incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);
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
            Statistics::global().incrementClippingRegionTests();
            if (!clippingShape->inside(&localIntersection.getPoint())) {
                intersectionFound = false;
                break;
            }
            Statistics::global().incrementClippingRegionTestsSucceeded();
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

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(ray->getOrigin());

        Statistics::global().incrementBoundingRegionTests();
        {
            Intersection _boundingHit;
            if (!boundingShape->intersect(ray, _boundingHit) &&
                !boundingShape->inside(&rayOrigin)) {
                return (false);
            }
        }
        Statistics::global().incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);
    anyIntersectionFound = false;
    this->getGeometry()->allIntersections(ray, localDepthQueue);

    for (const Intersection& candidate : *localDepthQueue) {
        localIntersection = candidate;

        localIntersection.setObject(this);
        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];

            Statistics::global().incrementClippingRegionTests();
            if (RenderingConfiguration::global().hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "Test (%.4f, %.4f, %.4f)\n", localIntersection.getPoint().x(),                     localIntersection.getPoint().y(), localIntersection.getPoint().z());
                    Logger::reportMessage("Composite", Logger::WARNING, "", _logMsg);
                }
            }
            if (!clippingShape->inside(&localIntersection.getPoint())) {
                if (RenderingConfiguration::global().hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
                    Logger::reportMessage("Composite", Logger::WARNING, "", "not ok\n");
                }
                intersectionFound = false;
                break;
            }
            Statistics::global().incrementClippingRegionTestsSucceeded();
        }

        if (intersectionFound) {
            if (RenderingConfiguration::global().hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
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
    BoundedGeometry *newObject = new BoundedGeometry(
        (TransformableElement *)this->getGeometry()->copy(),
        this->getObjectTexture(),
        this->getObjectColor(),
        this->getNoShadowFlag());
    newObject->getBoundingShapes().clear();
    newObject->getClippingShapes().clear();
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        newObject->getBoundingShapes().add(copiedShape);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        newObject->getClippingShapes().add(copiedShape);
    }

    if (newObject->getObjectTexture() != nullptr) {
        newObject->setObjectTexture(newObject->getObjectTexture()->copy());
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
    newObject->getSimpleBodies().clear();
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        copiedObject = (BoundedGeometry *)localObject->copy();
        newObject->getSimpleBodies().add(copiedObject);
    }

    newObject->getBoundingShapes().clear();
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        newObject->getBoundingShapes().add(copiedShape);
    }
    newObject->getClippingShapes().clear();
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        newObject->getClippingShapes().add(copiedShape);
    }
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
