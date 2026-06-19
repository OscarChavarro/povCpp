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

    java::ArrayList<TransformableElement*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<TransformableElement*>& getBoundingShapes() const { return boundingShapes; }
    void setBoundingShapes(const java::ArrayList<TransformableElement*> &value)
    {
        boundingShapes = value;
    }
    java::ArrayList<TransformableElement*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<TransformableElement*>& getClippingShapes() const { return clippingShapes; }
    void setClippingShapes(const java::ArrayList<TransformableElement*> &value)
    {
        clippingShapes = value;
    }
    TransformableElement *getGeometry() const { return geometry; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    void setNoShadowFlag(bool value) { noShadowFlag = value; }
    ColorRgba *getObjectColor() const { return objectColor; }
    void setObjectColor(ColorRgba *value) { objectColor = value; }
    Material *getObjectTexture() const { return objectTexture; }
    Material *&getObjectTextureRef() { return objectTexture; }
    void setObjectTexture(Material *value) { objectTexture = value; }

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
