#ifndef __SIMPLE_BODY_H__
#define __SIMPLE_BODY_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/elements/GeometryTypes.h"
#include "environment/material/Material.h"

class SimpleBody : public Geometry {
  public:
    Geometry *boundingShapes;
    Geometry *clippingShapes;
    Geometry *geometry;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  protected:
    static SimpleBody *createBasicObject();
    static inline void linkSimpleBody(
        Geometry *newObject, Geometry **field, Geometry **oldObjectList);
    static inline void linkSimpleBody(
        SimpleBody *newObject, Geometry **field, SimpleBody **oldObjectList);
};

#endif
