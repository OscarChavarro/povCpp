#ifndef __POV_SIMPLE_BODY__
#define __POV_SIMPLE_BODY__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/PostRayHitElement.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/TransformStep.h"

class Sphere;

class RayCastingBody : public SimpleBody, public PostRayHitElement {
  public:
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray,
        const RayCrossingQuery& query, RayCrossingSink& sink) const override;
    // Callers that hand RayCastingBody an objectTexture whose destruction needs
    // more than a plain `delete` (e.g. POV-Ray's shared default-texture
    // aliasing) supply the release logic themselves at construction time, so
    // the scene layer never needs to know the concrete Material subtype.
    using MaterialReleaser = void (*)(Material *);

  protected:
    GeometryDistanceStatus queryCertifiedSphere(const Sphere*& sphere, bool& inverted,
        Matrix4x4d& forward, Matrix4x4d& inverse) const;
    static GeometryContainment orientCertifiedSide(GeometryContainment side, bool inverted);
    // The VITRAL solid and binding inversion of a compatible unclipped,
    // unbounded leaf; false for wrappers whose policies change the solid.
    bool querySolidCore(const Geometry*& intrinsic, bool& inverted) const;
    // True when every bounding region is certified to contain the core solid,
    // so the bounded_by hint cannot change membership.
    bool boundingRegionsVerified() const;
    // Owned regions live in this body's outer frame and follow its object-level
    // edits. The check validates the complete region tree before any change.
    bool canMoveRegions(const Matrix4x4d& delta, const Matrix4x4d& deltaInverse) const;
    void moveRegions(const Matrix4x4d& delta, const Matrix4x4d& deltaInverse);
    java::ArrayList<RayCastingBody*> boundingShapes{4};
    java::ArrayList<RayCastingBody*> clippingShapes{4};
    Material *geometryMaterial;
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    Matrix4x4d *geometryTransformation = nullptr;
    Matrix4x4d *geometryTransformationInverse = nullptr;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
    MaterialReleaser objectTextureReleaser = nullptr;
    java::ArrayList<TransformStep> bodySteps{4};
    java::ArrayList<TransformStep> geometrySteps{4};
    bool bakedTransformFolded = false;
    bool arbitraryAffineTransform = false;

  public:
    RayCastingBody(
        RayCastingGeometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<RayCastingBody*> &boundingShapes,
        const java::ArrayList<RayCastingBody*> &clippingShapes,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr,
        const java::ArrayList<TransformStep> &bodyStepsInit = java::ArrayList<TransformStep>(),
        const java::ArrayList<TransformStep> &geometryStepsInit = java::ArrayList<TransformStep>(),
        MaterialReleaser objectTextureReleaser = nullptr) :
        SimpleBody(),
        boundingShapes(boundingShapes),
        clippingShapes(clippingShapes),
        geometryMaterial(geometryMaterial),
        transformation(transformation),
        transformationInverse(transformationInverse),
        geometryTransformation(geometryTransformation),
        geometryTransformationInverse(geometryTransformationInverse),
        noShadowFlag(noShadowFlag),
        objectColor(objectColor),
        objectTexture(objectTexture),
        objectTextureReleaser(objectTextureReleaser),
        bodySteps(bodyStepsInit),
        geometrySteps(geometryStepsInit)
    {
        setGeometry(geometry);
    }
    Material* getSurfaceMaterial() const override {
        return objectTexture ? objectTexture : geometryMaterial ? geometryMaterial : SimpleBody::getSurfaceMaterial();
    }
    SimpleMaterial* getMaterial() const override;
    bool supportsDecomposedPlacement() const override { return false; }
    Vector3Dd getPosition() const override { return getAffinePlacement().multiply(Vector3Dd()); }
    void setPosition(const Vector3Dd& position) override;
    Vector3Dd getScale() const override;
    Matrix4x4d getRotation() const override;
    Matrix4x4d getRotationInverse() const override;
    void setScale(const Vector3Dd&) override;
    void setRotation(const Matrix4x4d&) override;
    void setRotationInverse(const Matrix4x4d&) override;
    void setGeometry(Geometry* geometry) override;
    void setGeometryReference(Geometry* geometry) override;
    RayCastingBody(const RayCastingBody &other);
    virtual ~RayCastingBody();
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) const override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) const override;


    // ObjectParser builds short-lived RayCastingBody/RayCastingComposite wrappers purely to
    // invoke their virtual translate/rotate/scale/invert (which know how to transform
    // a bounded object's children in place), then extracts the resulting
    // geometry/objectTexture/objectColor/bounding-and-clipping-shapes back into local
    // parser variables before discarding the wrapper. detachOwnership() clears every
    // owned field on the wrapper (without deleting them) so that subsequent
    // `delete wrapper;` is a safe no-op for those fields - ownership has already moved
    // to whoever called this. Virtual so RayCastingComposite::detachOwnership() can also clear
    // its own simpleBodies.
    virtual void detachOwnership();

    java::ArrayList<RayCastingBody*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<RayCastingBody*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<RayCastingBody*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<RayCastingBody*>& getClippingShapes() const { return clippingShapes; }
    Matrix4x4d getTransformationMatrix() const override { return getAffinePlacement(); }
    bool hasArbitraryAffineTransform() const { return arbitraryAffineTransform; }
    RayCastingGeometry *getGeometry() const override { return static_cast<RayCastingGeometry*>(geometry); }
    const Geometry* getSolidEquivalenceGeometry() const override;
    // Bounds of the shared core solid; an inverted binding is unbounded and
    // region policies are unsupported until their typed semantics exist.
    void getSolidWorldBounds(GeometryBounds& result) const override;
    GeometryContainment classifyWorldBoxCertified(const GeometryBounds& box) const override;
    // The core solid ignores bounding and clipping regions; it is the shared
    // geometry under this body's placement and binding inversion.
    bool queryCoreSolid(const Geometry*& intrinsic, bool& inverted) const;
    GeometryContainment classifyCoreCertified(const Vector3Dd& point, double tolerance) const;
    void getCoreSolidWorldBounds(GeometryBounds& result) const;
    Material *getGeometryMaterial() const { return geometryMaterial; }
    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }
    Matrix4x4d *getGeometryTransformation() const { return geometryTransformation; }
    Matrix4x4d *getGeometryTransformationInverse() const { return geometryTransformationInverse; }
    const java::ArrayList<TransformStep> &getBodySteps() const { return bodySteps; }
    const java::ArrayList<TransformStep> &getGeometrySteps() const { return geometrySteps; }
    bool isBakedTransformFolded() const { return bakedTransformFolded; }
    void setBakedTransformFolded(bool value) { bakedTransformFolded = value; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    ColorRgba *getObjectColor() const { return objectColor; }
    Material *getObjectTexture() const { return objectTexture; }
    Vector3Dd worldPointToLocal(const Vector3Dd &point) const;

    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse) override;
    void resetAffinePlacement() override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
    GeometryDistanceStatus encloseMinimumBoundaryDistance(const Vector3Dd& point, double maxWidth,
        GeometryContainment& side, double& lower, double& upper) const override;
    GeometryDistanceStatus encloseBoundaryDistance(const Vector3Dd& point, const Vector3Dd& localDirection,
        GeometryContainment& side, double& lower, double& upper) const override;
    GeometryDistanceStatus estimateBoundaryDistance(const Vector3Dd& point,
        double& distance, double& estimatedError) const override;
    GeometryContainment classifyPointCertified(const Vector3Dd& point, double distanceTolerance) const override;
    AxisAlignedBoundingBox getAABB() const;

    virtual bool doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out);
    virtual int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr);
    virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance);
    void doExtraInformation(const RayWithTracingState &ray, double t, RayCastingHit *hit) override;
    virtual void *copy();
    // Virtual so a nested RayCastingComposite child reached through a RayCastingBody*
    // (e.g. from RayCastingComposite::translate's simpleBodies loop) dispatches to
    // RayCastingComposite's override, which propagates the transform into its own
    // simpleBodies. Without this, transforms applied to an outer composite
    // never reach the children of a nested composite.
    virtual void translate(Vector3Dd *vector);
    virtual void rotate(Vector3Dd *vector);
    virtual void scale(Vector3Dd *vector);
    void translateGeometryLayer(Vector3Dd *vector);
    void rotateGeometryLayer(Vector3Dd *vector);
    void scaleGeometryLayer(Vector3Dd *vector);
    void translateOwnerOnly(Vector3Dd *vector);
    void rotateOwnerOnly(Vector3Dd *vector);
    void scaleOwnerOnly(Vector3Dd *vector);
    virtual void invert();
    virtual void propagateOwnedTranslation(Vector3Dd *vector);
    virtual void propagateOwnedRotation(Vector3Dd *vector);
    virtual void propagateOwnedScale(Vector3Dd *vector);

  protected:
    void applyTranslationToBodyTransform(Vector3Dd *vector);
    void applyRotationToBodyTransform(Vector3Dd *vector);
    void applyScaleToBodyTransform(Vector3Dd *vector);
    void applyTranslationToGeometryTransform(Vector3Dd *vector);
    void applyRotationToGeometryTransform(Vector3Dd *vector);
    void applyScaleToGeometryTransform(Vector3Dd *vector);
    void applyOwnedTranslation(Vector3Dd *vector);
    void applyOwnedRotation(Vector3Dd *vector);
    void applyOwnedScale(Vector3Dd *vector);
    Vector3Dd objectPointToGeometryLocal(const Vector3Dd &point) const;
    Vector3Dd geometryPointToObjectLocal(const Vector3Dd &point) const;
    Vector3Dd objectDirectionToGeometryLocal(const Vector3Dd &direction) const;
    Vector3Dd geometryNormalToObjectLocal(const Vector3Dd &normal) const;
    Vector3Dd localPointToWorld(const Vector3Dd &point) const;
    Vector3Dd worldDirectionToLocal(const Vector3Dd &direction) const;
    Vector3Dd localNormalToWorld(const Vector3Dd &normal) const;

  private:
    void ensureMatrices();
    void ensureGeometryMatrices();
};

#endif
