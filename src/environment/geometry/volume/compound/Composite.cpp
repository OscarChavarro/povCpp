/**
objects.c

This module implements the methods for objects and composite objects.
*/

#include <cstdio>

#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/material/MaterialUtils.h"

inline void
Composite::Composite::linkSimpleBody(
    SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

SimpleBody *
Composite::createBasicObject()
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
    newObject->type = GeometryTypes::OBJECT_TYPE;
    newObject->methods = &Composite::basicObjectMethodTable;
    return newObject;
}

Methods Composite::compositeMethodTable = {
    Composite::allCompositeIntersections, Composite::insideCompositeObject,
    nullptr, Composite::copyCompositeObject,
    Composite::translateCompositeObject, Composite::rotateCompositeObject,
    Composite::scaleCompositeObject, Composite::invertCompositeObject};

Methods Composite::basicObjectMethodTable = {
    Composite::allObjectIntersections, Composite::insideBasicObject, nullptr,
    Composite::copyBasicObject, Composite::translateBasicObject,
    Composite::rotateBasicObject, Composite::scaleBasicObject,
    Composite::invertBasicObject};

int
Composite::allCompositeIntersections(
    SimpleBody *object, RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    Geometry *boundingShape;
    Intersection *boundingIntersection;
    Geometry *clippingShape;
    Intersection localIntersection;
    SimpleBody *localObject;
    java::PriorityQueue<Intersection> *localDepthQueue;

    for (boundingShape = ((Composite *)object)->boundingShapes;
        boundingShape != nullptr; boundingShape = boundingShape->nextObject) {

        Statistics::global().boundingRegionTests++;
        if ((boundingIntersection = GeometryOperations::intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete boundingIntersection;
        } else if (!GeometryOperations::inside(
                       &ray->position, (SimpleBody *)boundingShape)) {
            return (false);
        }
        Statistics::global().boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);
    anyIntersectionFound = false;

    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {

        GeometryOperations::allIntersections(localObject, ray, localDepthQueue);
    }

    while (localDepthQueue->size() > 0) {
        localIntersection = localDepthQueue->poll();

        intersectionFound = true;

        for (clippingShape = object->clippingShapes; clippingShape != nullptr;
            clippingShape = clippingShape->nextObject) {
            Statistics::global().clippingRegionTests++;
            if (!GeometryOperations::inside(
                    &localIntersection.Point, (SimpleBody *)clippingShape)) {
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
    PriorityQueuePool<Intersection>::pqPush(localDepthQueue);
    return (anyIntersectionFound);
}

int
Composite::allObjectIntersections(
    SimpleBody *object, RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    Intersection localIntersection;
    Intersection *boundingIntersection;
    Geometry *boundingShape;
    Geometry *clippingShape;
    java::PriorityQueue<Intersection> *localDepthQueue;

    for (boundingShape = object->boundingShapes; boundingShape != nullptr;
        boundingShape = boundingShape->nextObject) {

        Statistics::global().boundingRegionTests++;
        if ((boundingIntersection = GeometryOperations::intersect(
                 (SimpleBody *)boundingShape, ray)) != nullptr) {
            delete boundingIntersection;
        } else if (!GeometryOperations::inside(
                       &ray->position, (SimpleBody *)boundingShape)) {
            return (false);
        }
        Statistics::global().boundingRegionTestsSucceeded++;
    }

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);
    anyIntersectionFound = false;
    GeometryOperations::allIntersections(
        (SimpleBody *)object->geometry, ray, localDepthQueue);

    while (localDepthQueue->size() > 0) {
        localIntersection = localDepthQueue->poll();

        localIntersection.Object = object;
        intersectionFound = true;

        for (clippingShape = object->clippingShapes; clippingShape != nullptr;
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
                    &localIntersection.Point, (SimpleBody *)clippingShape)) {
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
    PriorityQueuePool<Intersection>::pqPush(localDepthQueue);
    return (anyIntersectionFound);
}

int
Composite::insideBasicObject(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;

    for (boundingShape = object->boundingShapes; boundingShape != nullptr;
        boundingShape = boundingShape->nextObject) {

        if (!GeometryOperations::inside(
                testPoint, (SimpleBody *)boundingShape)) {
            return (false);
        }
    }

    for (clippingShape = object->clippingShapes; clippingShape != nullptr;
        clippingShape = clippingShape->nextObject) {

        if (!GeometryOperations::inside(
                testPoint, (SimpleBody *)clippingShape)) {
            return (false);
        }
    }

    if (GeometryOperations::inside(testPoint, (SimpleBody *)object->geometry)) {
        return (true);
    }
    return (false);
}

int
Composite::insideCompositeObject(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *boundingShape;
    Geometry *clippingShape;
    SimpleBody *localObject;

    for (boundingShape = ((Composite *)object)->boundingShapes;
        boundingShape != nullptr; boundingShape = boundingShape->nextObject) {

        if (!GeometryOperations::inside(
                testPoint, (SimpleBody *)boundingShape)) {
            return (false);
        }
    }

    for (clippingShape = ((Composite *)object)->clippingShapes;
        clippingShape != nullptr; clippingShape = clippingShape->nextObject) {

        if (!GeometryOperations::inside(
                testPoint, (SimpleBody *)clippingShape)) {
            return (false);
        }
    }

    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {

        if (GeometryOperations::inside(testPoint, localObject)) {
            return (true);
        }
    }

    return (false);
}

void *
Composite::copyBasicObject(SimpleBody *object)
{
    Geometry *localShape;
    Geometry *copiedShape;
    SimpleBody *newObject;

    newObject = Composite::createBasicObject();
    *newObject = *object;
    newObject->nextObject = nullptr;
    newObject->boundingShapes = nullptr;
    newObject->clippingShapes = nullptr;
    for (localShape = object->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy((SimpleBody *)localShape);
        Composite::linkSimpleBody((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->nextObject),
            (SimpleBody **)&(newObject->boundingShapes));

    }

    for (localShape = object->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy((SimpleBody *)localShape);
        Composite::linkSimpleBody((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->nextObject),
            (SimpleBody **)&(newObject->clippingShapes));

    }

    newObject->geometry =
        (Geometry *)GeometryOperations::copy((SimpleBody *)object->geometry);

    if (newObject->objectTexture != nullptr) {
        newObject->objectTexture =
            MaterialUtils::instance().copyTexture(newObject->objectTexture);
    }

    return ((void *)newObject);
}

void *
Composite::copyCompositeObject(SimpleBody *object)
{
    Composite *newObject;
    Geometry *localShape;
    SimpleBody *localObject;
    SimpleBody *copiedObject;

    newObject = new Composite;
    *newObject = *((Composite *)object);
    newObject->nextObject = nullptr;
    newObject->simpleBodies = nullptr;
    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {

        copiedObject = (SimpleBody *)GeometryOperations::copy(localObject);
        Composite::linkSimpleBody(
            copiedObject, &(copiedObject->nextObject), &(newObject->simpleBodies));
    }

    newObject->boundingShapes = nullptr;
    for (localShape = ((Composite *)object)->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        copiedObject =
            (SimpleBody *)GeometryOperations::copy((SimpleBody *)localShape);
        Composite::linkSimpleBody(copiedObject, &(copiedObject->nextObject),
            (SimpleBody **)&(newObject->boundingShapes));
    }
    newObject->clippingShapes = nullptr;
    for (localShape = ((Composite *)object)->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        copiedObject =
            (SimpleBody *)GeometryOperations::copy((SimpleBody *)localShape);
        Composite::linkSimpleBody(copiedObject, &(copiedObject->nextObject),
            (SimpleBody **)&(newObject->clippingShapes));
    }
    return ((void *)newObject);
}

void
Composite::translateBasicObject(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = object->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }

    GeometryOperations::translate((SimpleBody *)object->geometry, vector);

    MaterialUtils::instance().translateTexture(&object->objectTexture, vector);
}

void
Composite::rotateBasicObject(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = object->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = object->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }

    GeometryOperations::rotate((SimpleBody *)object->geometry, vector);

    MaterialUtils::instance().rotateTexture(&object->objectTexture, vector);
}

void
Composite::scaleBasicObject(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = object->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }

    for (localShape = object->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }

    GeometryOperations::scale((SimpleBody *)object->geometry, vector);

    MaterialUtils::instance().scaleTexture(&object->objectTexture, vector);
}

void
Composite::translateCompositeObject(SimpleBody *object, Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {

        GeometryOperations::translate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }
}

void
Composite::rotateCompositeObject(SimpleBody *object, Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {

        GeometryOperations::rotate(localObject, vector);
    }

    for (localShape = ((Composite *)object)->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }
}

void
Composite::scaleCompositeObject(SimpleBody *object, Vector3Dd *vector)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {

        GeometryOperations::scale(localObject, vector);
    }

    for (localShape = ((Composite *)object)->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }

    for (localShape = ((Composite *)object)->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }
}

void
Composite::invertBasicObject(SimpleBody *object)
{
    Geometry *localShape;

    for (localShape = object->boundingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }

    for (localShape = object->clippingShapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }
    GeometryOperations::invert((SimpleBody *)object->geometry);
}

void
Composite::invertCompositeObject(SimpleBody *object)
{
    SimpleBody *localObject;
    Geometry *localShape;

    for (localObject = ((Composite *)object)->simpleBodies; localObject != nullptr;
        localObject = localObject->nextObject) {
        GeometryOperations::invert(localObject);
    }

    for (localShape = ((Composite *)object)->boundingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }

    for (localShape = ((Composite *)object)->clippingShapes;
        localShape != nullptr; localShape = localShape->nextObject) {
        GeometryOperations::invert((SimpleBody *)localShape);
    }
}
