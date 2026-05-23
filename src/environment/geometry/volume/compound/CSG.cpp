/****************************************************************************
 *                         csg.c
 *
 *  This module implements routines for constructive solid geometry.
 *
 *****************************************************************************/

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
Methods csgUnionMethods = {Composite::objectIntersect,
    CSG::allCsgUnionIntersections, CSG::insideCsgUnion, nullptr, CSG::copyCsg,
    CSG::translateCsg, CSG::rotateCsg, CSG::scaleCsg, CSG::invertCsg};

Methods csgIntersectionMethods = {Composite::objectIntersect,
    CSG::allCsgIntersectIntersections, CSG::insideCsgIntersection, nullptr,
    CSG::copyCsg, CSG::translateCsg, CSG::rotateCsg, CSG::scaleCsg,
    CSG::invertCsg};

int
CSG::allCsgUnionIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    int intersectionFound;
    CSG *shape = (CSG *)object;
    Geometry *localShape;

    intersectionFound = FALSE;
    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        if (GeometryOperations::allIntersections(
                (SimpleBody *)localShape, ray, depthQueue)) {
            intersectionFound = TRUE;
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

    anyIntersectionFound = FALSE;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::allIntersections(
            (SimpleBody *)localShape, ray, localDepthQueue);

        for (localIntersection = localDepthQueue->getHighest();
            localIntersection != nullptr; localDepthQueue->deleteHighest(),
            localIntersection = localDepthQueue->getHighest()) {

            intersectionFound = TRUE;

            for (shape2 = shape->Shapes; shape2 != nullptr;
                shape2 = shape2->nextObject) {

                if (shape2 != localShape) {
                    if (!GeometryOperations::inside(
                            &localIntersection->Point, (SimpleBody *)shape2)) {
                        intersectionFound = FALSE;
                        break;
                    }
                }
            }

            if (intersectionFound) {
                depthQueue->add(localIntersection);
                anyIntersectionFound = TRUE;
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
            return (TRUE);
        }
    }
    return (FALSE);
}

int
CSG::insideCsgIntersection(Vector3Dd *testPoint, SimpleBody *object)
{
    Geometry *localShape;
    CSG *shape = (CSG *)object;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        if (!GeometryOperations::inside(testPoint, (SimpleBody *)localShape)) {
            return (FALSE);
        }
    }

    return (TRUE);
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
        ObjectUtils::link((SimpleBody *)copiedShape,
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

    if (csg->Type == CSG_INTERSECTION_TYPE) {
        csg->Type = CSG_UNION_TYPE;
        csg->methods = &csgUnionMethods;
    } else if (csg->Type == CSG_UNION_TYPE) {
        csg->Type = CSG_INTERSECTION_TYPE;
        csg->methods = &csgIntersectionMethods;
    }

    for (localShape = csg->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::invert((SimpleBody *)localShape);
    }
}
