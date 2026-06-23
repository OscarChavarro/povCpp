#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "common/Config.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/compound/CSG.h"

CSG::CSG(BooleanSetOperations initialGeometryType) :
    geometryType(initialGeometryType)
{
}

CSG::~CSG()
{
    for (long int i = 0; i < shapes.size(); i++) {
        delete shapes[i];
    }
}

int
CSG::insideCsgChild(Vector3Dd *point, TransformableElement *shape)
{
    // CSG children are produced by the POV parsers through SceneBuilder::wrap,
    // so the stored TransformableElement is a SimpleBody; its doContainmentTest()
    // override forwards straight to the wrapped geometry, with no need to
    // know that concretely here.
    return shape->doContainmentTest(*point, Config::SMALL_TOLERANCE) != TransformableElement::OUTSIDE;
}

int
CSG::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
        return allCsgIntersectIntersections(ray, depthQueue);
    }
    return allCsgUnionIntersections(ray, depthQueue);
}

int
CSG::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *material)
{
    (void)material;
    return allIntersections(ray, depthQueue);
}

int
CSG::allCsgUnionIntersections(
    RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
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
    RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    bool intersectionFound;
    const CSG *shape = this;
    TransformableElement *localShape;
    TransformableElement *shape2;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    IntersectionCandidate localIntersection;

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);

    bool anyIntersectionFound = false;

    for (long int i = shape->getShapes().size() - 1; i >= 0; i--) {
        localShape = shape->getShapes()[i];

        localShape->allIntersections(ray, localDepthQueue);

        for (const IntersectionCandidate& candidate : *localDepthQueue) {
            localIntersection = candidate;

            intersectionFound = true;

            for (long int j = shape->getShapes().size() - 1; j >= 0; j--) {
                shape2 = shape->getShapes()[j];

                if (shape2 != localShape) {
                    if (!CSG::insideCsgChild(&localIntersection.getIntersection().point, shape2)) {
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
CSG::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)distanceTolerance;
    Vector3Dd mutablePoint = point;
    bool isInside;
    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
        isInside = insideCsgIntersection(&mutablePoint);
    } else {
        isInside = insideCsgUnion(&mutablePoint);
    }
    return isInside ? INSIDE : OUTSIDE;
}
