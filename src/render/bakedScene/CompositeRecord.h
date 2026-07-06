#ifndef __COMPOSITE_RECORD__
#define __COMPOSITE_RECORD__

#include "environment/scene/Composite.h"

class CompositeRecord {
  public:
    CompositeRecord(
        Composite *object,
        const AxisAlignedBoundingBox &worldBounds,
        const Matrix4x4d &objectToWorld,
        const Matrix4x4d &worldToObject,
        bool bounded,
        bool noShadowFlag,
        bool hasObjectTransform,
        bool hasBoundingShapes,
        bool hasClippingShapes,
        const java::ArrayList<int> &boundingObjectIndices,
        const java::ArrayList<int> &clippingObjectIndices,
        const java::ArrayList<int> &childObjectIndices) :
        object(object),
        worldBounds(worldBounds),
        objectToWorld(objectToWorld),
        worldToObject(worldToObject),
        bounded(bounded),
        noShadowFlag(noShadowFlag),
        hasObjectTransform(hasObjectTransform),
        hasBoundingShapes(hasBoundingShapes),
        hasClippingShapes(hasClippingShapes),
        boundingObjectIndices(boundingObjectIndices),
        clippingObjectIndices(clippingObjectIndices),
        childObjectIndices(childObjectIndices)
    {}

    Composite *getObject() const { return object; }
    const AxisAlignedBoundingBox &getWorldBounds() const { return worldBounds; }
    const Matrix4x4d &getObjectToWorld() const { return objectToWorld; }
    const Matrix4x4d &getWorldToObject() const { return worldToObject; }
    bool getBounded() const { return bounded; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    bool getHasObjectTransform() const { return hasObjectTransform; }
    bool getHasBoundingShapes() const { return hasBoundingShapes; }
    bool getHasClippingShapes() const { return hasClippingShapes; }
    const java::ArrayList<int> &getBoundingObjectIndices() const { return boundingObjectIndices; }
    const java::ArrayList<int> &getClippingObjectIndices() const { return clippingObjectIndices; }
    const java::ArrayList<int> &getChildObjectIndices() const { return childObjectIndices; }

  private:
    Composite *object;
    AxisAlignedBoundingBox worldBounds;
    Matrix4x4d objectToWorld;
    Matrix4x4d worldToObject;
    bool bounded;
    bool noShadowFlag;
    bool hasObjectTransform;
    bool hasBoundingShapes;
    bool hasClippingShapes;
    java::ArrayList<int> boundingObjectIndices;
    java::ArrayList<int> clippingObjectIndices;
    java::ArrayList<int> childObjectIndices;
};

#endif
