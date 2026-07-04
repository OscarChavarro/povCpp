#ifndef __BAKED_SCENE__
#define __BAKED_SCENE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/element/AxisAlignedBox.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "environment/material/Material.h"
#include "environment/scene/TransformStep.h"

class SimpleBody;
class Composite;
class CsgOperand;

// Plan 6: the rebuilt baked-model layer. Unlike the Scene.h Baked* structs
// it supersedes (Plan 6 Phase 4 deletes those), every field here is either
// resolved once at build time or is a cold debug pointer back to the parsed
// model - nothing on this struct is meant to be re-classified per ray.
// Phase 1 only builds this model (BakedSceneBuilder) alongside the old one;
// no trace code reads it yet. Phases 2-3 wire `render/bakedScene`'s trace
// entry points to read from here instead of Scene::CompiledTracingScene.
class BakedScene {
  public:
    enum class TraceKind {
        Empty,
        DirectPrimitive,
        Csg,
        Composite,
        BoundedGeneric,
        GenericFallback,
    };

    enum class CsgAlgorithm {
        MorganRules,
        RaySegments,
    };

    enum class CsgPlanKind {
        GenericMorgan,
        GenericRaySegments,
        TopLevelPlaneUnion,
        DisjointBoundedUnion,
        SingleCorePlaneIntersection,
        Fallback,
    };

    enum class CsgOperandKind {
        Empty,
        DirectAnnotatedPrimitive,
        DirectPrimitive,
        DirectPlane,
        TransformedPlane,
        NestedCsg,
        TransformedNestedCsg,
        TransformedQuadric,
        TransformedSphere,
        TransformedPrimitive,
        GenericFallback,
    };

    // One CSG operand, world/parent-space transform pair plus (if the
    // operand was foldable) an owned world-space coefficient-rewritten
    // copy - carried over unchanged from Plan 5's BakedCsgOperand.
    struct CsgOperandRecord {
        CsgOperandKind kind = CsgOperandKind::Empty;
        // Detail-owner pushed onto the shading stack for per-operand
        // material/texture lookups (CsgOperand::doExtraInformation) - not a
        // cold debug pointer, this is read on the hot path exactly like the
        // old model's BakedCsgOperand::operand.
        CsgOperand *operand = nullptr;
        Geometry *geometry = nullptr;
        Quadric *quadricGeometry = nullptr;
        Material *material = nullptr;
        int nestedCsgProgramIndex = -1;
        AxisAlignedBox bakedBounds = AxisAlignedBox::unbounded();
        Matrix4x4d objectToLocal = Matrix4x4d::identityMatrix();
        Matrix4x4d localToObject = Matrix4x4d::identityMatrix();
        bool hasTransform = false;
        bool bounded = false;
        bool cullSafe = false;
        bool isInfinitePlane = false;
        Vector3Dd planeNormal = Vector3Dd(0.0, 1.0, 0.0);
        double planeDistance = 0.0;
        // Plan 7: slot indices into the task-owned RaySharedCache, assigned
        // once at build time by BakedSceneBuilder - replaces the mutable
        // planeVp*/Quadric::objectVpConstant per-record caches (shared
        // mutable state that raced under -parallel) with a flat array
        // indexed per render task. -1 when the operand kind does not need a
        // viewpoint constant.
        int quadricViewpointSlot = -1;
        int planeViewpointSlot = -1;
        Quadric bakedQuadricStorage;
        bool hasBakedQuadric = false;
        InfinitePlane bakedPlaneStorage;
        bool hasBakedPlane = false;
        bool compiledTransformedNestedCorePlane = false;
        int compiledNestedCoreOperandIndex = -1;
        bool compiledNestedCoreDirectQuadric = false;
        bool compiledNestedCoreTransformedQuadric = false;
        java::ArrayList<int> compiledNestedPlaneOperandIndices;
        java::ArrayList<int> compiledNestedContainmentOperandIndices;
        // Plan 8 Phase R2: the full elementary-step list already baked into
        // bakedQuadricStorage/bakedPlaneStorage so far, kept so a SECOND
        // (outer-level) push-down pass reached through recursion can extend
        // it instead of re-deriving from `operand->getSteps()` alone - which
        // would silently forget whatever an earlier, deeper push-down pass
        // already folded in. Empty until the first push-down collapse.
        java::ArrayList<TransformStep> pushdownAccumulatedSteps;
        // Plan 8 Phase R2: true on a nested-CSG wrapper whose transform was
        // pushed down into its program (kind demoted TransformedNestedCsg ->
        // NestedCsg, matrices reset to identity). Lets buildCsgExecutionPlan
        // keep compiling the single-core-plane fast path for it - without
        // this the push-down silently trades the compiled kernel (trueMiss
        // early-out, no clone, no scratch queue) for the generic nested
        // recursion, which is what made the collapse performance-neutral.
        // Deliberately NOT set on wrappers that were never transformed, so
        // scenes push-down never touched keep their exact byte behavior.
        bool pushdownFolded = false;
    };

    // One compiled CSG node: algorithm/specialization chosen once at build
    // time, plus the operand-kind bucket arrays the fused-plan kernels
    // iterate (planes, direct primitives, nested CSGs, transformed
    // primitives) instead of re-deriving them per ray.
    struct CsgProgram {
        CsgAlgorithm algorithm = CsgAlgorithm::MorganRules;
        CsgPlanKind planKind = CsgPlanKind::Fallback;
        BooleanSetOperations geometryType = BooleanSetOperations::UNION;
        bool topLevel = false;
        bool specializationValid = false;
        int specializationCoreOperandIndex = -1;
        java::ArrayList<int> planeOperandIndices;
        java::ArrayList<int> nestedOperandIndices;
        java::ArrayList<int> transformedPrimitiveOperandIndices;
        java::ArrayList<int> directPrimitiveOperandIndices;
        java::ArrayList<CsgOperandRecord> operands;
    };

    // One traceable object: a plain SimpleBody, a CSG-wrapping SimpleBody,
    // or a Composite (in which case `compositeIndex` points at the extra
    // per-composite data in `composites` - bounding/clipping/composite kind
    // is folded into `kind` at build time, not re-derived per ray).
    struct TraceableObject {
        TraceKind kind = TraceKind::Empty;
        SimpleBody *object = nullptr;
        Geometry *geometry = nullptr;
        Quadric *quadricGeometry = nullptr;
        Material *geometryMaterial = nullptr;
        Material *objectTexture = nullptr;
        ColorRgba *objectColor = nullptr;
        AxisAlignedBox worldBounds = AxisAlignedBox::unbounded();
        bool bounded = false;
        bool castsShadow = true;
        bool noShadowFlag = false;
        int csgProgramIndex = -1;
        int compositeIndex = -1;
        Matrix4x4d objectToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToObject = Matrix4x4d::identityMatrix();
        Matrix4x4d geometryToObject = Matrix4x4d::identityMatrix();
        Matrix4x4d objectToGeometry = Matrix4x4d::identityMatrix();
        Matrix4x4d geometryToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToGeometry = Matrix4x4d::identityMatrix();
        bool hasObjectTransform = false;
        bool hasGeometryTransform = false;
        bool hasBoundingShapes = false;
        bool hasClippingShapes = false;
        java::ArrayList<int> boundingObjectIndices;
        java::ArrayList<int> clippingObjectIndices;
        Quadric bakedQuadricStorage;
        bool hasBakedQuadric = false;
        InfinitePlane bakedPlaneStorage;
        bool hasBakedPlane = false;
        // Plan 7: slot index into the task-owned RaySharedCache for the
        // direct-quadric fast path in BakedTrace::traceSimpleBodyAllCrossings.
        int quadricViewpointSlot = -1;
    };

    struct CompositeRecord {
        Composite *object = nullptr;
        AxisAlignedBox worldBounds = AxisAlignedBox::unbounded();
        Matrix4x4d objectToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToObject = Matrix4x4d::identityMatrix();
        bool bounded = false;
        bool noShadowFlag = false;
        bool hasObjectTransform = false;
        bool hasBoundingShapes = false;
        bool hasClippingShapes = false;
        java::ArrayList<int> boundingObjectIndices;
        java::ArrayList<int> clippingObjectIndices;
        // Traversal order preserved verbatim from the parsed Composite's
        // simple-body list - see doc/performanceReviewPlan6.md Phase 0
        // item 4 (equal-depth ordering is load-bearing for some scenes).
        java::ArrayList<int> childObjectIndices;
    };

    struct Statistics {
        long countByKind[6] = {0, 0, 0, 0, 0, 0};
        long csgProgramCount = 0;
        long csgPlanTopLevelPlaneUnion = 0;
        long csgPlanDisjointBoundedUnion = 0;
        long csgPlanSingleCorePlaneIntersection = 0;
        long csgPlanGenericMorgan = 0;
        long csgPlanGenericRaySegments = 0;
        long csgPlanFallback = 0;
        long residualBakedQuadricOperands = 0;
        long residualBakedPlaneOperands = 0;
        long residualTransformedOperands = 0;
        // Plan 7: sizes for RaySharedCache::ensureCapacity - one slot per
        // quadric/plane-bearing record assigned during build().
        long quadricViewpointSlotCount = 0;
        long planeViewpointSlotCount = 0;
        // Plan 8 Phase R0: split residualTransformedOperands by the three
        // root causes identified in doc/performanceReviewPlan8.md.
        // Category 1: TransformedNestedCsg - never collapsible today.
        long residualCategory1NestedCsg = 0;
        // ...of which the nested program's operands are ALL themselves
        // bakeable kinds (quadric/plane, recursively bakeable nested) -
        // the Phase R2 push-down candidates.
        long residualCategory1PushdownEligible = 0;
        // Category 2: TransformedQuadric/TransformedPlane whose source
        // CsgOperand has an empty step list (parser/instantiation gap).
        long residualCategory2EmptySteps = 0;
        // Category 3: TransformedSphere/TransformedPrimitive - no
        // coefficient congruence exists for these kinds today.
        long residualCategory3Unbakeable = 0;
    };

    java::ArrayList<TraceableObject> traceableObjects;
    java::ArrayList<CsgProgram> csgPrograms;
    java::ArrayList<CompositeRecord> composites;
    java::ArrayList<int> topLevelObjectIndices;
    java::ArrayList<int> boundedObjectIndices;
    java::ArrayList<int> unboundedObjectIndices;
    java::ArrayList<int> shadowCastingObjectIndices;
    java::ArrayList<int> boundedShadowCastingObjectIndices;
    java::ArrayList<int> unboundedShadowCastingObjectIndices;
    Statistics statistics;
};

#endif
