#ifndef __CSG_CONTAINMENT_TEST__
#define __CSG_CONTAINMENT_TEST__

#include "render/bakedScene/BakedScene.h"

// Point-in-solid containment testing for baked CSG programs: per-operand
// containment dispatch plus the point-in-membership checks used by the
// ray-segment and Morgan-intersection trace paths to validate a candidate
// crossing against the operands it did not come from.
class CsgContainmentTest {
public:
    static int containmentTest(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        double distanceTolerance);

    static int containmentTestOperand(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool candidateInsideAllOtherOperands(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex);

    static bool candidateInsideOperandsCoreFirst(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex,
        long int coreIndex);
};

#endif
