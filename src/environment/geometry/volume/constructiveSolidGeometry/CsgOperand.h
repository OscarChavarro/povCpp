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
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/scene/TransformStep.h"

class CsgOperand : public RayOperationOwner {
  private:
    Geometry *geometry = nullptr;
    Material *material = nullptr;
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    // Elementary steps behind `transformation` above, chronologically
    // recorded (including Invert). See doc/performanceReviewPlan5.md Phase 1.
    java::ArrayList<TransformStep> steps{4};
    mutable bool bakedBoundsValid = false;
    mutable AxisAlignedBox bakedBounds = AxisAlignedBox::unbounded();
    mutable bool bakedBoundsBounded = false;
    mutable bool bakedBoundsCullSafe = false;

    void ensureMatrices()
    {
        if (transformation == nullptr) {
            transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
            transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }
    }

    void invalidateBakedBounds()
    {
        bakedBoundsValid = false;
        bakedBounds = AxisAlignedBox::unbounded();
        bakedBoundsBounded = false;
        bakedBoundsCullSafe = false;
    }

    void ensureBakedBounds() const
    {
        if (bakedBoundsValid) {
            return;
        }
        if (geometry == nullptr) {
            bakedBounds = AxisAlignedBox::unbounded();
            bakedBoundsBounded = false;
            bakedBoundsValid = true;
            return;
        }

        bakedBounds = geometry->getMinMax();
        if (transformation != nullptr && !bakedBounds.isUnbounded()) {
            bakedBounds = AxisAlignedBox::fromTransformedCorners(
                bakedBounds.min, bakedBounds.max, transformation);
        }
        if (!bakedBounds.isUnbounded()) {
            const Vector3Dd padding(1e-6, 1e-6, 1e-6);
            bakedBounds.min = bakedBounds.min.subtract(padding);
            bakedBounds.max = bakedBounds.max.add(padding);
        }
        bakedBoundsBounded = !bakedBounds.isUnbounded();
        bakedBoundsCullSafe =
            bakedBoundsBounded &&
            (dynamic_cast<Sphere *>(geometry) != nullptr ||
             dynamic_cast<Box *>(geometry) != nullptr ||
             dynamic_cast<Blob *>(geometry) != nullptr ||
             dynamic_cast<Quadric *>(geometry) != nullptr ||
             dynamic_cast<HeightField *>(geometry) != nullptr);
        bakedBoundsValid = true;
    }

    static bool rayIntersectsAabbForward(
        const RayWithSegments &ray, const AxisAlignedBox &box)
    {
        const Vector3Dd origin = ray.getOrigin();
        const Vector3Dd direction = ray.getDirection();
        double tMin = 0.0;
        double tMax = 1e30;

        auto updateAxis = [&](double originCoord, double directionCoord,
                              double minCoord, double maxCoord) -> bool {
            if (directionCoord > -1e-12 && directionCoord < 1e-12) {
                return originCoord >= minCoord && originCoord <= maxCoord;
            }
            const double invDir = 1.0 / directionCoord;
            double nearT = (minCoord - originCoord) * invDir;
            double farT = (maxCoord - originCoord) * invDir;
            if (nearT > farT) {
                const double tmp = nearT;
                nearT = farT;
                farT = tmp;
            }
            tMin = nearT > tMin ? nearT : tMin;
            tMax = farT < tMax ? farT : tMax;
            return tMin <= tMax;
        };

        return
            updateAxis(origin.x(), direction.x(), box.min.x(), box.max.x()) &&
            updateAxis(origin.y(), direction.y(), box.min.y(), box.max.y()) &&
            updateAxis(origin.z(), direction.z(), box.min.z(), box.max.z()) &&
            tMax >= 0.0;
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

    CsgOperand(
        Geometry *geometry,
        Material *material,
        Matrix4x4d *transformation,
        Matrix4x4d *transformationInverse,
        const java::ArrayList<TransformStep> &steps) :
        geometry(geometry),
        material(material),
        transformation(transformation),
        transformationInverse(transformationInverse),
        steps(steps)
    {
    }

    CsgOperand(const CsgOperand &other) :
        geometry(other.geometry != nullptr ?
            (Geometry *)other.geometry->copy() : nullptr),
        material(other.material != nullptr ? other.material->copy() : nullptr),
        transformation(other.transformation != nullptr ?
            new Matrix4x4d(*other.transformation) : nullptr),
        transformationInverse(other.transformationInverse != nullptr ?
            new Matrix4x4d(*other.transformationInverse) : nullptr),
        steps(other.steps)
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
    const java::ArrayList<TransformStep> &getSteps() const { return steps; }
    Material *getEffectiveMaterial(Material *materialOverride) const
    {
        return material != nullptr ? material : materialOverride;
    }
    const AxisAlignedBox &getBakedBounds() const
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
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        if (geometry == nullptr) {
            return false;
        }
        ensureBakedBounds();
        if (bakedBoundsBounded && bakedBoundsCullSafe &&
            !rayIntersectsAabbForward(*ray, bakedBounds)) {
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
        return getBakedBounds();
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
            steps.add(TransformStep(TransformStep::Kind::Invert, Vector3Dd(0.0, 0.0, 0.0)));
        }
    }
};

#endif
