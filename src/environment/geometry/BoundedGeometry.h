#ifndef __BOUNDED_GEOMETRY__
#define __BOUNDED_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/material/Material.h"
#include "environment/scene/SimpleBody.h"

class BoundedGeometry : public Geometry {
  private:
    java::ArrayList<SimpleBody*> boundingShapes{4};
    java::ArrayList<SimpleBody*> clippingShapes{4};
    SimpleBody *geometry;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;

  public:
    BoundedGeometry(
        SimpleBody *geometry,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes) :
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

    java::ArrayList<SimpleBody*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<SimpleBody*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<SimpleBody*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<SimpleBody*>& getClippingShapes() const { return clippingShapes; }
    SimpleBody *getGeometry() const { return geometry; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    ColorRgba *getObjectColor() const { return objectColor; }
    Material *getObjectTexture() const { return objectTexture; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *material) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    // Virtual so a nested Composite child reached through a BoundedGeometry*
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
