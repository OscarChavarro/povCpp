#ifndef __CSG_OPERAND_RECORD__
#define __CSG_OPERAND_RECORD__

#include "render/bakedScene/BakedSceneKinds.h"
#include "render/bakedScene/BakedSurfaceTrace.h"
#include "environment/geometry/surface/RayCastingInfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/element/TransformStep.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "vsdk/toolkit/environment/material/Material.h"

// Bake-time record for one CSG operand, stored **by value** in a contiguous
// java::ArrayList<CsgOperandRecord> (CsgProgram::operands) rather than behind
// individually heap-allocated pointers - the per-ray CSG trace kernels
// (CsgMorganUnionTrace, SingleCorePlaneCsgTrace, CsgOperandTrace,
// BakedPlaneIntersector, CsgContainmentTest, AabbCullingSupport) index that
// array on every ray/shadow test, and a heap-scattered layout there costs a
// dependent pointer-chase load per field access (measured: same instruction
// and test counts, but +8% cycles and IPC 2.38->2.18 on drums.pov). A default
// constructor is required so java::ArrayList<T>'s `new T[maxSize]` backing
// store can allocate elements.
//
// Reclassifying/re-baking an operand during a later pass (viewpoint slots,
// transform pushdown, execution plan compilation) means constructing a fresh
// CsgOperandRecord value from the old one's getters and assigning it in place
// (`operands[i] = cloneOperandWithX(...)`), never mutating fields of the
// existing object directly.
class CsgOperandRecord {
  public:
    CsgOperandRecord() :
        kind(BakedSceneCsgOperandKind::Empty),
        operand(nullptr),
        originalGeometry(nullptr),
        detailSurface(nullptr),
        detailFamily(SurfaceDetailFamily::None),
        traceState(nullptr),
        intrinsicSphere(nullptr),
        originalQuadricGeometry(nullptr),
        material(nullptr),
        nestedCsgProgramIndex(-1),
        hasTransform(false),
        bounded(false),
        cullSafe(false),
        isInfinitePlane(false),
        planeDistance(0.0),
        quadricViewpointSlot(-1),
        planeViewpointSlot(-1),
        hasBakedQuadric(false),
        hasBakedPlane(false),
        compiledTransformedNestedCorePlane(false),
        compiledNestedCoreOperandIndex(-1),
        compiledNestedCoreDirectQuadric(false),
        compiledNestedCoreTransformedQuadric(false),
        pushdownFolded(false)
    {}

    CsgOperandRecord(
        BakedSceneCsgOperandKind kind,
        CsgOperand *operand,
        RayCastingGeometry *originalGeometry,
        Quadric *originalQuadricGeometry,
        Material *material,
        int nestedCsgProgramIndex,
        const AxisAlignedBoundingBox &bakedBounds,
        const Matrix4x4d &objectToLocal,
        const Matrix4x4d &localToObject,
        bool hasTransform,
        bool bounded,
        bool cullSafe,
        bool isInfinitePlane,
        const Vector3Dd &planeNormal,
        double planeDistance,
        int quadricViewpointSlot,
        int planeViewpointSlot,
        const Quadric &bakedQuadricStorage,
        bool hasBakedQuadric,
        const RayCastingInfinitePlane &bakedPlaneStorage,
        bool hasBakedPlane,
        bool compiledTransformedNestedCorePlane,
        int compiledNestedCoreOperandIndex,
        bool compiledNestedCoreDirectQuadric,
        bool compiledNestedCoreTransformedQuadric,
        const java::ArrayList<int> &compiledNestedPlaneOperandIndices,
        const java::ArrayList<int> &compiledNestedContainmentOperandIndices,
        const java::ArrayList<TransformStep> &pushdownAccumulatedSteps,
        bool pushdownFolded) :
        kind(kind),
        operand(operand),
        originalGeometry(originalGeometry),
        detailSurface(originalGeometry != nullptr ? originalGeometry->getDetailSurface() : nullptr),
        detailFamily(originalGeometry != nullptr ? originalGeometry->getDetailFamily() :
            SurfaceDetailFamily::None),
        traceState(originalGeometry != nullptr ? originalGeometry->getTraceState() : nullptr),
        intrinsicSphere(originalGeometry != nullptr ?
            dynamic_cast<const Sphere *>(originalGeometry->getIntrinsicGeometry()) : nullptr),
        originalQuadricGeometry(originalQuadricGeometry),
        material(material),
        nestedCsgProgramIndex(nestedCsgProgramIndex),
        bakedBounds(bakedBounds),
        objectToLocal(objectToLocal),
        localToObject(localToObject),
        hasTransform(hasTransform),
        bounded(bounded),
        cullSafe(cullSafe),
        isInfinitePlane(isInfinitePlane),
        planeNormal(planeNormal),
        planeDistance(planeDistance),
        quadricViewpointSlot(quadricViewpointSlot),
        planeViewpointSlot(planeViewpointSlot),
        bakedQuadricStorage(bakedQuadricStorage),
        hasBakedQuadric(hasBakedQuadric),
        bakedPlaneStorage(bakedPlaneStorage),
        hasBakedPlane(hasBakedPlane),
        compiledTransformedNestedCorePlane(compiledTransformedNestedCorePlane),
        compiledNestedCoreOperandIndex(compiledNestedCoreOperandIndex),
        compiledNestedCoreDirectQuadric(compiledNestedCoreDirectQuadric),
        compiledNestedCoreTransformedQuadric(compiledNestedCoreTransformedQuadric),
        compiledNestedPlaneOperandIndices(compiledNestedPlaneOperandIndices),
        compiledNestedContainmentOperandIndices(compiledNestedContainmentOperandIndices),
        pushdownAccumulatedSteps(pushdownAccumulatedSteps),
        pushdownFolded(pushdownFolded)
    {}

    BakedSceneCsgOperandKind getKind() const { return kind; }
    CsgOperand *getOperand() const { return operand; }
    RayCastingGeometry *getGeometry() const
    {
        if (hasBakedQuadric) {
            return const_cast<Quadric *>(&bakedQuadricStorage);
        }
        if (hasBakedPlane) {
            return const_cast<RayCastingInfinitePlane *>(&bakedPlaneStorage);
        }
        return originalGeometry;
    }
    Quadric *getQuadricGeometry() const
    {
        return hasBakedQuadric ? const_cast<Quadric *>(&bakedQuadricStorage) : originalQuadricGeometry;
    }
    Geometry *getDetailSurface() const
    {
        if (hasBakedQuadric) {
            return const_cast<Quadric &>(bakedQuadricStorage).getDetailSurface();
        }
        if (hasBakedPlane) {
            return const_cast<RayCastingInfinitePlane &>(bakedPlaneStorage).getDetailSurface();
        }
        return detailSurface;
    }
    SurfaceDetailFamily getDetailFamily() const
    {
        if (hasBakedQuadric) {
            return SurfaceDetailFamily::Quadric;
        }
        if (hasBakedPlane) {
            return SurfaceDetailFamily::Plane;
        }
        return detailFamily;
    }
    SurfaceTraceState *getTraceState() const
    {
        if (hasBakedQuadric) {
            return const_cast<Quadric &>(bakedQuadricStorage).getTraceState();
        }
        if (hasBakedPlane) {
            return const_cast<RayCastingInfinitePlane &>(bakedPlaneStorage).getTraceState();
        }
        return traceState;
    }
    int emitCrossings(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue, Material *materialOverride) const
    {
        return BakedSurfaceTrace::emitCrossings(getDetailSurface(), getDetailFamily(),
            getTraceState(), getGeometry(), ray, depthQueue, materialOverride);
    }
    int emitCrossingsAnnotated(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) const
    {
        return BakedSurfaceTrace::emitCrossingsAnnotated(getDetailSurface(), getDetailFamily(),
            getTraceState(), getGeometry(), ray, depthQueue, context);
    }
    bool firstHit(RayWithTracingState *ray, IntersectionCandidate &out,
        Material *materialOverride) const
    {
        return BakedSurfaceTrace::firstHit(getDetailSurface(), getDetailFamily(),
            getTraceState(), getGeometry(), ray, out, materialOverride);
    }
    int containment(const Vector3Dd &point, double distanceTolerance) const
    {
        return BakedSurfaceTrace::containment(getDetailSurface(), getDetailFamily(),
            getTraceState(), getGeometry(), point, distanceTolerance);
    }
    RayCastingGeometry *getOriginalGeometry() const { return originalGeometry; }
    const Sphere *getIntrinsicSphere() const { return intrinsicSphere; }
    Quadric *getOriginalQuadricGeometry() const { return originalQuadricGeometry; }
    const Quadric &getBakedQuadricStorage() const { return bakedQuadricStorage; }
    const RayCastingInfinitePlane &getBakedPlaneStorage() const { return bakedPlaneStorage; }
    Material *getMaterial() const { return material; }
    int getNestedCsgProgramIndex() const { return nestedCsgProgramIndex; }
    const AxisAlignedBoundingBox &getBakedBounds() const { return bakedBounds; }
    const Matrix4x4d &getObjectToLocal() const { return objectToLocal; }
    const Matrix4x4d &getLocalToObject() const { return localToObject; }
    bool getHasTransform() const { return hasTransform; }
    bool getBounded() const { return bounded; }
    bool getCullSafe() const { return cullSafe; }
    bool getIsInfinitePlane() const { return isInfinitePlane; }
    const Vector3Dd &getPlaneNormal() const { return planeNormal; }
    double getPlaneDistance() const { return planeDistance; }
    int getQuadricViewpointSlot() const { return quadricViewpointSlot; }
    int getPlaneViewpointSlot() const { return planeViewpointSlot; }
    bool getHasBakedQuadric() const { return hasBakedQuadric; }
    bool getHasBakedPlane() const { return hasBakedPlane; }
    bool getCompiledTransformedNestedCorePlane() const { return compiledTransformedNestedCorePlane; }
    int getCompiledNestedCoreOperandIndex() const { return compiledNestedCoreOperandIndex; }
    bool getCompiledNestedCoreDirectQuadric() const { return compiledNestedCoreDirectQuadric; }
    bool getCompiledNestedCoreTransformedQuadric() const { return compiledNestedCoreTransformedQuadric; }
    const java::ArrayList<int> &getCompiledNestedPlaneOperandIndices() const
    {
        return compiledNestedPlaneOperandIndices;
    }
    const java::ArrayList<int> &getCompiledNestedContainmentOperandIndices() const
    {
        return compiledNestedContainmentOperandIndices;
    }
    const java::ArrayList<TransformStep> &getPushdownAccumulatedSteps() const
    {
        return pushdownAccumulatedSteps;
    }
    bool getPushdownFolded() const { return pushdownFolded; }

  private:
    BakedSceneCsgOperandKind kind;
    CsgOperand *operand;
    RayCastingGeometry *originalGeometry;
    Geometry *detailSurface;
    SurfaceDetailFamily detailFamily;
    SurfaceTraceState *traceState;
    const Sphere *intrinsicSphere;
    Quadric *originalQuadricGeometry;
    Material *material;
    int nestedCsgProgramIndex;
    AxisAlignedBoundingBox bakedBounds;
    Matrix4x4d objectToLocal;
    Matrix4x4d localToObject;
    bool hasTransform;
    bool bounded;
    bool cullSafe;
    bool isInfinitePlane;
    Vector3Dd planeNormal;
    double planeDistance;
    int quadricViewpointSlot;
    int planeViewpointSlot;
    Quadric bakedQuadricStorage;
    bool hasBakedQuadric;
    RayCastingInfinitePlane bakedPlaneStorage;
    bool hasBakedPlane;
    bool compiledTransformedNestedCorePlane;
    int compiledNestedCoreOperandIndex;
    bool compiledNestedCoreDirectQuadric;
    bool compiledNestedCoreTransformedQuadric;
    java::ArrayList<int> compiledNestedPlaneOperandIndices;
    java::ArrayList<int> compiledNestedContainmentOperandIndices;
    java::ArrayList<TransformStep> pushdownAccumulatedSteps;
    bool pushdownFolded;
};

#endif
