#ifndef __SIMPLE_BODY__
#define __SIMPLE_BODY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/material/Material.h"

class SimpleBody : public Geometry {
  private:
    java::ArrayList<TransformedGeometry*> boundingShapes{4};
    java::ArrayList<TransformedGeometry*> clippingShapes{4};
    TransformedGeometry *geometry;
    Material *geometryMaterial;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;

  public:
    SimpleBody(
        TransformedGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<TransformedGeometry*> &boundingShapes,
        const java::ArrayList<TransformedGeometry*> &clippingShapes) :
        boundingShapes(boundingShapes),
        clippingShapes(clippingShapes),
        geometry(geometry),
        geometryMaterial(geometryMaterial),
        noShadowFlag(noShadowFlag),
        objectColor(objectColor),
        objectTexture(objectTexture)
    {
    }
    SimpleBody(const SimpleBody &other);
    ~SimpleBody() override;

    // ObjectParser builds short-lived SimpleBody/Composite wrappers purely to
    // invoke their virtual translate/rotate/scale/invert (which know how to transform
    // a bounded object's children in place), then extracts the resulting
    // geometry/objectTexture/objectColor/bounding-and-clipping-shapes back into local
    // parser variables before discarding the wrapper. detachOwnership() clears every
    // owned field on the wrapper (without deleting them) so that subsequent
    // `delete wrapper;` is a safe no-op for those fields - ownership has already moved
    // to whoever called this. Virtual so Composite::detachOwnership() can also clear
    // its own simpleBodies.
    virtual void detachOwnership();

    java::ArrayList<TransformedGeometry*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<TransformedGeometry*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<TransformedGeometry*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<TransformedGeometry*>& getClippingShapes() const { return clippingShapes; }
    TransformedGeometry *getGeometry() const { return geometry; }
    Material *getGeometryMaterial() const { return geometryMaterial; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    ColorRgba *getObjectColor() const { return objectColor; }
    Material *getObjectTexture() const { return objectTexture; }

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    // Virtual so a nested Composite child reached through a SimpleBody*
    // (e.g. from Composite::translate's simpleBodies loop) dispatches to
    // Composite's override, which propagates the transform into its own
    // simpleBodies. Without this, transforms applied to an outer composite
    // never reach the children of a nested composite.
    virtual void translate(Vector3Dd *vector);
    virtual void rotate(Vector3Dd *vector);
    virtual void scale(Vector3Dd *vector);
    virtual void invert();
};

#endif
