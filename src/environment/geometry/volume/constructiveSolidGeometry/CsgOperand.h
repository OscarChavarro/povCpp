#ifndef __CSG_OPERAND__
#define __CSG_OPERAND__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/element/RayOperationOwner.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"

class CsgOperand : public RayOperationOwner {
  private:
    Geometry *geometry = nullptr;
    Material *material = nullptr;
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;

    void ensureMatrices()
    {
        if (transformation == nullptr) {
            transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
            transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }
    }

  public:
    CsgOperand(Geometry *geometry, Material *material) :
        geometry(geometry), material(material)
    {
    }

    CsgOperand(
        Geometry *geometry,
        Material *material,
        Matrix4x4d *transformation,
        Matrix4x4d *transformationInverse) :
        geometry(geometry),
        material(material),
        transformation(transformation),
        transformationInverse(transformationInverse)
    {
    }

    CsgOperand(const CsgOperand &other) :
        geometry(other.geometry != nullptr ?
            (Geometry *)other.geometry->copy() : nullptr),
        material(other.material != nullptr ? other.material->copy() : nullptr),
        transformation(other.transformation != nullptr ?
            new Matrix4x4d(*other.transformation) : nullptr),
        transformationInverse(other.transformationInverse != nullptr ?
            new Matrix4x4d(*other.transformationInverse) : nullptr)
    {
    }

    ~CsgOperand()
    {
        delete geometry;
        delete material;
        delete transformation;
        delete transformationInverse;
    }

    CsgOperand *copy() const { return new CsgOperand(*this); }

    Geometry *getGeometry() const { return geometry; }
    Material *getMaterial() const { return material; }
    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }
    Material *getEffectiveMaterial(Material *materialOverride) const
    {
        return material != nullptr ? material : materialOverride;
    }

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        if (geometry == nullptr) {
            return false;
        }

        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            ray->getIntersectionQueuePool()->pop(128);
        // Build a local-space ray clone only when this operand actually carries
        // a transform. The clone is consumed solely by the geometry call below,
        // so when there is no transform the geometry can intersect the parent
        // ray directly - avoiding a RayWithSegments construction/destruction per
        // operand per ray, which is the hottest allocation on the CSG path.
        int found;
        if (transformationInverse != nullptr) {
            RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, *ray);
            localRay.setOrigin(transformationInverse->transformPoint(ray->getOrigin()));
            localRay.setDirection(transformationInverse->transformDirection(ray->getDirection()));
            localRay.setQuadricConstantsCached(false);
            found = geometry->doIntersectionForAllRayCrossings(
                &localRay, localDepthQueue, getEffectiveMaterial(materialOverride));
        } else {
            found = geometry->doIntersectionForAllRayCrossings(
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
            // in SimpleBody::doIntersectionForAllRayCrossings. length() would
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

    int doContainmentTest(const Vector3Dd &point, double distanceTolerance)
    {
        if (geometry == nullptr) {
            return Geometry::OUTSIDE;
        }
        const Vector3Dd localPoint = transformationInverse != nullptr ?
            transformationInverse->transformPoint(point) : point;
        return geometry->doContainmentTest(localPoint, distanceTolerance);
    }

    AxisAlignedBox getMinMax() const
    {
        if (geometry == nullptr) {
            return AxisAlignedBox::unbounded();
        }
        AxisAlignedBox box = geometry->getMinMax();
        if (transformation != nullptr && !box.isUnbounded()) {
            return AxisAlignedBox::fromTransformedCorners(
                box.min, box.max, transformation);
        }
        return box;
    }

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void doExtraInformation(
        const RayWithSegments &ray, double t, PovRayHit *hit) override
    {
        // Peel this operand's transform off the ray/point, then recurse into
        // the next inner detail owner (outermost-first consumption). The
        // owners were pushed innermost-first while collecting the hit, so for
        // a nested CSG operand (e.g. a declared intersection scaled inside an
        // outer union operand) the outer operand's transform must be removed
        // before the inner ones. Only the innermost owner (no further owner
        // left) evaluates its own primitive geometry's normal; each level then
        // re-applies its inverse to the normal while unwinding.
        RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, ray);
        const Vector3Dd parentPoint = hit->p;
        if (transformationInverse != nullptr) {
            localRay.setOrigin(transformationInverse->transformPoint(ray.getOrigin()));
            localRay.setDirection(transformationInverse->transformDirection(ray.getDirection()));
            localRay.setQuadricConstantsCached(false);
            hit->p = transformationInverse->transformPoint(parentPoint);
        }

        RayOperationOwner *next = hit->popDetailOwnerBack();
        if (next != nullptr) {
            next->doExtraInformation(localRay, t, hit);
        } else if (geometry != nullptr) {
            geometry->doExtraInformation(localRay, t, hit);
        }
        if (transformationInverse != nullptr) {
            // Deferred normalization: apply the inverse-transpose but do NOT
            // normalize here. With several nested non-uniform operand scales,
            // renormalizing between levels rescales the normal before the next
            // non-uniform multiply and corrupts the composition. The final
            // normalize happens once at the top of the chain (SimpleBody).
            hit->n = transformationInverse->withoutTranslation().multiply(hit->n);
        }
        hit->p = parentPoint;
    }

    void invert()
    {
        if (geometry != nullptr) {
            geometry->invertGeometry();
        }
    }
};

#endif
