#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "java/util/PriorityQueue.txx"
#include <typeinfo>
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"

void
CsgOperand::translate(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    invalidateBakedBounds();
    ++modificationVersion;
    steps.add(TransformStep(TransformStep::Kind::Translate, *vector));
    if (material != nullptr) {
        material = material->translate(vector);
    }
}

void
CsgOperand::rotate(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    invalidateBakedBounds();
    ++modificationVersion;
    steps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
    if (material != nullptr) {
        material = material->rotate(vector);
    }
}

void
CsgOperand::scale(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    invalidateBakedBounds();
    ++modificationVersion;
    steps.add(TransformStep(TransformStep::Kind::Scale, *vector));
    if (material != nullptr) {
        material = material->scale(vector);
    }
}

RaySolidQueryStatus
CsgOperand::classifyRay(const Ray& ray, const RayCrossingQuery& query, RaySolidClassification& result) const
{
    result.clear();
    if (geometry == nullptr) {
        return RaySolidQueryStatus::Unsupported;
    }
    Matrix4x4d forward, inverse;
    const AffinePlacementStatus placement = queryAffinePlacement(forward, inverse);
    if (placement == AffinePlacementStatus::Invalid) return RaySolidQueryStatus::InvalidInput;
    if (placement == AffinePlacementStatus::Unsupported) return RaySolidQueryStatus::Unsupported;
    if (placement != AffinePlacementStatus::Valid) return RaySolidQueryStatus::Indeterminate;
    // The direction is not renormalized, so the parameter stays the parent's.
    const Ray localRay = Ray::fromAffine(inverse.multiply(ray.getOrigin()),
        inverse.withoutTranslation().multiply(ray.getDirection()), ray.getT());
    const RaySolidQueryStatus status = geometry->classifyRay(localRay, query, result);
    if (status != RaySolidQueryStatus::Supported) result.clear();
    return status;
}

GeometryContainment
CsgOperand::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{
    const Geometry* intrinsic = nullptr;
    bool inverted = false;
    if (!RayCastingBody::queryWrappedSolid(geometry, intrinsic, inverted)) {
        return GeometryContainment::Unsupported;
    }
    Matrix4x4d forward, inverse;
    const AffinePlacementStatus placement = queryAffinePlacement(forward, inverse);
    if (placement == AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement == AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (placement != AffinePlacementStatus::Valid) return GeometryContainment::Indeterminate;
    return SimpleBody::classifyPlacedPointCertified(*intrinsic, inverted, forward, inverse, point, tolerance);
}

bool
CsgOperand::isStructuralTraversalSafe() const
{
    // Material and transform-step logs do not change the solid.
    return typeid(*this) == typeid(CsgOperand);
}

bool
CsgOperand::queryModificationVersion(long long& version) const
{
    version = modificationVersion;
    return true;
}
