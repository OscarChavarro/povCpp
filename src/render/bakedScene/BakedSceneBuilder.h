#ifndef __BAKED_SCENE_BUILDER__
#define __BAKED_SCENE_BUILDER__

#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CullSafeEntry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

class BakedSceneBuilder {
  public:
    static void build(const java::ArrayList<SimpleBody*> &objects, BakedScene &out);

  private:
    static bool isBakeableSimpleBody(SimpleBody *object);
    static Matrix4x4d copyOrIdentity(const Matrix4x4d *matrix);
    static BakedSceneTraceKind classifyTraceableObject(
        bool hasGeometry, bool boundedOrClipped, bool hasCsg);
    static BakedSceneCsgOperandKind classifyOperandKind(
        Geometry *geometry,
        int nestedCsgProgramIndex,
        bool hasTransform,
        bool isInfinitePlane,
        Quadric *quadricGeometry);
    static bool hasFiniteInteriorBounds(const CsgOperandRecord *operand);
    static bool areSeparated(
        const AxisAlignedBoundingBox &left, const AxisAlignedBoundingBox &right);
    static bool hasPairwiseDisjointFiniteOperands(
        const java::ArrayList<CsgOperandRecord> &operands);
    static void classifyCsgProgramSpecialization(
        BooleanSetOperations geometryType,
        bool topLevel,
        const java::ArrayList<CsgOperandRecord> &operands,
        BakedSceneCsgPlanKind &planKind,
        bool &specializationValid,
        int &specializationCoreOperandIndex);
    static void buildCsgExecutionPlan(
        BakedSceneCsgAlgorithm algorithm,
        bool specializationValid,
        BakedSceneCsgPlanKind &planKind,
        java::ArrayList<CsgOperandRecord> &operands,
        const BakedScene &out,
        java::ArrayList<int> &planeOperandIndices,
        java::ArrayList<int> &nestedOperandIndices,
        java::ArrayList<int> &transformedPrimitiveOperandIndices,
        java::ArrayList<int> &directPrimitiveOperandIndices);
    static void compileTracingObjects(
        const java::ArrayList<SimpleBody*> &objects,
        BakedScene &out,
        java::ArrayList<int> &result);
    static TraceableObject *bakeSimpleBody(SimpleBody *object, BakedScene &out);
    static bool isPushdownCandidateOperand(const CsgOperandRecord *operand);
    static bool nestedProgramFullyBakeable(
        const BakedScene &scene, int programIndex, int depthGuard);
    static void pushDownStepsIntoProgram(
        BakedScene &out, int programIndex,
        const java::ArrayList<TransformStep> &parentSteps,
        const Matrix4x4d &parentForwardTransform);
    static CsgOperandRecord bakeCsgOperand(CsgOperand *operand, BakedScene &out);

    // "Reconstruct with N fields changed" helpers used by the multi-pass
    // bake pipeline (execution-plan compilation, viewpoint-slot assignment,
    // transform pushdown): build a fresh record value from `base`'s getters
    // plus the given overrides, never mutate `base` itself. Returned by
    // value; callers assign it into the operands array slot
    // (`operands[i] = cloneOperandWithX(...)`), never manually delete it.
    static CsgOperandRecord cloneOperandWithCompiledPlan(
        const CsgOperandRecord *base,
        bool compiledTransformedNestedCorePlane,
        int compiledNestedCoreOperandIndex,
        bool compiledNestedCoreDirectQuadric,
        bool compiledNestedCoreTransformedQuadric,
        const java::ArrayList<int> &compiledNestedPlaneOperandIndices,
        const java::ArrayList<int> &compiledNestedContainmentOperandIndices);
    static CsgOperandRecord cloneOperandWithViewpointSlots(
        const CsgOperandRecord *base, int quadricViewpointSlot, int planeViewpointSlot);
    static CsgOperandRecord cloneOperandWithPushdownBake(
        const CsgOperandRecord *base,
        const AxisAlignedBoundingBox &bakedBounds,
        bool bounded,
        BakedSceneCsgOperandKind kind,
        bool hasTransform,
        const Matrix4x4d &objectToLocal,
        const Matrix4x4d &localToObject,
        bool pushdownFolded,
        const Quadric &bakedQuadricStorage,
        bool hasBakedQuadric,
        const InfinitePlane &bakedPlaneStorage,
        bool hasBakedPlane,
        const Vector3Dd &planeNormal,
        double planeDistance,
        const java::ArrayList<TransformStep> &pushdownAccumulatedSteps);
    static TraceableObject *cloneObjectWithViewpointSlot(
        const TraceableObject *base, int quadricViewpointSlot);
    static CsgProgram *cloneProgramWithReclassification(
        const CsgProgram *base,
        BakedSceneCsgPlanKind planKind,
        bool specializationValid,
        int specializationCoreOperandIndex,
        const java::ArrayList<int> &planeOperandIndices,
        const java::ArrayList<int> &nestedOperandIndices,
        const java::ArrayList<int> &transformedPrimitiveOperandIndices,
        const java::ArrayList<int> &directPrimitiveOperandIndices,
        const java::ArrayList<CsgOperandRecord> &operands);

    static constexpr long int kOperandCullBinThreshold = 17;
    static constexpr bool kEnableOperandCullBins = true;

    // Ascending insertion sort by key. Bake-time only, called once per wide
    // union bucket (never on the per-ray path) - same complexity trade-off
    // already accepted by AabbCullingSupport::sortPositionsDescending for
    // small per-ray arrays, here applied to a larger but still one-shot list.
    static void sortCullSafeEntriesByKey(java::ArrayList<CullSafeEntry> &entries);

    // Bake-time only. Returns nullptr when the bucket is too small to bin
    // (see OperandCullBins - stores bucket *positions*, never operand
    // pointers or global indices, so nothing dangles across later ArrayList
    // relocations).
    static OperandCullBins *buildOperandCullBinsForBucket(
        const java::ArrayList<int> &bucket,
        const java::ArrayList<CsgOperandRecord> &operands);

    static CsgProgram *bakeConstructiveSolidGeometry(
        ConstructiveSolidGeometry *geometry, BakedScene &out);
    static int compileConstructiveSolidGeometry(
        ConstructiveSolidGeometry *geometry, BakedScene &out);
    static CompositeRecord *bakeComposite(Composite *object, BakedScene &out);
    static int compileTracingObject(SimpleBody *object, BakedScene &out);
    static void accumulateStatistics(
        BakedScene &scene, long quadricViewpointSlotCount, long planeViewpointSlotCount);
};

#endif
