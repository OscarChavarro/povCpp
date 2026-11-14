
#include "render/bakedScene/CsgContainmentTest.h"

#include "environment/geometry/element/GeometryConfig.h"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingSelection.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

int
CsgContainmentTest::containmentTestOperand(
    const CsgOperandRecord *operand,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    switch (operand->getKind()) {
    case BakedScene::CsgOperandKind::Empty:
        return RayCastingGeometry::OUTSIDE;

    case BakedScene::CsgOperandKind::DirectPlane:
        return BakedPlaneIntersector::planeContainmentTest(operand, point, distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedPlane:
        return BakedPlaneIntersector::planeContainmentTest(
            operand,
            operand->getLocalToObject().transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::NestedCsg:
        return containmentTest(
            bakedCsgs[operand->getNestedCsgProgramIndex()],
            bakedCsgs,
            point,
            distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedNestedCsg:
        return containmentTest(
            bakedCsgs[operand->getNestedCsgProgramIndex()],
            bakedCsgs,
            operand->getLocalToObject().transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedQuadric:
    case BakedScene::CsgOperandKind::TransformedSphere:
    case BakedScene::CsgOperandKind::TransformedPrimitive:
        return operand->getGeometry()->doContainmentTest(
            operand->getLocalToObject().transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
    case BakedScene::CsgOperandKind::DirectPrimitive:
    case BakedScene::CsgOperandKind::GenericFallback:
        return operand->getGeometry() != nullptr ?
            operand->getGeometry()->doContainmentTest(point, distanceTolerance) :
            RayCastingGeometry::OUTSIDE;
    }
    return RayCastingGeometry::OUTSIDE;
}

bool
CsgContainmentTest::candidateInsideAllOtherOperands(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    return candidateInsideOperandsCoreFirst(bakedCsg, bakedCsgs, point, skipIndex, -1);
}

bool
CsgContainmentTest::candidateInsideOperandsCoreFirst(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    const java::ArrayList<CsgOperandRecord> &operands = bakedCsg->getOperands();
    auto inside = [&operands, &bakedCsgs, &point](long int j) {
        return containmentTestOperand(&operands[j], bakedCsgs, point,
            GeometryConfig::SMALL_TOLERANCE) != RayCastingGeometry::OUTSIDE;
    };
    return CsgCrossingSelection<decltype(inside)>::isSelected(
        CsgOperation::INTERSECTION, skipIndex, operands.size(), coreIndex, inside);
}

int
CsgContainmentTest::containmentTest(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (bakedCsg->getOperands().size() == 0) {
        return RayCastingGeometry::OUTSIDE;
    }

    if (bakedCsg->getPlanKind() ==
        BakedScene::CsgPlanKind::DisjointBoundedUnion) {
        for (long int i = 0; i < bakedCsg->getOperands().size(); i++) {
            const CsgOperandRecord *operand = &bakedCsg->getOperands()[i];
            if (!operand->getBakedBounds().containsPointWithTolerance(point, distanceTolerance)) {
                continue;
            }
            if (containmentTestOperand(
                    operand,
                    bakedCsgs,
                    point,
                    distanceTolerance) != RayCastingGeometry::OUTSIDE) {
                return RayCastingGeometry::INSIDE;
            }
        }
        return RayCastingGeometry::OUTSIDE;
    }

    // The Roth fold of the POV evaluators, stopping where the remaining
    // operands cannot change the result.
    const CsgOperation operation = bakedCsg->getGeometryType();
    bool isInside = containmentTestOperand(
        &bakedCsg->getOperands()[0], bakedCsgs, point, distanceTolerance) != RayCastingGeometry::OUTSIDE;
    for (long int i = 1; i < bakedCsg->getOperands().size(); i++) {
        if (CsgMembershipRule::isDecided(operation, isInside)) {
            break;
        }
        isInside = CsgMembershipRule::combine(operation, isInside,
            containmentTestOperand(
                &bakedCsg->getOperands()[i], bakedCsgs, point, distanceTolerance) !=
                RayCastingGeometry::OUTSIDE);
    }
    return isInside ? RayCastingGeometry::INSIDE : RayCastingGeometry::OUTSIDE;
}
