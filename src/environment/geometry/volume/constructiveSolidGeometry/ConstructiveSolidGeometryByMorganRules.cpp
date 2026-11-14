#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingSelection.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

ConstructiveSolidGeometryByMorganRules::ConstructiveSolidGeometryByMorganRules(CsgOperation initialGeometryType) :
    ConstructiveSolidGeometry(initialGeometryType)
{
}

ConstructiveSolidGeometryByMorganRules::ConstructiveSolidGeometryByMorganRules(const ConstructiveSolidGeometryByMorganRules &other) :
    ConstructiveSolidGeometry(other.getGeometryType())
{
    for (long int i = other.getOperands().size() - 1; i >= 0; i--) {
        addOperand(static_cast<CsgOperand *>(other.getOperands()[i]->copy()));
    }
}

int
ConstructiveSolidGeometryByMorganRules::insideCsgChild(Vector3Dd *point, CsgOperand *operand)
{
    return operand->doContainmentTest(*point, GeometryConfig::SMALL_TOLERANCE) != RayCastingGeometry::OUTSIDE;
}

int
ConstructiveSolidGeometryByMorganRules::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (getGeometryType() == CsgOperation::INTERSECTION) {
        return allCsgIntersectIntersections(ray, depthQueue, materialOverride);
    }
    return allCsgUnionIntersections(ray, depthQueue, materialOverride);
}

int
ConstructiveSolidGeometryByMorganRules::allCsgUnionIntersections(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    bool intersectionFound = false;
    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        CsgOperand *operand = getOperands()[i];
        if (operand->doIntersectionForAllRayCrossings(
                ray, depthQueue, materialOverride)) {
            intersectionFound = true;
        }
    }
    return (intersectionFound);
}

int
ConstructiveSolidGeometryByMorganRules::allCsgIntersectIntersections(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    const java::ArrayList<CsgOperand*> &operands = getOperands();
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);

    bool anyIntersectionFound = false;

    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->doIntersectionForAllRayCrossings(
            ray, localDepthQueue, materialOverride);

        // Read crossings straight from the scratch queue: nothing here mutates
        // them, the survivors are copied once into depthQueue, and the scratch
        // queue is cleared right after, so no defensive per-crossing copy.
        for (IntersectionCandidate& candidate : *localDepthQueue) {
            Vector3Dd *point = &candidate.getIntersection().point;
            auto inside = [&operands, point](long int j) {
                return ConstructiveSolidGeometryByMorganRules::insideCsgChild(point, operands[j]) != 0;
            };
            if (CsgCrossingSelection<decltype(inside)>::isSelected(
                    CsgOperation::INTERSECTION, i, operands.size(), -1, inside)) {
                depthQueue->offer(candidate);
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
    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        if (ConstructiveSolidGeometryByMorganRules::insideCsgChild(
                testPoint, getOperands()[i])) {
            return (true);
        }
    }
    return (false);
}

int
ConstructiveSolidGeometryByMorganRules::insideCsgIntersection(Vector3Dd *testPoint)
{
    for (long int i = getOperands().size() - 1; i >= 0; i--) {
        if (!ConstructiveSolidGeometryByMorganRules::insideCsgChild(
                testPoint, getOperands()[i])) {
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
    if (getGeometryType() == CsgOperation::INTERSECTION) {
        setGeometryType(CsgOperation::UNION);
    } else if (getGeometryType() == CsgOperation::UNION) {
        setGeometryType(CsgOperation::INTERSECTION);
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
    if (getGeometryType() == CsgOperation::INTERSECTION) {
        isInside = insideCsgIntersection(&mutablePoint);
    } else {
        isInside = insideCsgUnion(&mutablePoint);
    }
    return isInside ? INSIDE : OUTSIDE;
}
