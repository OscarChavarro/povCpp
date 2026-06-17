#ifndef __BOUNDED_GEOMETRY_H__
#define __BOUNDED_GEOMETRY_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/material/Material.h"

class BoundedGeometry : public Geometry {
  public:
    java::ArrayList<TransformableElement*> boundingShapes{4};
    java::ArrayList<TransformableElement*> clippingShapes{4};
    TransformableElement *geometry;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;

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
    static BoundedGeometry *createBasicObject();
};

#endif
