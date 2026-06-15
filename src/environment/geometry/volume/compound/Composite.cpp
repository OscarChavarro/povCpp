/**
objects.c

This module implements the methods for objects and composite objects.
*/

#include <cstdio>

#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "common/dataStructures/PriorityQueuePool.txx"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/material/MaterialUtils.h"
#include "java/util/ArrayList.txx"

SimpleBody *
SimpleBody::createBasicObject()
{
    SimpleBody *newObject;

    if ((newObject = new SimpleBody()) == nullptr) {
        Logger::reportMessage("Composite", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newObject->geometry = nullptr;
    newObject->objectTexture = MaterialUtils::instance().defaultTexture();
    newObject->objectColor = nullptr;
    newObject->noShadowFlag = false;
    newObject->geometryType = GeometryTypes::OBJECT_TYPE;
    return newObject;
}

int
Composite::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    Geometry *boundingShape;
    Intersection *boundingIntersection;
    Geometry *clippingShape;
    Intersection localIntersection;
    SimpleBody *localObject;
    java::PriorityQueue<Intersection> *localDepthQueue;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        boundingShape = this->boundingShapes[i];

        Statistics::global().boundingRegionTests++;
        if ((boundingIntersection = GeometryOperations::intersect(
                 boundingShape, ray)) != nullptr) {
            delete boundingIntersection;
        } else if (!GeometryOperations::inside(
                       &ray->position, boundingShape)) {
            return (false);
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
                    &localIntersection.Point, clippingShape)) {
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
SimpleBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    Intersection localIntersection;
    Intersection *boundingIntersection;
    Geometry *boundingShape;
    Geometry *clippingShape;
    java::PriorityQueue<Intersection> *localDepthQueue;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        boundingShape = this->boundingShapes[i];

        Statistics::global().boundingRegionTests++;
        if ((boundingIntersection = GeometryOperations::intersect(
                 boundingShape, ray)) != nullptr) {
            delete boundingIntersection;
        } else if (!GeometryOperations::inside(
                       &ray->position, boundingShape)) {
            return (false);
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
                    snprintf(_logMsg, sizeof(_logMsg), "Test (%.4f, %.4f, %.4f)\n", localIntersection.Point.x(),                     localIntersection.Point.y(), localIntersection.Point.z());
                    Logger::reportMessage("Composite", Logger::WARNING, "", _logMsg);
                }
            }
            if (!GeometryOperations::inside(
                    &localIntersection.Point, clippingShape)) {
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
SimpleBody::inside(Vector3Dd *point)
{
    Geometry *boundingShape;
    Geometry *clippingShape;

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
    Geometry *boundingShape;
    Geometry *clippingShape;
    SimpleBody *localObject;

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
SimpleBody::copy()
{
    Geometry *localShape;
    Geometry *copiedShape;
    SimpleBody *newObject;

    newObject = SimpleBody::createBasicObject();
    *newObject = *this;
    newObject->boundingShapes.clear();
    newObject->clippingShapes.clear();
    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        newObject->boundingShapes.add(copiedShape);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        newObject->clippingShapes.add(copiedShape);
    }

    newObject->geometry =
        (Geometry *)GeometryOperations::copy(this->geometry);

    if (newObject->objectTexture != nullptr) {
        newObject->objectTexture =
            MaterialUtils::instance().copyTexture(newObject->objectTexture);
    }

    return ((void *)newObject);
}

void *
Composite::copy()
{
    Composite *newObject;
    Geometry *localShape;
    Geometry *copiedShape;
    SimpleBody *localObject;
    SimpleBody *copiedObject;

    newObject = new Composite;
    *newObject = *this;
    newObject->simpleBodies.clear();
    for (long int i = this->simpleBodies.size() - 1; i >= 0; i--) {
        localObject = this->simpleBodies[i];

        copiedObject = (SimpleBody *)GeometryOperations::copy(localObject);
        newObject->simpleBodies.add(copiedObject);
    }

    newObject->boundingShapes.clear();
    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        newObject->boundingShapes.add(copiedShape);
    }
    newObject->clippingShapes.clear();
    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        newObject->clippingShapes.add(copiedShape);
    }
    return ((void *)newObject);
}

void
SimpleBody::translate(Vector3Dd *vector)
{
    Geometry *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::translate(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::translate(localShape, vector);
    }

    GeometryOperations::translate(this->geometry, vector);

    MaterialUtils::instance().translateTexture(&this->objectTexture, vector);
}

void
SimpleBody::rotate(Vector3Dd *vector)
{
    Geometry *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::rotate(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::rotate(localShape, vector);
    }

    GeometryOperations::rotate(this->geometry, vector);

    MaterialUtils::instance().rotateTexture(&this->objectTexture, vector);
}

void
SimpleBody::scale(Vector3Dd *vector)
{
    Geometry *localShape;

    for (long int i = this->boundingShapes.size() - 1; i >= 0; i--) {
        localShape = this->boundingShapes[i];

        GeometryOperations::scale(localShape, vector);
    }

    for (long int i = this->clippingShapes.size() - 1; i >= 0; i--) {
        localShape = this->clippingShapes[i];

        GeometryOperations::scale(localShape, vector);
    }

    GeometryOperations::scale(this->geometry, vector);

    MaterialUtils::instance().scaleTexture(&this->objectTexture, vector);
}

void
Composite::translate(Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

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
    SimpleBody *localObject;
    Geometry *localShape;

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
    SimpleBody *localObject;
    Geometry *localShape;

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
SimpleBody::invert()
{
    Geometry *localShape;

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
    SimpleBody *localObject;
    Geometry *localShape;

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
