#ifndef __CSG_OPERAND_RECORD__
#define __CSG_OPERAND_RECORD__

#include "render/bakedScene/BakedSceneKinds.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/element/TransformStep.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"

class Material;

// Immutable bake-time record for one CSG operand. Reclassifying/re-baking an
// operand during a later pass (viewpoint slots, transform pushdown, execution
// plan compilation) means constructing a fresh CsgOperandRecord and replacing
// the owning pointer, never mutating this object in place.
class CsgOperandRecord {
  public:
    CsgOperandRecord(
        BakedSceneCsgOperandKind kind,
        CsgOperand *operand,
        Geometry *originalGeometry,
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
        const InfinitePlane &bakedPlaneStorage,
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
    Geometry *getGeometry() const
    {
        if (hasBakedQuadric) {
            return const_cast<Quadric *>(&bakedQuadricStorage);
        }
        if (hasBakedPlane) {
            return const_cast<InfinitePlane *>(&bakedPlaneStorage);
        }
        return originalGeometry;
    }
    Quadric *getQuadricGeometry() const
    {
        return hasBakedQuadric ? const_cast<Quadric *>(&bakedQuadricStorage) : originalQuadricGeometry;
    }
    Geometry *getOriginalGeometry() const { return originalGeometry; }
    Quadric *getOriginalQuadricGeometry() const { return originalQuadricGeometry; }
    const Quadric &getBakedQuadricStorage() const { return bakedQuadricStorage; }
    const InfinitePlane &getBakedPlaneStorage() const { return bakedPlaneStorage; }
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
    Geometry *originalGeometry;
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
    InfinitePlane bakedPlaneStorage;
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
