#ifndef __COMPOSITE__
#define __COMPOSITE__

#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"

class RayCastingComposite : public RayCastingBody {
  public:
    RayCastingComposite(
        RayCastingGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<RayCastingBody*> &boundingShapes,
        const java::ArrayList<RayCastingBody*> &clippingShapes,
        const java::ArrayList<RayCastingBody*> &simpleBodies,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr,
        RayCastingBody::MaterialReleaser objectTextureReleaser = nullptr) :
        RayCastingBody(
            geometry, geometryMaterial, objectTexture, objectColor,
            noShadowFlag, boundingShapes, clippingShapes,
            transformation, transformationInverse,
            geometryTransformation, geometryTransformationInverse,
            java::ArrayList<TransformStep>(), java::ArrayList<TransformStep>(),
            objectTextureReleaser),
        simpleBodies(simpleBodies)
    {
    }
    RayCastingComposite(const RayCastingComposite &other);
    ~RayCastingComposite() override;
    void getWorldBounds(GeometryBounds& result) const override;
    void detachOwnership() override;

    java::ArrayList<RayCastingBody*> &getSimpleBodies();
    const java::ArrayList<RayCastingBody*> &getSimpleBodies() const;

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  private:
    java::ArrayList<RayCastingBody*> simpleBodies{4};

  protected:
    void propagateOwnedTranslation(Vector3Dd *vector) override;
    void propagateOwnedRotation(Vector3Dd *vector) override;
    void propagateOwnedScale(Vector3Dd *vector) override;
};

inline java::ArrayList<RayCastingBody*> &
RayCastingComposite::getSimpleBodies()
{
    return simpleBodies;
}

inline const java::ArrayList<RayCastingBody*> &
RayCastingComposite::getSimpleBodies() const
{
    return simpleBodies;
}

#endif
