#include "java/util/PriorityQueue.txx"
#include "common/dataStructures/PriorityQueuePool.txx"

/**
csg.c

This module implements routines for constructive solid geometry.
*/

#include "environment/geometry/volume/compound/CSG.h"
#include "java/util/ArrayList.txx"

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
    TransformableElement *localShape;

    intersectionFound = false;
    for (long int i = shape->shapes.size() - 1; i >= 0; i--) {
        localShape = shape->shapes[i];
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
    TransformableElement *localShape;
    TransformableElement *shape2;
    java::PriorityQueue<Intersection> *localDepthQueue;
    Intersection localIntersection;

    localDepthQueue = PriorityQueuePool<Intersection>::pqPop(128);

    anyIntersectionFound = false;

    for (long int i = shape->shapes.size() - 1; i >= 0; i--) {
        localShape = shape->shapes[i];

        GeometryOperations::allIntersections(
            localShape, ray, localDepthQueue);

        for (const Intersection& candidate : *localDepthQueue) {
            localIntersection = candidate;

            intersectionFound = true;

            for (long int j = shape->shapes.size() - 1; j >= 0; j--) {
                shape2 = shape->shapes[j];

                if (shape2 != localShape) {
                    if (!GeometryOperations::inside(
                            &localIntersection.point, shape2)) {
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
    TransformableElement *localShape;

    for (long int i = shape->shapes.size() - 1; i >= 0; i--) {
        localShape = shape->shapes[i];

        if (GeometryOperations::inside(testPoint, localShape)) {
            return (true);
        }
    }
    return (false);
}

int
CSG::insideCsgIntersection(Vector3Dd *testPoint)
{
    TransformableElement *localShape;
    const CSG *shape = this;

    for (long int i = shape->shapes.size() - 1; i >= 0; i--) {
        localShape = shape->shapes[i];

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
    TransformableElement *localShape;
    TransformableElement *copiedShape;

    newShape = new CSG;
    newShape->geometryType = shape->geometryType;

    for (long int i = shape->shapes.size() - 1; i >= 0; i--) {
        localShape = shape->shapes[i];

        copiedShape =
            (TransformableElement *)GeometryOperations::copy(localShape);
        newShape->shapes.add(copiedShape);
    }
    return ((void *)newShape);
}

// A CSG is a container of TranslatedBody children. Transforming it recurses
// into each child's full transform (geometry + material). The same recursion
// serves both as the direct entry point (CSG::translate, used while parsing the
// union's own braces) and as the geometric entry point invoked when the CSG is
// itself wrapped in a TranslatedBody (TranslatedBody::translate -> translateGeometry).

void
CSG::translateGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->shapes.size() - 1; i >= 0; i--) {
        localShape = this->shapes[i];

        GeometryOperations::translate(localShape, vector);
    }
}

void
CSG::translate(Vector3Dd *vector)
{
    translateGeometry(vector);
}

void
CSG::rotateGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->shapes.size() - 1; i >= 0; i--) {
        localShape = this->shapes[i];

        GeometryOperations::rotate(localShape, vector);
    }
}

void
CSG::rotate(Vector3Dd *vector)
{
    rotateGeometry(vector);
}

void
CSG::scaleGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->shapes.size() - 1; i >= 0; i--) {
        localShape = this->shapes[i];

        GeometryOperations::scale(localShape, vector);
    }
}

void
CSG::scale(Vector3Dd *vector)
{
    scaleGeometry(vector);
}

void
CSG::invertGeometry()
{
    TransformableElement *localShape;
    CSG * const csg = this;

    if (csg->geometryType == GeometryTypes::CSG_INTERSECTION_TYPE) {
        csg->geometryType = GeometryTypes::CSG_UNION_TYPE;
    } else if (csg->geometryType == GeometryTypes::CSG_UNION_TYPE) {
        csg->geometryType = GeometryTypes::CSG_INTERSECTION_TYPE;
    }

    for (long int i = csg->shapes.size() - 1; i >= 0; i--) {
        localShape = csg->shapes[i];

        GeometryOperations::invert(localShape);
    }
}

void
CSG::invert()
{
    invertGeometry();
}

int
CSG::inside(Vector3Dd *point)
{
    if (geometryType == GeometryTypes::CSG_INTERSECTION_TYPE) {
        return insideCsgIntersection(point);
    }
    return insideCsgUnion(point);
}
