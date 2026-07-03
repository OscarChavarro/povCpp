#ifndef __SIMPLE_BODY__
#define __SIMPLE_BODY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/AxisAlignedBox.h"
#include "environment/geometry/element/RayOperationOwner.h"
#include "environment/material/Material.h"
#include "environment/scene/TransformStep.h"

class SimpleBody : public RayOperationOwner {
  protected:
    java::ArrayList<SimpleBody*> boundingShapes{4};
    java::ArrayList<SimpleBody*> clippingShapes{4};
    Geometry *geometry;
    Material *geometryMaterial;
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    Matrix4x4d *geometryTransformation = nullptr;
    Matrix4x4d *geometryTransformationInverse = nullptr;
    bool noShadowFlag;
    ColorRgba *objectColor;
    Material *objectTexture;
    // Elementary steps behind `transformation`/`geometryTransformation`
    // above, recorded in the same chronological order those matrices were
    // composed in. geometrySteps is the innermost layer (applied to the raw
    // local geometry, including Invert); bodySteps is the outer object-level
    // layer. See doc/performanceReviewPlan5.md Phase 1.
    java::ArrayList<TransformStep> bodySteps{4};
    java::ArrayList<TransformStep> geometrySteps{4};

  public:
    SimpleBody(
        Geometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<SimpleBody*> &boundingShapes,
        const java::ArrayList<SimpleBody*> &clippingShapes,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr,
        const java::ArrayList<TransformStep> &bodyStepsInit = java::ArrayList<TransformStep>(),
        const java::ArrayList<TransformStep> &geometryStepsInit = java::ArrayList<TransformStep>()) :
        boundingShapes(boundingShapes),
        clippingShapes(clippingShapes),
        geometry(geometry),
        geometryMaterial(geometryMaterial),
        transformation(transformation),
        transformationInverse(transformationInverse),
        geometryTransformation(geometryTransformation),
        geometryTransformationInverse(geometryTransformationInverse),
        noShadowFlag(noShadowFlag),
        objectColor(objectColor),
        objectTexture(objectTexture),
        bodySteps(bodyStepsInit),
        geometrySteps(geometryStepsInit)
    {
    }
    SimpleBody(const SimpleBody &other);
    virtual ~SimpleBody();

    // ObjectParser builds short-lived SimpleBody/Composite wrappers purely to
    // invoke their virtual translate/rotate/scale/invert (which know how to transform
    // a bounded object's children in place), then extracts the resulting
    // geometry/objectTexture/objectColor/bounding-and-clipping-shapes back into local
    // parser variables before discarding the wrapper. detachOwnership() clears every
    // owned field on the wrapper (without deleting them) so that subsequent
    // `delete wrapper;` is a safe no-op for those fields - ownership has already moved
    // to whoever called this. Virtual so Composite::detachOwnership() can also clear
    // its own simpleBodies.
    virtual void detachOwnership();

    java::ArrayList<SimpleBody*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<SimpleBody*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<SimpleBody*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<SimpleBody*>& getClippingShapes() const { return clippingShapes; }
    Geometry *getGeometry() const { return geometry; }
    Material *getGeometryMaterial() const { return geometryMaterial; }
    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }
    Matrix4x4d *getGeometryTransformation() const { return geometryTransformation; }
    Matrix4x4d *getGeometryTransformationInverse() const { return geometryTransformationInverse; }
    const java::ArrayList<TransformStep> &getBodySteps() const { return bodySteps; }
    const java::ArrayList<TransformStep> &getGeometrySteps() const { return geometrySteps; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    ColorRgba *getObjectColor() const { return objectColor; }
    Material *getObjectTexture() const { return objectTexture; }
    Vector3Dd worldPointToLocal(const Vector3Dd &point) const;

    AxisAlignedBox getAABB() const;

    virtual bool doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out);
    virtual int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr);
    virtual int doContainmentTest(const Vector3Dd &point, double distanceTolerance);
    void doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit) override;
    virtual void *copy();
    // Virtual so a nested Composite child reached through a SimpleBody*
    // (e.g. from Composite::translate's simpleBodies loop) dispatches to
    // Composite's override, which propagates the transform into its own
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
