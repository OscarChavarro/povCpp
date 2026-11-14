#ifndef __POVRAY_BODY__
#define __POVRAY_BODY__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/scene/SceneSolid.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"
#include "environment/intersection/hit/SurfaceDetailFamily.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/RayCastingHit.h"
#include "environment/intersection/hit/RayCastingContainment.h"
#include "environment/intersection/hit/PostRayHitElement.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "environment/intersection/ray/TransformStep.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/hit/SurfaceDetailFamily.h"

class CsgOperandView;
class CsgTree;
class CsgBodyOperand;

class ProjectedTriangleTraversal;
class ConstructiveSolidGeometry;
class Light;

class PovrayBody : public SceneSolid, public PostRayHitElement {
  public:
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray,
        const RayCrossingQuery& query, RayCrossingSink& sink) const override;
    // Callers that hand PovrayBody an objectTexture whose destruction needs
    // more than a plain `delete` (e.g. POV-Ray's shared default-texture
    // aliasing) supply the release logic themselves at construction time, so
    // the scene layer never needs to know the concrete Material subtype.
    using MaterialReleaser = void (*)(Material *);

  protected:
    // Whether this class keeps the shared core solid of its geometry under its
    // placement (plain bodies and CSG operands); wrappers such as composites
    // do not, so their core solid is never reported.
    virtual bool preservesCoreSolid() const;
    // The VITRAL solid and binding inversion of a compatible unclipped,
    // unbounded leaf; false for wrappers whose policies change the solid.
    bool queryDenotedSolid(const Geometry*& intrinsic, bool& inverted) const override;
    // True when every bounding region is certified to contain the core solid,
    // so the bounded_by hint cannot change membership.
    bool boundingRegionsVerified() const;
    // Builds clipped_by as the shared intersection of the core and every clip.
    // Views are owned by the caller and must be deleted even on failure.
    bool buildClippedExpression(const CsgOperandView& core, CsgTree& expression,
        java::ArrayList<CsgBodyOperand*>& views) const;
    // Owned regions live in this body's outer frame and follow its object-level
    // edits. The check validates the complete region tree before any change.
    bool canMoveRegions(const Matrix4x4d& delta, const Matrix4x4d& deltaInverse) const;
    void moveRegions(const Matrix4x4d& delta, const Matrix4x4d& deltaInverse);
    java::ArrayList<PovrayBody*> boundingShapes{4};
    java::ArrayList<PovrayBody*> clippingShapes{4};
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
    // The traced VITRAL surface of the geometry, its family and this body's
    // POV trace state, which holds the inversion of box, polynomial, blob,
    // sphere and generic surfaces.
    Geometry *surface = nullptr;
    // The POV CSG this body owns instead of a geometry.
    ConstructiveSolidGeometry *csg = nullptr;
    // The POV light this body owns instead of a geometry: it has no
    // crossings and no interior (family Empty).
    Light *light = nullptr;
    // POV's projected traversal of a triangle mesh geometry (its inversion).
    ProjectedTriangleTraversal *triangleTraversal = nullptr;
    SurfaceDetailFamily surfaceFamily = SurfaceDetailFamily::None;
    SurfaceIntersectionState surfaceState;

    // Rebinds surface, family and a fresh trace state to the geometry.
    void bindSurface();
    // The geometry's crossings and containment through the family intersector
    // with this body's state; a CSG is traced by its evaluator.
    int traceSurfaceCrossings(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue, Material *materialOverride);
    int surfaceContainment(const Vector3Dd &point, double distanceTolerance);
    // Render culling volume of the geometry in its own frame.
    BoundingVolumeHierarchy *createSurfaceBoundingVolume() const;
    // VITRAL crossings of a projected mesh, traced by POV's traversal within
    // this body's placement.
    RayCrossingSupport emitMeshCrossings(const Ray& source, const RayCrossingQuery& query,
        RayCrossingSink& sink) const;
    // Inverts the surface: the state flag for the families that keep it,
    // the geometry otherwise (which also invalidates the viewpoint cache).
    void invertSurface();

  public:
    PovrayBody(
        Geometry *geometry,
        Material *geometryMaterial,
        Material *objectTexture,
        ColorRgba *objectColor,
        bool noShadowFlag,
        const java::ArrayList<PovrayBody*> &boundingShapes,
        const java::ArrayList<PovrayBody*> &clippingShapes,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        Matrix4x4d *geometryTransformation = nullptr,
        Matrix4x4d *geometryTransformationInverse = nullptr,
        const java::ArrayList<TransformStep> &bodyStepsInit = java::ArrayList<TransformStep>(),
        const java::ArrayList<TransformStep> &geometryStepsInit = java::ArrayList<TransformStep>(),
        MaterialReleaser objectTextureReleaser = nullptr) :
        SceneSolid(),
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
        return objectTexture ? objectTexture : geometryMaterial ? geometryMaterial : SceneSolid::getSurfaceMaterial();
    }
    void setGeometry(Geometry* geometry) override;
    void setGeometryReference(Geometry* geometry) override;
    PovrayBody(const PovrayBody &other);
    virtual ~PovrayBody();
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) const override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) const override;


    java::ArrayList<PovrayBody*>& getBoundingShapes() { return boundingShapes; }
    const java::ArrayList<PovrayBody*>& getBoundingShapes() const { return boundingShapes; }
    java::ArrayList<PovrayBody*>& getClippingShapes() { return clippingShapes; }
    const java::ArrayList<PovrayBody*>& getClippingShapes() const { return clippingShapes; }
    bool hasArbitraryAffineTransform() const { return arbitraryAffineTransform; }
    // The POV light this body owns, or null.
    Light *getLight() const { return light; }
    // Takes ownership of a light, which replaces the geometry.
    void setLight(Light *value);
    // The POV CSG this body owns, or null.
    ConstructiveSolidGeometry *getCsg() const { return csg; }
    // Takes ownership of a CSG, which replaces the geometry.
    void setCsg(ConstructiveSolidGeometry *value);
    // True when the body has a geometry, a CSG or a light.
    bool hasShape() const { return geometry != nullptr || csg != nullptr || light != nullptr; }
    // Bounds of the shared core solid; an inverted binding is unbounded and
    // region policies are unsupported until their typed semantics exist.
    void getSolidWorldBounds(GeometryBounds& result) const override;
    // The core solid ignores bounding and clipping regions; it is the shared
    // geometry under this body's placement and binding inversion.
    bool queryCoreSolid(const Geometry*& intrinsic, bool& inverted) const;
    GeometryContainment classifyCoreCertified(const Vector3Dd& point, double tolerance) const;
    RaySolidQueryStatus classifyCoreRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const;
    void getCoreSolidWorldBounds(GeometryBounds& result) const;
    Material *getGeometryMaterial() const { return geometryMaterial; }
    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }
    Matrix4x4d *getGeometryTransformation() const { return geometryTransformation; }
    Matrix4x4d *getGeometryTransformationInverse() const { return geometryTransformationInverse; }
    const java::ArrayList<TransformStep> &getBodySteps() const { return bodySteps; }
    const java::ArrayList<TransformStep> &getGeometrySteps() const { return geometrySteps; }
    Geometry *getSurface() const { return surface; }
    // Takes ownership of the projected traversal of this body's triangle
    // mesh, which makes the surface a POV mesh.
    void setTriangleTraversal(ProjectedTriangleTraversal *value);
    ProjectedTriangleTraversal *getTriangleTraversal() const { return triangleTraversal; }
    SurfaceDetailFamily getSurfaceFamily() const { return surfaceFamily; }
    SurfaceIntersectionState *getSurfaceIntersectionState() { return &surfaceState; }
    bool isSurfaceInverted() const { return surfaceState.isInverted(); }
    // Sets the inversion flag of a family that keeps it (see invertSurface).
    void setSurfaceInverted(bool value);
    // Bounds of the geometry in its own frame; unbounded when inverted.
    void getSurfaceBounds(GeometryBounds &result) const;
    void getWorldBounds(GeometryBounds &result) const override;
    GeometryContainment classifyPoint(const Vector3Dd &point, double distanceTolerance) const override;
    bool isBakedTransformFolded() const { return bakedTransformFolded; }
    void setBakedTransformFolded(bool value) { bakedTransformFolded = value; }
    bool getNoShadowFlag() const { return noShadowFlag; }
    void setNoShadowFlag(bool value) { noShadowFlag = value; }
    ColorRgba *getObjectColor() const { return objectColor; }
    // Takes ownership of the color and deletes the previous one.
    void setObjectColor(ColorRgba *color);
    Material *getObjectTexture() const { return objectTexture; }
    // Replaces the texture without releasing the previous one: textures are
    // shared or layered by the caller, which decides their release.
    void setObjectTexture(Material *texture) { objectTexture = texture; }
    Vector3Dd worldPointToLocal(const Vector3Dd &point) const;

    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse) override;
    void resetAffinePlacement() override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
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
    // Virtual so a nested RayCastingComposite child reached through a PovrayBody*
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
