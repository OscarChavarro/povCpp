#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/SimpleBody.h"

CSG::CSG(BooleanSetOperations initialGeometryType) :
    geometryType(initialGeometryType)
{
}

int
CSG::insideCsgChild(Vector3Dd *point, TransformableElement *shape)
{
    // CSG children are produced by the POV parsers through SceneBuilder::wrap,
    // so the stored TransformableElement is a SimpleBody. CSG inside tests
    // only need geometry, not material/colour ownership.
    return static_cast<SimpleBody *>(shape)->getGeometry()->inside(point);
}

int
CSG::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
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
    return allIntersections(ray, depthQueue);
}

int
CSG::allCsgUnionIntersections(
    RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    const CSG *shape = this;
    TransformableElement *localShape;

    bool intersectionFound = false;
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
    const CSG *shape = this;
    TransformableElement *localShape;
    TransformableElement *shape2;
    java::PriorityQueue<Intersection> *localDepthQueue;
    Intersection localIntersection;

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);

    bool anyIntersectionFound = false;

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

CSG::CSG(const CSG &other) :
    geometryType(other.geometryType)
{
    for (long int i = other.getShapes().size() - 1; i >= 0; i--) {
        shapes.add((TransformableElement *)other.getShapes()[i]->copy());
    }
}

void *
CSG::copy()
{
    return new CSG(*this);
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

    if (csg->getGeometryType() == BooleanSetOperations::INTERSECTION) {
        csg->setGeometryType(BooleanSetOperations::UNION);
    } else if (csg->getGeometryType() == BooleanSetOperations::UNION) {
        csg->setGeometryType(BooleanSetOperations::INTERSECTION);
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
    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
        return insideCsgIntersection(point);
    }
    return insideCsgUnion(point);
}
