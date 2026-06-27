#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "common/Config.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.h"

ConstructiveSolidGeometryByMorganRules::ConstructiveSolidGeometryByMorganRules(BooleanSetOperations initialGeometryType) :
    ConstructiveSolidGeometry(initialGeometryType)
{
}

ConstructiveSolidGeometryByMorganRules::ConstructiveSolidGeometryByMorganRules(const ConstructiveSolidGeometryByMorganRules &other) :
    ConstructiveSolidGeometry(other.getGeometryType())
{
    for (long int i = other.getOperands().size() - 1; i >= 0; i--) {
        addOperand(other.getOperands()[i]->copy());
    }
}

int
ConstructiveSolidGeometryByMorganRules::insideCsgChild(Vector3Dd *point, TransformedGeometry *shape)
{
    return shape->doContainmentTest(*point, Config::SMALL_TOLERANCE) != Geometry::OUTSIDE;
}

int
ConstructiveSolidGeometryByMorganRules::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
        return allCsgIntersectIntersections(ray, depthQueue, materialOverride);
    }
    return allCsgUnionIntersections(ray, depthQueue, materialOverride);
}

int
ConstructiveSolidGeometryByMorganRules::allCsgUnionIntersections(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    TransformedGeometry *localShape;

    bool intersectionFound = false;
    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        CsgOperand *operand = getOperands()[i];
        localShape = operand->getGeometry();
        if (localShape->doIntersectionForAllRayCrossings(
                ray, depthQueue, operand->getEffectiveMaterial(materialOverride))) {
            intersectionFound = true;
        }
    }

    return (intersectionFound);
}

int
ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    bool intersectionFound;
    TransformedGeometry *localShape;
    TransformedGeometry *shape2;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    IntersectionCandidate localIntersection;

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);

    bool anyIntersectionFound = false;

    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        CsgOperand *operand = getOperands()[i];
        localShape = operand->getGeometry();

        localShape->doIntersectionForAllRayCrossings(
            ray, localDepthQueue, operand->getEffectiveMaterial(materialOverride));

        for (const IntersectionCandidate& candidate : *localDepthQueue) {
            localIntersection = candidate;

            intersectionFound = true;

            for (long int j = getOperands().size() - 1; j >= 0; j--) {
                shape2 = getOperands()[j]->getGeometry();

                if (shape2 != localShape) {
                    if (!ConstructiveSolidGeometryByMorganRules::insideCsgChild(&localIntersection.getIntersection().point, shape2)) {
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
ConstructiveSolidGeometryByMorganRules::insideCsgUnion(Vector3Dd *testPoint)
{
    TransformedGeometry *localShape;

    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        localShape = getOperands()[i]->getGeometry();

        if (ConstructiveSolidGeometryByMorganRules::insideCsgChild(testPoint, localShape)) {
            return (true);
        }
    }
    return (false);
}

int
ConstructiveSolidGeometryByMorganRules::insideCsgIntersection(Vector3Dd *testPoint)
{
    TransformedGeometry *localShape;

    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        localShape = getOperands()[i]->getGeometry();

        if (!ConstructiveSolidGeometryByMorganRules::insideCsgChild(testPoint, localShape)) {
            return (false);
        }
    }

    return (true);
}

void *
ConstructiveSolidGeometryByMorganRules::copy()
{
    return new ConstructiveSolidGeometryByMorganRules(*this);
}

void
ConstructiveSolidGeometryByMorganRules::invertGeometry()
{
    if (getGeometryType() == BooleanSetOperations::INTERSECTION) {
        setGeometryType(BooleanSetOperations::UNION);
    } else if (getGeometryType() == BooleanSetOperations::UNION) {
        setGeometryType(BooleanSetOperations::INTERSECTION);
    }

    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        getOperands()[i]->invert();
    }
}

int
ConstructiveSolidGeometryByMorganRules::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
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
