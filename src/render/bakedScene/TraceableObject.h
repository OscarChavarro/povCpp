#ifndef __TRACEABLE_OBJECT__
#define __TRACEABLE_OBJECT__

#include "render/bakedScene/BakedSceneKinds.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/SimpleBody.h"

// One traceable object: a plain SimpleBody, a CSG-wrapping SimpleBody,
// or a Composite (in which case `compositeIndex` points at the extra
// per-composite data in `BakedScene::composites` - bounding/clipping/
// composite kind is folded into `kind` at build time, not re-derived per
// ray). Immutable: a later bake pass (viewpoint slots) reclassifies by
// constructing a fresh TraceableObject and replacing the owning pointer.
class TraceableObject {
  public:
    TraceableObject(
        BakedSceneTraceKind kind,
        SimpleBody *object,
        Geometry *originalGeometry,
        Quadric *originalQuadricGeometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        const AxisAlignedBoundingBox &worldBounds,
        bool bounded,
        bool castsShadow,
        bool noShadowFlag,
        int csgProgramIndex,
        int compositeIndex,
        const Matrix4x4d &objectToWorld,
        const Matrix4x4d &worldToObject,
        const Matrix4x4d &geometryToObject,
        const Matrix4x4d &objectToGeometry,
        const Matrix4x4d &geometryToWorld,
        const Matrix4x4d &worldToGeometry,
        bool hasObjectTransform,
        bool hasGeometryTransform,
        bool hasBoundingShapes,
        bool hasClippingShapes,
        const java::ArrayList<int> &boundingObjectIndices,
        const java::ArrayList<int> &clippingObjectIndices,
        const Quadric &bakedQuadricStorage,
        bool hasBakedQuadric,
        const InfinitePlane &bakedPlaneStorage,
        bool hasBakedPlane,
        int quadricViewpointSlot) :
        kind(kind),
        object(object),
        originalGeometry(originalGeometry),
        originalQuadricGeometry(originalQuadricGeometry),
        geometryMaterial(geometryMaterial),
        objectTexture(objectTexture),
        objectColor(objectColor),
        worldBounds(worldBounds),
        bounded(bounded),
        castsShadow(castsShadow),
        noShadowFlag(noShadowFlag),
        csgProgramIndex(csgProgramIndex),
        compositeIndex(compositeIndex),
        objectToWorld(objectToWorld),
        worldToObject(worldToObject),
        geometryToObject(geometryToObject),
        objectToGeometry(objectToGeometry),
        geometryToWorld(geometryToWorld),
        worldToGeometry(worldToGeometry),
        hasObjectTransform(hasObjectTransform),
        hasGeometryTransform(hasGeometryTransform),
        hasBoundingShapes(hasBoundingShapes),
        hasClippingShapes(hasClippingShapes),
        boundingObjectIndices(boundingObjectIndices),
        clippingObjectIndices(clippingObjectIndices),
        bakedQuadricStorage(bakedQuadricStorage),
        hasBakedQuadric(hasBakedQuadric),
        bakedPlaneStorage(bakedPlaneStorage),
        hasBakedPlane(hasBakedPlane),
        quadricViewpointSlot(quadricViewpointSlot)
    {}

    BakedSceneTraceKind getKind() const { return kind; }
    SimpleBody *getObject() const { return object; }
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
    Material *getGeometryMaterial() const { return geometryMaterial; }
    Material *getObjectTexture() const { return objectTexture; }
    ColorRgba *getObjectColor() const { return objectColor; }
    const AxisAlignedBoundingBox &getWorldBounds() const { return worldBounds; }
    bool getBounded() const { return bounded; }
    bool getCastsShadow() const { return castsShadow; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    int getCsgProgramIndex() const { return csgProgramIndex; }
    int getCompositeIndex() const { return compositeIndex; }
    const Matrix4x4d &getObjectToWorld() const { return objectToWorld; }
    const Matrix4x4d &getWorldToObject() const { return worldToObject; }
    const Matrix4x4d &getGeometryToObject() const { return geometryToObject; }
    const Matrix4x4d &getObjectToGeometry() const { return objectToGeometry; }
    const Matrix4x4d &getGeometryToWorld() const { return geometryToWorld; }
    const Matrix4x4d &getWorldToGeometry() const { return worldToGeometry; }
    bool getHasObjectTransform() const { return hasObjectTransform; }
    bool getHasGeometryTransform() const { return hasGeometryTransform; }
    bool getHasBoundingShapes() const { return hasBoundingShapes; }
    bool getHasClippingShapes() const { return hasClippingShapes; }
    const java::ArrayList<int> &getBoundingObjectIndices() const { return boundingObjectIndices; }
    const java::ArrayList<int> &getClippingObjectIndices() const { return clippingObjectIndices; }
    bool getHasBakedQuadric() const { return hasBakedQuadric; }
    bool getHasBakedPlane() const { return hasBakedPlane; }
    int getQuadricViewpointSlot() const { return quadricViewpointSlot; }

  private:
    BakedSceneTraceKind kind;
    SimpleBody *object;
    Geometry *originalGeometry;
    Quadric *originalQuadricGeometry;
    Material *geometryMaterial;
    Material *objectTexture;
    ColorRgba *objectColor;
    AxisAlignedBoundingBox worldBounds;
    bool bounded;
    bool castsShadow;
    bool noShadowFlag;
    int csgProgramIndex;
    int compositeIndex;
    Matrix4x4d objectToWorld;
    Matrix4x4d worldToObject;
    Matrix4x4d geometryToObject;
    Matrix4x4d objectToGeometry;
    Matrix4x4d geometryToWorld;
    Matrix4x4d worldToGeometry;
    bool hasObjectTransform;
    bool hasGeometryTransform;
    bool hasBoundingShapes;
    bool hasClippingShapes;
    java::ArrayList<int> boundingObjectIndices;
    java::ArrayList<int> clippingObjectIndices;
    Quadric bakedQuadricStorage;
    bool hasBakedQuadric;
    InfinitePlane bakedPlaneStorage;
    bool hasBakedPlane;
    int quadricViewpointSlot;
};

#endif
