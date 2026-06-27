#ifndef __COMPOSITE__
#define __COMPOSITE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/scene/SimpleBody.h"

class Composite : public SimpleBody {
  public:
    Composite(
        TransformedGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<TransformedGeometry*> &boundingShapes,
        const java::ArrayList<TransformedGeometry*> &clippingShapes,
        const java::ArrayList<SimpleBody*> &simpleBodies,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr) :
        SimpleBody(
            geometry, geometryMaterial, objectTexture, objectColor,
            noShadowFlag, boundingShapes, clippingShapes,
            transformation, transformationInverse),
        simpleBodies(simpleBodies)
    {
    }
    Composite(const Composite &other);
    ~Composite() override;
    void detachOwnership() override;

    java::ArrayList<SimpleBody*> &getSimpleBodies();
    const java::ArrayList<SimpleBody*> &getSimpleBodies() const;

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  private:
    java::ArrayList<SimpleBody*> simpleBodies{4};
};

inline java::ArrayList<SimpleBody*> &
Composite::getSimpleBodies()
{
    return simpleBodies;
}

inline const java::ArrayList<SimpleBody*> &
Composite::getSimpleBodies() const
{
    return simpleBodies;
}

#endif
