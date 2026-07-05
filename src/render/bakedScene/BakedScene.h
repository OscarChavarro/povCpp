#ifndef __BAKED_SCENE__
#define __BAKED_SCENE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/element/AxisAlignedBoundingBox.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "environment/material/Material.h"
#include "environment/scene/TransformStep.h"

class SimpleBody;
class Composite;
class CsgOperand;

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

    struct CsgOperandRecord {
        CsgOperandKind kind = CsgOperandKind::Empty;
        CsgOperand *operand = nullptr;
        Geometry *geometry = nullptr;
        Quadric *quadricGeometry = nullptr;
        Material *material = nullptr;
        int nestedCsgProgramIndex = -1;
        AxisAlignedBoundingBox bakedBounds = AxisAlignedBoundingBox::unbounded();
        Matrix4x4d objectToLocal = Matrix4x4d::identityMatrix();
        Matrix4x4d localToObject = Matrix4x4d::identityMatrix();
        bool hasTransform = false;
        bool bounded = false;
        bool cullSafe = false;
        bool isInfinitePlane = false;
        Vector3Dd planeNormal = Vector3Dd(0.0, 1.0, 0.0);
        double planeDistance = 0.0;
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
        java::ArrayList<TransformStep> pushdownAccumulatedSteps;
        bool pushdownFolded = false;
    };

    struct OperandCullBins {
        bool built = false;
        java::ArrayList<AxisAlignedBoundingBox> binBounds;
        java::ArrayList<int> binMemberStart;
        java::ArrayList<int> binMemberCount;
        java::ArrayList<int> binMembers;
        java::ArrayList<int> alwaysTestedPositions;
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
        const OperandCullBins *directPrimitiveCullBins = nullptr;
        const OperandCullBins *transformedPrimitiveCullBins = nullptr;
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
        AxisAlignedBoundingBox worldBounds = AxisAlignedBoundingBox::unbounded();
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
        int quadricViewpointSlot = -1;
    };

    struct CompositeRecord {
        Composite *object = nullptr;
        AxisAlignedBoundingBox worldBounds = AxisAlignedBoundingBox::unbounded();
        Matrix4x4d objectToWorld = Matrix4x4d::identityMatrix();
        Matrix4x4d worldToObject = Matrix4x4d::identityMatrix();
        bool bounded = false;
        bool noShadowFlag = false;
        bool hasObjectTransform = false;
        bool hasBoundingShapes = false;
        bool hasClippingShapes = false;
        java::ArrayList<int> boundingObjectIndices;
        java::ArrayList<int> clippingObjectIndices;
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
        long quadricViewpointSlotCount = 0;
        long planeViewpointSlotCount = 0;
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
        long unionProgramOperandHistogram[4] = {0, 0, 0, 0};
        long unionProgramOperandCullSafeCount = 0;
        long unionProgramOperandTotalCount = 0;
        long topLevelObjectCount = 0;
        long topLevelObjectCullSafeCount = 0;
    };

    BakedScene() = default;
    BakedScene(const BakedScene &) = delete;
    BakedScene &operator=(const BakedScene &) = delete;
    ~BakedScene()
    {
        for (long int i = 0; i < operandCullBinsStorage.size(); i++) {
            delete operandCullBinsStorage[i];
        }
    }

    java::ArrayList<TraceableObject> traceableObjects;
    java::ArrayList<CsgProgram> csgPrograms;
    java::ArrayList<OperandCullBins *> operandCullBinsStorage;
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
