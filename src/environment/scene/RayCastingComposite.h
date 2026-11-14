#ifndef __COMPOSITE__
#define __COMPOSITE__

#include "environment/scene/PovrayBody.h"

class RayCastingComposite : public PovrayBody {
  public:
    RayCastingComposite(
        RayCastingGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<PovrayBody*> &boundingShapes,
        const java::ArrayList<PovrayBody*> &clippingShapes,
        const java::ArrayList<PovrayBody*> &simpleBodies,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr,
        PovrayBody::MaterialReleaser objectTextureReleaser = nullptr) :
        PovrayBody(
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

    java::ArrayList<PovrayBody*> &getSimpleBodies();
    const java::ArrayList<PovrayBody*> &getSimpleBodies() const;

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
    java::ArrayList<PovrayBody*> simpleBodies{4};

  protected:
    void propagateOwnedTranslation(Vector3Dd *vector) override;
    void propagateOwnedRotation(Vector3Dd *vector) override;
    void propagateOwnedScale(Vector3Dd *vector) override;
};

inline java::ArrayList<PovrayBody*> &
RayCastingComposite::getSimpleBodies()
{
    return simpleBodies;
}

inline const java::ArrayList<PovrayBody*> &
RayCastingComposite::getSimpleBodies() const
{
    return simpleBodies;
}

#endif
