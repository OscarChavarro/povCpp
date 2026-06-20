#ifndef __COMPOSITE__
#define __COMPOSITE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/volume/compound/CSG.h"

class Composite : public BoundedGeometry {
  public:
    Composite(
        TransformableElement *geometry = nullptr,
        Material *objectTexture = nullptr,
        ColorRgba *objectColor = nullptr,
        bool noShadowFlag = false) :
        BoundedGeometry(geometry, objectTexture, objectColor, noShadowFlag)
    {
    }
    Composite(
        TransformableElement *geometry,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<TransformableElement*> &boundingShapes,
        const java::ArrayList<TransformableElement*> &clippingShapes,
        const java::ArrayList<BoundedGeometry*> &simpleBodies) :
        BoundedGeometry(
            geometry, objectTexture, objectColor, noShadowFlag, boundingShapes,
            clippingShapes),
        simpleBodies(simpleBodies)
    {
    }
    Composite(const Composite &other);

    java::ArrayList<BoundedGeometry*> &getSimpleBodies();
    const java::ArrayList<BoundedGeometry*> &getSimpleBodies() const;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  private:
    java::ArrayList<BoundedGeometry*> simpleBodies{4};
};

inline java::ArrayList<BoundedGeometry*> &
Composite::getSimpleBodies()
{
    return simpleBodies;
}

inline const java::ArrayList<BoundedGeometry*> &
Composite::getSimpleBodies() const
{
    return simpleBodies;
}

#endif
