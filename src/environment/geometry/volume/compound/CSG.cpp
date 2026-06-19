/**
csg.c

This module implements routines for constructive solid geometry.
*/

#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/SimpleBody.h"

#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"

CSG::CSG(GeometryTypes initialGeometryType) :
    geometryType(initialGeometryType)
{
}

int
CSG::insideCsgChild(Vector3Dd *point, TransformableElement *shape)
{
    // CSG children are produced by the POV parsers through ModelBuilder::wrap,
    // so the stored TransformableElement is a SimpleBody. CSG inside tests
    // only need geometry, not material/colour ownership.
    return static_cast<SimpleBody *>(shape)->getGeometry()->inside(point);
}

int
CSG::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    if (getGeometryType() == GeometryTypes::CSG_INTERSECTION_TYPE) {
        return allCsgIntersectIntersections(ray, depthQueue);
    }
    return allCsgUnionIntersections(ray, depthQueue);
}

int
CSG::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    (void)owner;
    return allIntersections(ray, depthQueue);
}

int
CSG::allCsgUnionIntersections(
    RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    bool intersectionFound;
    const CSG *shape = this;
    TransformableElement *localShape;

    intersectionFound = false;
    for (long int i = shape->getShapes().size() - 1; i >= 0; i--) {
        localShape = shape->getShapes()[i];
        if (localShape->allIntersections(ray, depthQueue)) {
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

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);

    anyIntersectionFound = false;

    for (long int i = shape->getShapes().size() - 1; i >= 0; i--) {
        localShape = shape->getShapes()[i];

        localShape->allIntersections(ray, localDepthQueue);

        for (const Intersection& candidate : *localDepthQueue) {
            localIntersection = candidate;

            intersectionFound = true;

            for (long int j = shape->getShapes().size() - 1; j >= 0; j--) {
                shape2 = shape->getShapes()[j];

                if (shape2 != localShape) {
                    if (!CSG::insideCsgChild(&localIntersection.getPoint(), shape2)) {
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

    ray->getIntersectionQueuePool()->push(localDepthQueue);

    return (anyIntersectionFound);
}

int
CSG::insideCsgUnion(Vector3Dd *testPoint)
{
    const CSG *shape = this;
    TransformableElement *localShape;

    for (long int i = shape->getShapes().size() - 1; i >= 0; i--) {
        localShape = shape->getShapes()[i];

        if (CSG::insideCsgChild(testPoint, localShape)) {
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

    for (long int i = shape->getShapes().size() - 1; i >= 0; i--) {
        localShape = shape->getShapes()[i];

        if (!CSG::insideCsgChild(testPoint, localShape)) {
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

    newShape = new CSG(shape->getGeometryType());

    for (long int i = shape->getShapes().size() - 1; i >= 0; i--) {
        localShape = shape->getShapes()[i];

        copiedShape =
            (TransformableElement *)localShape->copy();
        newShape->getShapes().add(copiedShape);
    }
    return ((void *)newShape);
}

// A CSG is a container of SimpleBody children. Transforming it recurses
// into each child's full transform (geometry + material). The same recursion
// serves both as the direct entry point (CSG::translate, used while parsing the
// union's own braces) and as the geometric entry point invoked when the CSG is
// itself wrapped in a SimpleBody (SimpleBody::translate -> translateGeometry).

void
CSG::translateGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = this->getShapes().size() - 1; i >= 0; i--) {
        localShape = this->getShapes()[i];

        localShape->translate(vector);
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

    for (long int i = this->getShapes().size() - 1; i >= 0; i--) {
        localShape = this->getShapes()[i];

        localShape->rotate(vector);
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

    for (long int i = this->getShapes().size() - 1; i >= 0; i--) {
        localShape = this->getShapes()[i];

        localShape->scale(vector);
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

    if (csg->getGeometryType() == GeometryTypes::CSG_INTERSECTION_TYPE) {
        csg->setGeometryType(GeometryTypes::CSG_UNION_TYPE);
    } else if (csg->getGeometryType() == GeometryTypes::CSG_UNION_TYPE) {
        csg->setGeometryType(GeometryTypes::CSG_INTERSECTION_TYPE);
    }

    for (long int i = csg->getShapes().size() - 1; i >= 0; i--) {
        localShape = csg->getShapes()[i];

        localShape->invert();
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
    if (getGeometryType() == GeometryTypes::CSG_INTERSECTION_TYPE) {
        return insideCsgIntersection(point);
    }
    return insideCsgUnion(point);
}
