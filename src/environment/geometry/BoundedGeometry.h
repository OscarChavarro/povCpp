#ifndef __BOUNDED_GEOMETRY__
#define __BOUNDED_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/material/Material.h"

class BoundedGeometry : public Geometry {
  private:
    java::ArrayList<TransformableElement*> boundingShapes{4};
    java::ArrayList<TransformableElement*> clippingShapes{4};
    TransformableElement *geometry;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;

  public:
    BoundedGeometry(
        TransformableElement *geometry = nullptr,
        Material *objectTexture = nullptr,
        ColorRgba *objectColor = nullptr,
        bool noShadowFlag = false) :
        geometry(geometry),
        noShadowFlag(noShadowFlag),
        objectColor(objectColor),
        objectTexture(objectTexture)
    {
    }
    BoundedGeometry(
        TransformableElement *geometry,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<TransformableElement*> &boundingShapes,
        const java::ArrayList<TransformableElement*> &clippingShapes) :
        boundingShapes(boundingShapes),
        clippingShapes(clippingShapes),
        geometry(geometry),
        noShadowFlag(noShadowFlag),
        objectColor(objectColor),
        objectTexture(objectTexture)
    {
    }
    BoundedGeometry(const BoundedGeometry &other);
    ~BoundedGeometry() override;

    // ObjectParser builds short-lived BoundedGeometry/Composite wrappers purely to
    // invoke their virtual translate/rotate/scale/invert (which know how to transform
    // a bounded object's children in place), then extracts the resulting
    // geometry/objectTexture/objectColor/bounding-and-clipping-shapes back into local
    // parser variables before discarding the wrapper. detachOwnership() clears every
    // owned field on the wrapper (without deleting them) so that subsequent
    // `delete wrapper;` is a safe no-op for those fields - ownership has already moved
    // to whoever called this. Virtual so Composite::detachOwnership() can also clear
    // its own simpleBodies.
    virtual void detachOwnership();

    java::ArrayList<TransformableElement*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<TransformableElement*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<TransformableElement*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<TransformableElement*>& getClippingShapes() const { return clippingShapes; }
    TransformableElement *getGeometry() const { return geometry; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    ColorRgba *getObjectColor() const { return objectColor; }
    Material *getObjectTexture() const { return objectTexture; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *material) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
};

#endif
