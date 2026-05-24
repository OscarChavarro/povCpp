/****************************************************************************
 *                         csg.c
 *
 *  This module implements routines for constructive solid geometry.
 *
 *****************************************************************************/

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
Methods CSG::unionMethodTable = {Composite::objectIntersect,
    CSG::allCsgUnionIntersections, CSG::insideCsgUnion, nullptr, CSG::copyCsg,
    CSG::translateCsg, CSG::rotateCsg, CSG::scaleCsg, CSG::invertCsg};

Methods CSG::intersectionMethodTable = {Composite::objectIntersect,
    CSG::allCsgIntersectIntersections, CSG::insideCsgIntersection, nullptr,
    CSG::copyCsg, CSG::translateCsg, CSG::rotateCsg, CSG::scaleCsg,
    CSG::invertCsg};

static inline void
linkShapeNode(
    SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

int
CSG::allCsgUnionIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    intersectionFound = LegacyBoolean::FALSE_VALUE;
    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        if (GeometryOperations::allIntersections(
                (SimpleBody *)localShape, ray, depthQueue)) {
            intersectionFound = LegacyBoolean::TRUE_VALUE;
        }
    }

    return (intersectionFound);
}

int
CSG::allCsgIntersectIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    int anyIntersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;
    Geometry *shape2;
    PriorityQueueNode *localDepthQueue;
    Intersection *localIntersection;

    localDepthQueue = IntersectionPriorityQueuePool::pqPop(128);

    anyIntersectionFound = LegacyBoolean::FALSE_VALUE;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::allIntersections(
            (SimpleBody *)localShape, ray, localDepthQueue);

        for (localIntersection = localDepthQueue->getHighest();
            localIntersection != nullptr; localDepthQueue->deleteHighest(),
            localIntersection = localDepthQueue->getHighest()) {

            intersectionFound = LegacyBoolean::TRUE_VALUE;

            for (shape2 = shape->Shapes; shape2 != nullptr;
                shape2 = shape2->nextObject) {

                if (shape2 != localShape) {
                    if (!GeometryOperations::inside(
                            &localIntersection->Point, (SimpleBody *)shape2)) {
                        intersectionFound = LegacyBoolean::FALSE_VALUE;
                        break;
                    }
                }
            }

            if (intersectionFound) {
                depthQueue->add(localIntersection);
                anyIntersectionFound = LegacyBoolean::TRUE_VALUE;
            }
        }
    }

    IntersectionPriorityQueuePool::pqPush(localDepthQueue);

    return (anyIntersectionFound);
}

int
CSG::insideCsgUnion(Vector3Dd *testPoint, SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        if (GeometryOperations::inside(testPoint, (SimpleBody *)localShape)) {
            return (LegacyBoolean::TRUE_VALUE);
        }
    }
    return (LegacyBoolean::FALSE_VALUE);
}

int
CSG::insideCsgIntersection(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *localShape;
    CSG *shape = (CSG *)object;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)localShape)) {
            return (LegacyBoolean::FALSE_VALUE);
        }
    }

    return (LegacyBoolean::TRUE_VALUE);
}

void *
CSG::copyCsg(SimpleBody *object)
{
    CSG *shape = (CSG *)object;
    CSG *newShape;
    Geometry *localShape;
    Geometry *copiedShape;

    newShape = new CSG;
    newShape->methods = shape->methods;
    newShape->Type = shape->Type;
    newShape->nextObject = nullptr;
    newShape->Shapes = nullptr;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy((SimpleBody *)localShape);
        linkShapeNode((SimpleBody *)copiedShape,
            (SimpleBody **)&(copiedShape->nextObject),
            (SimpleBody **)&(newShape->Shapes));
    }
    return ((void *)newShape);
}

void
CSG::translateCsg(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::translate((SimpleBody *)localShape, vector);
    }
}

void
CSG::rotateCsg(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::rotate((SimpleBody *)localShape, vector);
    }
}

void
CSG::scaleCsg(SimpleBody *object, Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = ((CSG *)object)->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::scale((SimpleBody *)localShape, vector);
    }
}

void
CSG::invertCsg(SimpleBody *object)
{
    Geometry *localShape;
    CSG *csg = (CSG *)object;

    if (csg->Type == GeometryOperations::CSG_INTERSECTION_TYPE) {
        csg->Type = GeometryOperations::CSG_UNION_TYPE;
        csg->methods = &CSG::unionMethodTable;
    } else if (csg->Type == GeometryOperations::CSG_UNION_TYPE) {
        csg->Type = GeometryOperations::CSG_INTERSECTION_TYPE;
        csg->methods = &CSG::intersectionMethodTable;
    }

    for (localShape = csg->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::invert((SimpleBody *)localShape);
    }
}
