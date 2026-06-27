#ifndef __COMPOSITE__
#define __COMPOSITE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

class Composite : public BoundedGeometry {
  public:
    Composite(
        SimpleBody *geometry,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes,
        const java::ArrayList<BoundedGeometry*> &simpleBodies) :
        BoundedGeometry(
            geometry, objectTexture, objectColor, noShadowFlag, boundingShapes,
            clippingShapes),
        simpleBodies(simpleBodies)
    {
    }
    Composite(const Composite &other);
    ~Composite() override;
    void detachOwnership() override;

    java::ArrayList<BoundedGeometry*> &getSimpleBodies();
    const java::ArrayList<BoundedGeometry*> &getSimpleBodies() const;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
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
