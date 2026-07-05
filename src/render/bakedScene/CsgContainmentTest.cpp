#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "render/bakedScene/CsgContainmentTest.h"

#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/Geometry.h"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"

int
CsgContainmentTest::containmentTestOperand(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    switch (operand.kind) {
    case BakedScene::CsgOperandKind::Empty:
        return Geometry::OUTSIDE;

    case BakedScene::CsgOperandKind::DirectPlane:
        return BakedPlaneIntersector::planeContainmentTest(operand, point, distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedPlane:
        return BakedPlaneIntersector::planeContainmentTest(
            operand,
            operand.localToObject.transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::NestedCsg:
        return containmentTest(
            bakedCsgs[operand.nestedCsgProgramIndex],
            bakedCsgs,
            point,
            distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedNestedCsg:
        return containmentTest(
            bakedCsgs[operand.nestedCsgProgramIndex],
            bakedCsgs,
            operand.localToObject.transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::TransformedQuadric:
    case BakedScene::CsgOperandKind::TransformedSphere:
    case BakedScene::CsgOperandKind::TransformedPrimitive:
        return operand.geometry->doContainmentTest(
            operand.localToObject.transformPoint(point),
            distanceTolerance);

    case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
    case BakedScene::CsgOperandKind::DirectPrimitive:
    case BakedScene::CsgOperandKind::GenericFallback:
        return operand.geometry != nullptr ?
            operand.geometry->doContainmentTest(point, distanceTolerance) :
            Geometry::OUTSIDE;
    }
    return Geometry::OUTSIDE;
}

bool
CsgContainmentTest::candidateInsideAllOtherOperands(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    for (long int j = bakedCsg.operands.size() - 1; j >= 0; j--) {
        if (j == skipIndex) {
            continue;
        }
        if (containmentTestOperand(
                bakedCsg.operands[j],
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
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    if (coreIndex >= 0 && coreIndex != skipIndex) {
        if (containmentTestOperand(
                bakedCsg.operands[coreIndex],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    for (long int j = bakedCsg.operands.size() - 1; j >= 0; j--) {
        if (j == skipIndex || j == coreIndex) {
            continue;
        }
        if (containmentTestOperand(
                bakedCsg.operands[j],
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
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (bakedCsg.operands.size() == 0) {
        return Geometry::OUTSIDE;
    }

    if (bakedCsg.planKind ==
        BakedScene::CsgPlanKind::DisjointBoundedUnion) {
        for (long int i = 0; i < bakedCsg.operands.size(); i++) {
            const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[i];
            if (!AabbCullingSupport::pointInsideAabb(point, operand.bakedBounds, distanceTolerance)) {
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
    switch (bakedCsg.geometryType) {
    case BooleanSetOperations::DIFFERENCE:
        isInside =
            containmentTestOperand(
                bakedCsg.operands[0], bakedCsgs, point, distanceTolerance) != Geometry::OUTSIDE;
        for (long int i = 1; isInside && (i < bakedCsg.operands.size()); i++) {
            if (containmentTestOperand(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) != Geometry::OUTSIDE) {
                isInside = false;
            }
        }
        break;

    case BooleanSetOperations::INTERSECTION:
        isInside = true;
        for (long int i = 0; isInside && (i < bakedCsg.operands.size()); i++) {
            if (containmentTestOperand(
                    bakedCsg.operands[i],
                    bakedCsgs,
                    point,
                    distanceTolerance) == Geometry::OUTSIDE) {
                isInside = false;
            }
        }
        break;

    default:
        isInside = false;
        for (long int i = 0; !isInside && (i < bakedCsg.operands.size()); i++) {
            if (containmentTestOperand(
                    bakedCsg.operands[i],
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
