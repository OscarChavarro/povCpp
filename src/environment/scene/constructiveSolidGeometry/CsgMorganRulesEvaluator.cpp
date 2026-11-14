#include "environment/geometry/element/GeometryConfig.h"
#include "environment/scene/constructiveSolidGeometry/CsgMorganRulesEvaluator.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingSelection.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
CsgMorganRulesEvaluator::insideOperand(const Vector3Dd &point, CsgOperand *operand)
{
    return operand->doContainmentTest(point, GeometryConfig::SMALL_TOLERANCE) != RayCastingContainment::OUTSIDE;
}

int
CsgMorganRulesEvaluator::allRayCrossings(
    CsgOperation operation,
    const java::ArrayList<CsgOperand*> &operands,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operation == CsgOperation::INTERSECTION) {
        return intersectionCrossings(ray, operands, depthQueue, materialOverride);
    }
    bool intersectionFound = false;
    for (long int i = operands.size() - 1; i >= 0; i--) {
        if (operands[i]->doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride)) {
            intersectionFound = true;
        }
    }
    return intersectionFound;
}

int
CsgMorganRulesEvaluator::intersectionCrossings(
    RayWithTracingState *ray,
    const java::ArrayList<CsgOperand*> &operands,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        ray->getIntersectionQueuePool()->pop(128);

    bool anyIntersectionFound = false;

    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->doIntersectionForAllRayCrossings(ray, localDepthQueue, materialOverride);

        // Read crossings straight from the scratch queue: nothing here mutates
        // them, the survivors are copied once into depthQueue, and the scratch
        // queue is cleared right after, so no defensive per-crossing copy.
        for (IntersectionCandidate& candidate : *localDepthQueue) {
            const Vector3Dd &point = candidate.getIntersection().point;
            auto inside = [&operands, &point](long int j) {
                return CsgMorganRulesEvaluator::insideOperand(point, operands[j]);
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

    return anyIntersectionFound;
}

bool
CsgMorganRulesEvaluator::contains(
    CsgOperation operation, const java::ArrayList<CsgOperand*> &operands, const Vector3Dd &point)
{
    if (operation == CsgOperation::INTERSECTION) {
        for (long int i = operands.size() - 1; i >= 0; i--) {
            if (!insideOperand(point, operands[i])) {
                return false;
            }
        }
        return true;
    }
    for (long int i = operands.size() - 1; i >= 0; i--) {
        if (insideOperand(point, operands[i])) {
            return true;
        }
    }
    return false;
}

CsgOperation
CsgMorganRulesEvaluator::invert(CsgOperation operation, const java::ArrayList<CsgOperand*> &operands)
{
    CsgOperation result = operation;
    if (operation == CsgOperation::INTERSECTION) {
        result = CsgOperation::UNION;
    } else if (operation == CsgOperation::UNION) {
        result = CsgOperation::INTERSECTION;
    }
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->invert();
    }
    return result;
}
