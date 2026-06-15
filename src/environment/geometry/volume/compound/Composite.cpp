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

inline void
SimpleBody::linkSimpleBody(
    Geometry *newObject, Geometry **field, Geometry **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

inline void
SimpleBody::linkSimpleBody(
    SimpleBody *newObject, Geometry **field, SimpleBody **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

SimpleBody *
SimpleBody::createBasicObject()
{
    SimpleBody *newObject;

    if ((newObject = new SimpleBody()) == nullptr) {
        Logger::reportMessage("Composite", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newObject->nextObject = nullptr;
    newObject->geometry = nullptr;
    newObject->boundingShapes = nullptr;
    newObject->clippingShapes = nullptr;
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

    for (boundingShape = this->boundingShapes;
        boundingShape != nullptr; boundingShape = boundingShape->nextObject) {

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

    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {

        GeometryOperations::allIntersections(localObject, ray, localDepthQueue);
    }

    for (const Intersection& candidate : *localDepthQueue) {
        localIntersection = candidate;

        intersectionFound = true;

        for (clippingShape = this->clippingShapes; clippingShape != nullptr;
            clippingShape = clippingShape->nextObject) {
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

    for (boundingShape = this->boundingShapes; boundingShape != nullptr;
        boundingShape = boundingShape->nextObject) {

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

        for (clippingShape = this->clippingShapes; clippingShape != nullptr;
            clippingShape = clippingShape->nextObject) {

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

    for (boundingShape = this->boundingShapes; boundingShape != nullptr;
        boundingShape = boundingShape->nextObject) {

        if (!GeometryOperations::inside(
                point, boundingShape)) {
            return (false);
        }
    }

    for (clippingShape = this->clippingShapes; clippingShape != nullptr;
        clippingShape = clippingShape->nextObject) {

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

    for (boundingShape = this->boundingShapes;
        boundingShape != nullptr; boundingShape = boundingShape->nextObject) {

        if (!GeometryOperations::inside(
                point, boundingShape)) {
            return (false);
        }
    }

    for (clippingShape = this->clippingShapes;
        clippingShape != nullptr; clippingShape = clippingShape->nextObject) {

        if (!GeometryOperations::inside(
                point, clippingShape)) {
            return (false);
        }
    }

    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {

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
    newObject->nextObject = nullptr;
    newObject->boundingShapes = nullptr;
    newObject->clippingShapes = nullptr;
    for (localShape = this->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        SimpleBody::linkSimpleBody(copiedShape,
            &(copiedShape->nextObject),
            &(newObject->boundingShapes));

    }

    for (localShape = this->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        SimpleBody::linkSimpleBody(copiedShape,
            &(copiedShape->nextObject),
            &(newObject->clippingShapes));

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
    newObject->nextObject = nullptr;
    newObject->simpleBodies = nullptr;
    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {

        copiedObject = (SimpleBody *)GeometryOperations::copy(localObject);
        SimpleBody::linkSimpleBody(
            copiedObject, &(copiedObject->nextObject), &(newObject->simpleBodies));
    }

    newObject->boundingShapes = nullptr;
    for (localShape = this->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        SimpleBody::linkSimpleBody(copiedShape, &(copiedShape->nextObject),
            &(newObject->boundingShapes));
    }
    newObject->clippingShapes = nullptr;
    for (localShape = this->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        SimpleBody::linkSimpleBody(copiedShape, &(copiedShape->nextObject),
            &(newObject->clippingShapes));
    }
    return ((void *)newObject);
}

void
SimpleBody::translate(Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = this->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::translate(localShape, vector);
    }

    for (localShape = this->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::translate(localShape, vector);
    }

    GeometryOperations::translate(this->geometry, vector);

    MaterialUtils::instance().translateTexture(&this->objectTexture, vector);
}

void
SimpleBody::rotate(Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = this->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::rotate(localShape, vector);
    }

    for (localShape = this->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::rotate(localShape, vector);
    }

    GeometryOperations::rotate(this->geometry, vector);

    MaterialUtils::instance().rotateTexture(&this->objectTexture, vector);
}

void
SimpleBody::scale(Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = this->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::scale(localShape, vector);
    }

    for (localShape = this->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

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

    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {

        GeometryOperations::translate(localObject, vector);
    }

    for (localShape = this->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::translate(localShape, vector);
    }

    for (localShape = this->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::translate(localShape, vector);
    }
}

void
Composite::rotate(Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {

        GeometryOperations::rotate(localObject, vector);
    }

    for (localShape = this->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::rotate(localShape, vector);
    }

    for (localShape = this->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::rotate(localShape, vector);
    }
}

void
Composite::scale(Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {

        GeometryOperations::scale(localObject, vector);
    }

    for (localShape = this->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::scale(localShape, vector);
    }

    for (localShape = this->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::scale(localShape, vector);
    }
}

void
SimpleBody::invert()
{
    Geometry *localShape;

    for (localShape = this->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        GeometryOperations::invert(localShape);
    }

    for (localShape = this->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        GeometryOperations::invert(localShape);
    }
    GeometryOperations::invert(this->geometry);
}

void
Composite::invert()
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = this->simpleBodies; localObject != nullptr;
        localObject = static_cast<SimpleBody *>(localObject->nextObject)) {
        GeometryOperations::invert(localObject);
    }

    for (localShape = this->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {
        GeometryOperations::invert(localShape);
    }

    for (localShape = this->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {
        GeometryOperations::invert(localShape);
    }
}
