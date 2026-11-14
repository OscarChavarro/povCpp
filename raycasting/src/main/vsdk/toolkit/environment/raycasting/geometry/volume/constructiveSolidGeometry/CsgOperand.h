#ifndef __CSG_OPERAND__
#define __CSG_OPERAND__

#include "vsdk/toolkit/environment/raycasting/scene/PovrayBody.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/RayCastingHit.h"
#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/Blob.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/HeightField.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingBox.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/Quadric.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/TransformStep.h"

/**
A `PovrayBody` leaf of a POV CSG: one placement layer (the body layer), the
operand material, and no regions, object texture, color or shadow flag. Its
solid is its geometry under that placement, so the shared CSG structure may
descend into a nested CSG geometry. The render role differs from a top-level
body: crossings register this operand as a detail owner, are culled by the
operand's padded bounds, and normals are unwound without normalization (the
top-level body normalizes once).
*/
class CsgOperand : public PovrayBody {
  private:
    mutable bool bakedBoundsValid = false;
    mutable AxisAlignedBoundingBox bakedBounds = AxisAlignedBoundingBox::unbounded();
    mutable bool bakedBoundsBounded = false;
    mutable bool bakedBoundsCullSafe = false;

    void invalidateBakedBounds()
    {
        bakedBoundsValid = false;
        bakedBounds = AxisAlignedBoundingBox::unbounded();
        bakedBoundsBounded = false;
        bakedBoundsCullSafe = false;
    }

    void ensureBakedBounds() const
    {
        if (bakedBoundsValid) {
            return;
        }
        if (geometry == nullptr) {
            bakedBounds = AxisAlignedBoundingBox::unbounded();
            bakedBoundsBounded = false;
            bakedBoundsValid = true;
            return;
        }

        RayCastingGeometry *shape = getGeometry();
        BoundingVolumeHierarchy *bounds = shape->createBoundingVolume();
        bakedBounds = bounds->axisAlignedExtent();
        delete bounds;
        if (transformation != nullptr && !bakedBounds.isUnbounded()) {
            bakedBounds = AxisAlignedBoundingBox::fromTransformedCorners(
                bakedBounds.getMin(), bakedBounds.getMax(), transformation);
        }
        if (!bakedBounds.isUnbounded()) {
            const Vector3Dd padding(1e-6, 1e-6, 1e-6);
            bakedBounds = AxisAlignedBoundingBox(
                bakedBounds.getMin().subtract(padding), bakedBounds.getMax().add(padding));
        }
        bakedBoundsBounded = !bakedBounds.isUnbounded();
        bakedBoundsCullSafe =
            bakedBoundsBounded &&
            (dynamic_cast<const Sphere *>(shape->getIntrinsicGeometry()) != nullptr ||
             dynamic_cast<RayCastingBox *>(shape) != nullptr ||
             dynamic_cast<Blob *>(shape) != nullptr ||
             dynamic_cast<Quadric *>(shape) != nullptr ||
             dynamic_cast<HeightField *>(shape) != nullptr);
        bakedBoundsValid = true;
    }

  protected:
    bool denotesPlacedGeometry() const override;
    bool preservesCoreSolid() const override;

  public:
    CsgOperand(
        RayCastingGeometry *geometry,
        Material *material,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        const java::ArrayList<TransformStep> &steps = java::ArrayList<TransformStep>());
    CsgOperand(const CsgOperand &other);

    void *copy() override { return new CsgOperand(*this); }

    Material *getMaterial() const { return geometryMaterial; }
    const java::ArrayList<TransformStep> &getSteps() const { return bodySteps; }
    Material *getEffectiveMaterial(Material *materialOverride) const
    {
        return geometryMaterial != nullptr ? geometryMaterial : materialOverride;
    }
    const AxisAlignedBoundingBox &getBakedBounds() const
    {
        ensureBakedBounds();
        return bakedBounds;
    }
    bool hasBoundedBakedBounds() const
    {
        ensureBakedBounds();
        return bakedBoundsBounded;
    }

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override
    {
        if (geometry == nullptr) {
            return false;
        }
        ensureBakedBounds();
        if (bakedBoundsBounded && bakedBoundsCullSafe &&
            !bakedBounds.intersectsRayForward(*ray)) {
            return false;
        }

        RayCastingGeometry *shape = getGeometry();
        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            ray->getIntersectionQueuePool()->pop(128);
        // Build a local-space ray clone only when this operand actually carries
        // a transform. The clone is consumed solely by the geometry call below,
        // so when there is no transform the geometry can intersect the parent
        // ray directly - avoiding a RayWithTracingState construction/destruction per
        // operand per ray, which is the hottest allocation on the CSG path.
        int found;
        if (transformationInverse != nullptr) {
            RayWithTracingState localRay = RayWithTracingState::localIntersectionClone(*ray);
            localRay.setOrigin(transformationInverse->transformPoint(ray->getOrigin()));
            localRay.setDirection(transformationInverse->transformDirection(ray->getDirection()));
            localRay.setQuadricConstantsCached(false);
            found = shape->doIntersectionForAllRayCrossings(
                &localRay, localDepthQueue, getEffectiveMaterial(materialOverride));
        } else {
            found = shape->doIntersectionForAllRayCrossings(
                ray, localDepthQueue, getEffectiveMaterial(materialOverride));
        }
        // Mutate each crossing in place in the scratch queue (which is cleared
        // and recycled immediately below) and offer it once, rather than
        // copying the ~168-byte candidate twice per crossing.
        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        const double dirLenSq = rayDir.dotProduct(rayDir);
        for (IntersectionCandidate &candidate : *localDepthQueue) {
            candidate.getAttributes().pushDetailOwner(this);
            candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
            if (transformation != nullptr) {
                candidate.getIntersection().point =
                    transformation->transformPoint(
                        candidate.getIntersection().point);
            }
            // Signed ray parameter, not a bare distance: see the matching note
            // in PovrayBody::doIntersectionForAllRayCrossings. length() would
            // flip crossings behind the origin to the front and break CSG
            // membership for internal/refracted rays.
            candidate.getIntersection().t =
                candidate.getIntersection().point
                    .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
            depthQueue->offer(candidate);
        }
        localDepthQueue->clear();
        ray->getIntersectionQueuePool()->push(localDepthQueue);
        return found;
    }

    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override
    {
        if (geometry == nullptr) {
            return RayCastingGeometry::OUTSIDE;
        }
        const Vector3Dd localPoint = transformationInverse != nullptr ?
            transformationInverse->transformPoint(point) : point;
        return getGeometry()->doContainmentTest(localPoint, distanceTolerance);
    }

    BoundingVolumeHierarchy *createBoundingVolume() const
    {
        return new AabbBoundingVolume(getBakedBounds());
    }

    // Body-layer edits that also move the operand material; regions do not exist.
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

    void doExtraInformation(
        const RayWithTracingState &ray, double t, RayCastingHit *hit) override
    {
        // Peel this operand's transform off the ray/point, then recurse into
        // the next inner detail owner (outermost-first consumption). The
        // owners were pushed innermost-first while collecting the hit, so for
        // a nested CSG operand (e.g. a declared intersection scaled inside an
        // outer union operand) the outer operand's transform must be removed
        // before the inner ones. Only the innermost owner (no further owner
        // left) evaluates its own primitive geometry's normal; each level then
        // re-applies its inverse to the normal while unwinding.
        RayWithTracingState localRay = RayWithTracingState::localIntersectionClone(ray);
        const Vector3Dd parentPoint = hit->p;
        if (transformationInverse != nullptr && !bakedTransformFolded) {
            localRay.setOrigin(transformationInverse->transformPoint(ray.getOrigin()));
            localRay.setDirection(transformationInverse->transformDirection(ray.getDirection()));
            localRay.setQuadricConstantsCached(false);
            hit->p = transformationInverse->transformPoint(parentPoint);
        }

        PostRayHitElement *next = hit->popDetailOwnerBack();
        if (next != nullptr) {
            next->doExtraInformation(localRay, t, hit);
        } else {
            PostRayHitElement *effectiveGeometry = bakedTransformFolded ? hit->hitGeometry : getGeometry();
            if (effectiveGeometry != nullptr) {
                effectiveGeometry->doExtraInformation(localRay, t, hit);
            }
        }
        if (transformationInverse != nullptr && !bakedTransformFolded) {
            // Deferred normalization: apply the inverse-transpose but do NOT
            // normalize here. With several nested non-uniform operand scales,
            // renormalizing between levels rescales the normal before the next
            // non-uniform multiply and corrupts the composition. The final
            // normalize happens once at the top of the chain (PovrayBody).
            hit->n = transformationInverse->withoutTranslation().multiply(hit->n);
        }
        hit->p = parentPoint;
    }
};

#endif
