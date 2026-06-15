#include "java/util/PriorityQueue.txx"
#include "common/dataStructures/PriorityQueuePool.txx"

/**
csg.c

This module implements routines for constructive solid geometry.
*/

#include "environment/geometry/volume/compound/CSG.h"

inline void
CSG::linkShapeNode(
    Geometry *newObject, Geometry **field, Geometry **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

int
CSG::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    if (geometryType == GeometryTypes::CSG_INTERSECTION_TYPE) {
        return allCsgIntersectIntersections(ray, depthQueue);
    }
    return allCsgUnionIntersections(ray, depthQueue);
}

int
CSG::allCsgUnionIntersections(
    RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    const CSG *shape = this;
    Geometry *localShape;

    intersectionFound = false;
    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {
        if (GeometryOperations::allIntersections(
                localShape, ray, depthQueue)) {
            intersectionFound = true;
        }
    }

    return (intersectionFound);
}

int
CSG::allCsgIntersectIntersections(
    RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    const CSG *shape = this;
    Geometry *localShape;
    Geometry *shape2;
    java::PriorityQueue<Intersection> *localDepthQueue;
    Intersection localIntersection;

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);

    anyIntersectionFound = false;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::allIntersections(
            localShape, ray, localDepthQueue);

        for (const Intersection& candidate : *localDepthQueue) {
            localIntersection = candidate;

            intersectionFound = true;

            for (shape2 = shape->Shapes; shape2 != nullptr;
                shape2 = shape2->nextObject) {

                if (shape2 != localShape) {
                    if (!GeometryOperations::inside(
                            &localIntersection.Point, shape2)) {
                        intersectionFound = false;
                        break;
                    }
                }
            }

            if (intersectionFound) {
                depthQueue->offer(localIntersection);
                anyIntersectionFound = true;
            }
        }

        localDepthQueue->clear();
    }

    PriorityQueuePool<Intersection>::pqPush(localDepthQueue);

    return (anyIntersectionFound);
}

int
CSG::insideCsgUnion(Vector3Dd *testPoint)
{
    const CSG *shape = this;
    Geometry *localShape;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        if (GeometryOperations::inside(testPoint, localShape)) {
            return (true);
        }
    }
    return (false);
}

int
CSG::insideCsgIntersection(Vector3Dd *testPoint)
{
    Geometry *localShape;
    const CSG *shape = this;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        if (!GeometryOperations::inside(testPoint, localShape)) {
            return (false);
        }
    }

    return (true);
}

void *
CSG::copy()
{
    const CSG *shape = this;
    CSG *newShape;
    Geometry *localShape;
    Geometry *copiedShape;

    newShape = new CSG;
    newShape->geometryType = shape->geometryType;
    newShape->nextObject = nullptr;
    newShape->Shapes = nullptr;

    for (localShape = shape->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        copiedShape =
            (Geometry *)GeometryOperations::copy(localShape);
        CSG::linkShapeNode(copiedShape,
            &(copiedShape->nextObject),
            &(newShape->Shapes));
    }
    return ((void *)newShape);
}

void
CSG::translate(Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = this->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::translate(localShape, vector);
    }
}

void
CSG::rotate(Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = this->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::rotate(localShape, vector);
    }
}

void
CSG::scale(Vector3Dd *vector)
{
    Geometry *localShape;

    for (localShape = this->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::scale(localShape, vector);
    }
}

void
CSG::invert()
{
    Geometry *localShape;
    CSG * const csg = this;

    if (csg->geometryType == GeometryTypes::CSG_INTERSECTION_TYPE) {
        csg->geometryType = GeometryTypes::CSG_UNION_TYPE;
    } else if (csg->geometryType == GeometryTypes::CSG_UNION_TYPE) {
        csg->geometryType = GeometryTypes::CSG_INTERSECTION_TYPE;
    }

    for (localShape = csg->Shapes; localShape != nullptr;
        localShape = localShape->nextObject) {

        GeometryOperations::invert(localShape);
    }
}

int
CSG::inside(Vector3Dd *point)
{
    if (geometryType == GeometryTypes::CSG_INTERSECTION_TYPE) {
        return insideCsgIntersection(point);
    }
    return insideCsgUnion(point);
}
