#ifndef __COMPOSITE_H__
#define __COMPOSITE_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/element/GeometryTypes.h"
#include "environment/geometry/volume/compound/CSG.h"

class Composite : public BoundedGeometry {
  public:
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
