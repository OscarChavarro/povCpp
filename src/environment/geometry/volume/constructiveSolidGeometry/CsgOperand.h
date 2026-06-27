#ifndef __CSG_OPERAND__
#define __CSG_OPERAND__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"

class CsgOperand {
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

        RayWithSegments localRay = *ray;
        RayWithSegments *geometryRay = ray;
        if (transformationInverse != nullptr) {
            localRay.setOrigin(transformationInverse->transformPoint(ray->getOrigin()));
            localRay.setDirection(transformationInverse->transformDirection(ray->getDirection()));
            localRay.setQuadricConstantsCached(false);
            geometryRay = &localRay;
        }

        if (transformation == nullptr) {
            return geometry->doIntersectionForAllRayCrossings(
                geometryRay, depthQueue, getEffectiveMaterial(materialOverride));
        }

        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            ray->getIntersectionQueuePool()->pop(128);
        const int found = geometry->doIntersectionForAllRayCrossings(
            geometryRay, localDepthQueue, getEffectiveMaterial(materialOverride));
        for (const IntersectionCandidate &candidate : *localDepthQueue) {
            IntersectionCandidate transformedCandidate = candidate;
            if (transformation != nullptr) {
                transformedCandidate.getIntersection().point =
                    transformation->transformPoint(
                        transformedCandidate.getIntersection().point);
            }
            depthQueue->offer(transformedCandidate);
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

    void invert()
    {
        if (geometry != nullptr) {
            geometry->invertGeometry();
        }
    }
};

#endif
