#ifndef __BAKED_SCENE_BUILDER__
#define __BAKED_SCENE_BUILDER__

#include "environment/scene/SimpleBody.h"
#include "render/bakedScene/BakedScene.h"

class ConstructiveSolidGeometry;

class BakedSceneBuilder {
  public:
    static void build(const java::ArrayList<SimpleBody*> &objects, BakedScene &out);

  private:
    static bool isBakeableSimpleBody(SimpleBody *object);
    static Matrix4x4d copyOrIdentity(const Matrix4x4d *matrix);
    static BakedScene::TraceKind classifyTraceableObject(
        bool hasGeometry, bool boundedOrClipped, bool hasCsg);
    static BakedScene::CsgOperandKind classifyOperandKind(
        const BakedScene::CsgOperandRecord &baked);
    static bool hasFiniteInteriorBounds(const BakedScene::CsgOperandRecord &operand);
    static bool areSeparated(
        const AxisAlignedBoundingBox &left, const AxisAlignedBoundingBox &right);
    static bool hasPairwiseDisjointFiniteOperands(
        const java::ArrayList<BakedScene::CsgOperandRecord> &operands);
    static void classifyCsgProgramSpecialization(BakedScene::CsgProgram &baked);
    static void buildCsgExecutionPlan(BakedScene::CsgProgram &baked, const BakedScene &out);
    static void compileTracingObjects(
        const java::ArrayList<SimpleBody*> &objects,
        BakedScene &out,
        java::ArrayList<int> &result);
    static BakedScene::TraceableObject bakeSimpleBody(SimpleBody *object, BakedScene &out);
    static bool isPushdownCandidateOperand(const BakedScene::CsgOperandRecord &operand);
    static bool nestedProgramFullyBakeable(
        const BakedScene &scene, int programIndex, int depthGuard);
    static void pushDownStepsIntoProgram(
        BakedScene &out, int programIndex,
        const java::ArrayList<TransformStep> &parentSteps,
        const Matrix4x4d &parentForwardTransform);
    static BakedScene::CsgOperandRecord bakeCsgOperand(CsgOperand *operand, BakedScene &out);

    static constexpr long int kOperandCullBinThreshold = 17;
    static constexpr bool kEnableOperandCullBins = true;

    // Sort key (chosen axis centroid coordinate) paired with the entry's
    // position in the bucket - private replacement for std::pair<double, int>.
    struct CullSafeEntry {
        double key;
        int position;
    };

    // Ascending insertion sort by key. Bake-time only, called once per wide
    // union bucket (never on the per-ray path) - same complexity trade-off
    // already accepted by AabbCullingSupport::sortPositionsDescending for
    // small per-ray arrays, here applied to a larger but still one-shot list.
    static void sortCullSafeEntriesByKey(java::ArrayList<CullSafeEntry> &entries);

    // Bake-time only. Stores bucket *positions* (see
    // BakedScene::OperandCullBins), never operand pointers or global
    // indices, so nothing dangles across later ArrayList relocations.
    static void buildOperandCullBinsForBucket(
        const java::ArrayList<int> &bucket,
        const java::ArrayList<BakedScene::CsgOperandRecord> &operands,
        BakedScene::OperandCullBins &out);

    static BakedScene::CsgProgram bakeConstructiveSolidGeometry(
        ConstructiveSolidGeometry *geometry, BakedScene &out);
    static int compileConstructiveSolidGeometry(
        ConstructiveSolidGeometry *geometry, BakedScene &out);
    static BakedScene::CompositeRecord bakeComposite(Composite *object, BakedScene &out);
    static int compileTracingObject(SimpleBody *object, BakedScene &out);
    static void accumulateStatistics(BakedScene &scene);
};

#endif
