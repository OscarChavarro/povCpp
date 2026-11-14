#ifndef __CSG_OPERAND_RECORD__
#define __CSG_OPERAND_RECORD__

#include "render/bakedScene/BakedSceneKinds.h"
#include "render/bakedScene/BakedSurfaceIntersection.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "environment/intersection/surface/QuadricIntersector.h"
#include "environment/intersection/ray/TransformStep.h"
#include "environment/scene/constructiveSolidGeometry/CsgOperand.h"
#include "vsdk/toolkit/environment/material/Material.h"

// Bake-time record for one CSG operand, stored **by value** in a contiguous
// java::ArrayList<CsgOperandRecord> (CsgProgram::operands) rather than behind
// individually heap-allocated pointers - the per-ray CSG trace routines
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
        csg(nullptr),
        selfTraced(false),
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
        bakedQuadricStorage(Vector3Dd(1.0, 1.0, 1.0), Vector3Dd(0.0, 0.0, 0.0),
            Vector3Dd(0.0, 0.0, 0.0), 1.0),
        hasBakedQuadric(false),
        bakedPlaneStorage(0.0, 1.0, 0.0, 0.0),
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
        QuadricSurface *originalQuadricGeometry,
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
        const QuadricSurface &bakedQuadricStorage,
        bool hasBakedQuadric,
        const PlanarHalfSpace &bakedPlaneStorage,
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
        csg(operand->getCsg()),
        selfTraced(operand->getCsg() != nullptr || operand->getLight() != nullptr),
        detailSurface(operand->getSurface()),
        detailFamily(operand->getSurfaceFamily()),
        traceState(operand->getSurfaceIntersectionState()),
        intrinsicSphere(dynamic_cast<const Sphere *>(operand->getSurface())),
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
    {
        if (hasBakedQuadric) {
            bakedStorageState.setNonZeroSquareTerm(
                QuadricIntersector::hasNonZeroSquareTerm(this->bakedQuadricStorage));
        }
        quadricSquareTerm = getIntersectionState() != nullptr && getIntersectionState()->hasNonZeroSquareTerm();
    }

    BakedSceneCsgOperandKind getKind() const { return kind; }
    CsgOperand *getOperand() const { return operand; }
    // Whether the record traces a shape: a family surface, a POV CSG or light
    // geometry.
    bool hasGeometry() const { return selfTraced || detailSurface != nullptr; }
    bool getQuadricSquareTerm() const { return quadricSquareTerm; }
    QuadricSurface *getQuadricGeometry() const
    {
        return hasBakedQuadric ? const_cast<QuadricSurface *>(&bakedQuadricStorage) : originalQuadricGeometry;
    }
    Geometry *getDetailSurface() const
    {
        if (hasBakedQuadric) {
            return const_cast<QuadricSurface *>(&bakedQuadricStorage);
        }
        if (hasBakedPlane) {
            return const_cast<PlanarHalfSpace *>(&bakedPlaneStorage);
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
    SurfaceIntersectionState *getIntersectionState() const
    {
        if (hasBakedQuadric || hasBakedPlane) {
            return const_cast<SurfaceIntersectionState *>(&bakedStorageState);
        }
        return traceState;
    }
    int emitCrossings(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue, Material *materialOverride) const
    {
        return BakedSurfaceIntersection::emitCrossings(getDetailSurface(), getDetailFamily(),
            getIntersectionState(), csg, ray, depthQueue, materialOverride);
    }
    int emitCrossingsAnnotated(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) const
    {
        return BakedSurfaceIntersection::emitCrossingsAnnotated(getDetailSurface(), getDetailFamily(),
            getIntersectionState(), csg, ray, depthQueue, context);
    }
    bool firstHit(RayWithTracingState *ray, IntersectionCandidate &out,
        Material *materialOverride) const
    {
        return BakedSurfaceIntersection::firstHit(getDetailSurface(), getDetailFamily(),
            getIntersectionState(), csg, ray, out, materialOverride);
    }
    int containment(const Vector3Dd &point, double distanceTolerance) const
    {
        return BakedSurfaceIntersection::containment(getDetailSurface(), getDetailFamily(),
            getIntersectionState(), csg, point, distanceTolerance);
    }
    const Sphere *getIntrinsicSphere() const { return intrinsicSphere; }
    QuadricSurface *getOriginalQuadricGeometry() const { return originalQuadricGeometry; }
    const QuadricSurface &getBakedQuadricStorage() const { return bakedQuadricStorage; }
    const PlanarHalfSpace &getBakedPlaneStorage() const { return bakedPlaneStorage; }
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
    ConstructiveSolidGeometry *csg;
    bool selfTraced;
    Geometry *detailSurface;
    SurfaceDetailFamily detailFamily;
    SurfaceIntersectionState *traceState;
    // Trace state of the baked quadric or plane storage, owned by the record.
    SurfaceIntersectionState bakedStorageState;
    const Sphere *intrinsicSphere;
    QuadricSurface *originalQuadricGeometry;
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
    QuadricSurface bakedQuadricStorage;
    // Whether the traced quadric has a nonzero square or mixed coefficient.
    bool quadricSquareTerm = false;
    bool hasBakedQuadric;
    PlanarHalfSpace bakedPlaneStorage;
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
