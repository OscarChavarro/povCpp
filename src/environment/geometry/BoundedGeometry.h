#ifndef __BOUNDED_GEOMETRY__
#define __BOUNDED_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/GeometryTypes.h"
#include "environment/material/Material.h"

class BoundedGeometry : public Geometry {
  private:
    java::ArrayList<TransformableElement*> boundingShapes{4};
    java::ArrayList<TransformableElement*> clippingShapes{4};
    TransformableElement *const geometry;
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

    java::ArrayList<TransformableElement*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<TransformableElement*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<TransformableElement*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<TransformableElement*>& getClippingShapes() const { return clippingShapes; }
    TransformableElement *getGeometry() const { return geometry; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    ColorRgba *getObjectColor() const { return objectColor; }
    Material *getObjectTexture() const { return objectTexture; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  protected:
    static BoundedGeometry *createBasicObject(Material *objectTexture);
};

#endif
