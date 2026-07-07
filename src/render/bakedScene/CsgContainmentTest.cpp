
#include "render/bakedScene/CsgContainmentTest.h"

#include "environment/geometry/element/GeometryConfig.h"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"
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
        return Geometry::OUTSIDE;

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
            Geometry::OUTSIDE;
    }
    return Geometry::OUTSIDE;
}

bool
CsgContainmentTest::candidateInsideAllOtherOperands(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    for (long int j = bakedCsg->getOperands().size() - 1; j >= 0; j--) {
        if (j == skipIndex) {
            continue;
        }
        if (containmentTestOperand(
                &bakedCsg->getOperands()[j],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
CsgContainmentTest::candidateInsideOperandsCoreFirst(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    if (coreIndex >= 0 && coreIndex != skipIndex) {
        if (containmentTestOperand(
                &bakedCsg->getOperands()[coreIndex],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    for (long int j = bakedCsg->getOperands().size() - 1; j >= 0; j--) {
        if (j == skipIndex || j == coreIndex) {
            continue;
        }
        if (containmentTestOperand(
                &bakedCsg->getOperands()[j],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

int
CsgContainmentTest::containmentTest(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (bakedCsg->getOperands().size() == 0) {
        return Geometry::OUTSIDE;
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
                    distanceTolerance) != Geometry::OUTSIDE) {
                return Geometry::INSIDE;
            }
        }
        return Geometry::OUTSIDE;
    }

    bool isInside;
    switch (bakedCsg->getGeometryType()) {
    case BooleanSetOperations::DIFFERENCE:
        isInside =
            containmentTestOperand(
                &bakedCsg->getOperands()[0], bakedCsgs, point, distanceTolerance) != Geometry::OUTSIDE;
        for (long int i = 1; isInside && (i < bakedCsg->getOperands().size()); i++) {
            if (containmentTestOperand(
                    &bakedCsg->getOperands()[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) != Geometry::OUTSIDE) {
                isInside = false;
            }
        }
        break;

    case BooleanSetOperations::INTERSECTION:
        isInside = true;
        for (long int i = 0; isInside && (i < bakedCsg->getOperands().size()); i++) {
            if (containmentTestOperand(
                    &bakedCsg->getOperands()[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) == Geometry::OUTSIDE) {
                isInside = false;
            }
        }
        break;

    default:
        isInside = false;
        for (long int i = 0; !isInside && (i < bakedCsg->getOperands().size()); i++) {
            if (containmentTestOperand(
                    &bakedCsg->getOperands()[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) != Geometry::OUTSIDE) {
                isInside = true;
            }
        }
        break;
    }
    return isInside ? Geometry::INSIDE : Geometry::OUTSIDE;
}
