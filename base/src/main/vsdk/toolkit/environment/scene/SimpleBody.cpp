#include <cfloat>
#include <cmath>

#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/media/NormalMapTexture.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/SurfaceRayIntersection.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
SimpleBody::SimpleBody()
    : geometry(0), ownsGeometry(true), geometryIsSphere(false), position(0, 0, 0), scale(1, 1, 1), rotation(), rotationInverse(),
      rotationQuaternion(rotation.exportToQuaternion()), rotationInverseQuaternion(rotationInverse.exportToQuaternion()),
      inverseScale(1, 1, 1), hasInvertibleScale(true),
      hasIdentityRotation(true), hasUnitScale(true), hasZeroTranslation(true),
      hasTranslationOnlyTransform(true), hasIdentityTransform(true),
      material(0), ownsMaterial(true), texture(0), normalMap(0), normalMapRgb(0),
      name(""), modificationVersion(0)
{
}

SimpleBody::~SimpleBody()
{
    delete normalMapRgb;
    if ( geometry != 0 ) {
        if ( ownsGeometry ) {
            delete geometry;
        }
        geometry = 0;
    }
    if ( material != 0 ) {
        if ( ownsMaterial ) {
            delete material;
        }
        material = 0;
    }
    if ( texture != 0 ) {
        delete texture;
        texture = 0;
    }
    if ( normalMap != 0 ) {
        delete normalMap;
        normalMap = 0;
    }
}

void SimpleBody::markModified() { modificationVersion++; }

bool SimpleBody::isIdentityRotation(const Matrix4x4d& matrix)
{
    return
        std::abs(matrix.get(0, 0) - 1.0) <= VSDK::EPSILON &&
        std::abs(matrix.get(0, 1)) <= VSDK::EPSILON &&
        std::abs(matrix.get(0, 2)) <= VSDK::EPSILON &&
        std::abs(matrix.get(1, 0)) <= VSDK::EPSILON &&
        std::abs(matrix.get(1, 1) - 1.0) <= VSDK::EPSILON &&
        std::abs(matrix.get(1, 2)) <= VSDK::EPSILON &&
        std::abs(matrix.get(2, 0)) <= VSDK::EPSILON &&
        std::abs(matrix.get(2, 1)) <= VSDK::EPSILON &&
        std::abs(matrix.get(2, 2) - 1.0) <= VSDK::EPSILON;
}

void SimpleBody::updateTransformFlags()
{
    hasIdentityRotation = isIdentityRotation(rotation);
    hasUnitScale =
        std::abs(scale.x() - 1.0) <= VSDK::EPSILON &&
        std::abs(scale.y() - 1.0) <= VSDK::EPSILON &&
        std::abs(scale.z() - 1.0) <= VSDK::EPSILON;
    hasZeroTranslation =
        std::abs(position.x()) <= VSDK::EPSILON &&
        std::abs(position.y()) <= VSDK::EPSILON &&
        std::abs(position.z()) <= VSDK::EPSILON;
    hasTranslationOnlyTransform = hasIdentityRotation && hasUnitScale;
    hasIdentityTransform = hasTranslationOnlyTransform && hasZeroTranslation;
}

const java::String& SimpleBody::getName() const { return name; }
long long SimpleBody::getModificationVersion() const { return modificationVersion; }
void SimpleBody::setName(const java::String& n) { name = n; markModified(); }

Geometry* SimpleBody::getGeometry() const { return geometry; }
void SimpleBody::setGeometry(Geometry* g)
{
    if ( geometry != g && geometry != 0 && ownsGeometry ) {
        delete geometry;
    }
    geometry = g;
    ownsGeometry = true;
    geometryIsSphere = dynamic_cast<Sphere*>(geometry) != 0;
    markModified();
}

void SimpleBody::setGeometryReference(Geometry* g)
{
    if ( geometry != g && geometry != 0 && ownsGeometry ) {
        delete geometry;
    }
    geometry = g;
    ownsGeometry = false;
    geometryIsSphere = dynamic_cast<Sphere*>(geometry) != 0;
    markModified();
}

Matrix4x4d SimpleBody::getRotation() const { return rotation; }
void SimpleBody::setRotation(const Matrix4x4d& r)
{
    Matrix4x4d sanitized = r.withoutTranslation();
    rotationQuaternion = sanitized.exportToQuaternion().normalized();
    rotation = sanitized;
    rotationInverseQuaternion = rotationQuaternion.conjugated();
    rotationInverse = Matrix4x4d().importFromQuaternion(rotationInverseQuaternion);
    updateTransformFlags();
    markModified();
}

Matrix4x4d SimpleBody::getRotationInverse() const { return rotationInverse; }
void SimpleBody::setRotationInverse(const Matrix4x4d& ri)
{
    Matrix4x4d sanitized = ri.withoutTranslation();
    rotationInverseQuaternion = sanitized.exportToQuaternion().normalized();
    rotationInverse = sanitized;
    rotationQuaternion = rotationInverseQuaternion.conjugated();
    rotation = Matrix4x4d().importFromQuaternion(rotationQuaternion);
    updateTransformFlags();
    markModified();
}

SimpleMaterial* SimpleBody::getMaterial() const { return material; }
void SimpleBody::setMaterial(SimpleMaterial* m)
{
    if ( material != m && material != 0 && ownsMaterial ) {
        delete material;
    }
    material = m;
    ownsMaterial = true;
    markModified();
}

void SimpleBody::setMaterialReference(SimpleMaterial* m)
{
    if ( material != m && material != 0 && ownsMaterial ) {
        delete material;
    }
    material = m;
    ownsMaterial = false;
    markModified();
}
Image* SimpleBody::getTexture() const { return texture; }
void SimpleBody::setTexture(Image* in)
{
    if ( texture != in && texture != 0 ) {
        delete texture;
    }
    texture = in;
    markModified();
}
NormalMap* SimpleBody::getNormalMap() const { return normalMap; }
RGBImageUncompressed* SimpleBody::getNormalMapRgb() const { return normalMapRgb; }

void SimpleBody::setNormalMap(NormalMap* in)
{
    if ( normalMap != in && normalMap != 0 ) {
        delete normalMap;
    }
    normalMap = in;
    delete normalMapRgb;
    normalMapRgb = in ? new NormalMapTexture(*in) : 0;
    markModified();
}

Vector3Dd SimpleBody::getPosition() const { return position; }
void SimpleBody::setPosition(const Vector3Dd& p) { position = p; updateTransformFlags(); markModified(); }
Vector3Dd SimpleBody::getScale() const { return scale; }

Matrix4x4d SimpleBody::getTransformationMatrix() const
{
    Matrix4x4d scaleMatrix = Matrix4x4d().scale(scale);
    Matrix4x4d translateMatrix = Matrix4x4d().translation(position);
    return translateMatrix.multiply(rotation.multiply(scaleMatrix));
}

void SimpleBody::setScale(const Vector3Dd& s)
{
    scale = s;
    hasInvertibleScale =
        std::abs(scale.x()) > VSDK::EPSILON &&
        std::abs(scale.y()) > VSDK::EPSILON &&
        std::abs(scale.z()) > VSDK::EPSILON;

    if ( hasInvertibleScale ) {
        inverseScale = Vector3Dd(1.0 / scale.x(), 1.0 / scale.y(), 1.0 / scale.z());
    }
    else {
        inverseScale = Vector3Dd();
    }
    updateTransformFlags();
    markModified();
}

Ray* SimpleBody::doIntersectionFirstHit(const Ray& inRay) const
{
    RayHit hit;
    if ( !doIntersectionFirstHit(inRay, &hit) || hit.getRay() == 0 ) {
        return 0;
    }
    return new Ray(*hit.getRay());
}

bool SimpleBody::doIntersectionWithTranslationOnlySphereFastPath(const Ray& inOutRay, RayHit* outHit) const
{
    const Sphere* sphere = static_cast<const Sphere*>(geometry);
    double t, farT;
    if (Sphere::intersectionRoots(inOutRay.getOrigin().subtract(position),
            inOutRay.getDirection(), sphere->getRadius(),
            RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), t, farT) == 0) {
        return false;
    }

    if ( outHit != 0 ) {
        if ( outHit->shouldStoreRay() ) {
            outHit->setRay(inOutRay.withT(t));
        }
        else {
            outHit->setHitDistance(t);
        }
    }
    return true;
}

bool SimpleBody::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit) const
{
    if ( geometry == 0 || !hasInvertibleScale ) {
        return false;
    }

    const int requestedDetailMask = outHit != 0 ? outHit->getRequiredDetailMask() : RayHit::DETAIL_NONE;

    if ( hasTranslationOnlyTransform && requestedDetailMask == RayHit::DETAIL_NONE && geometryIsSphere ) {
        return doIntersectionWithTranslationOnlySphereFastPath(inOutRay, outHit);
    }

    if ( hasIdentityTransform ) {
        return SurfaceRayIntersection::doIntersectionFirstHit(geometry, inOutRay, outHit);
    }

    if ( hasTranslationOnlyTransform ) {
        return doIntersectionWithTranslationOnly(inOutRay, outHit, requestedDetailMask);
    }

    Vector3Dd translatedOrigin = inOutRay.getOrigin().subtract(position);
    Vector3Dd rotatedOrigin = rotationInverseQuaternion.rotate(translatedOrigin);
    Vector3Dd localOrigin(
        rotatedOrigin.x() * inverseScale.x(),
        rotatedOrigin.y() * inverseScale.y(),
        rotatedOrigin.z() * inverseScale.z());

    Vector3Dd rotatedDirection = rotationInverseQuaternion.rotate(inOutRay.getDirection());
    Vector3Dd localDirection(
        rotatedDirection.x() * inverseScale.x(),
        rotatedDirection.y() * inverseScale.y(),
        rotatedDirection.z() * inverseScale.z());
    const double localDirectionLength = localDirection.length();
    if ( localDirectionLength <= VSDK::EPSILON ) {
        return false;
    }
    localDirection = localDirection.multiply(1.0 / localDirectionLength);

    double localRayT = inOutRay.getT();
    if ( localRayT >= DBL_MAX / localDirectionLength ) {
        localRayT = DBL_MAX;
    }
    else {
        localRayT *= localDirectionLength;
    }

    Ray localRay(localOrigin, localDirection, localRayT);
    const bool requestedStoreRay = outHit != 0 ? outHit->shouldStoreRay() : false;

    if ( outHit != 0 ) {
        outHit->setStoreRay(false);
        outHit->resetForDistanceOnly();
        if ( !SurfaceRayIntersection::doIntersectionFirstHit(geometry, localRay, outHit) ) {
            outHit->setStoreRay(requestedStoreRay);
            outHit->setRequiredDetailMask(requestedDetailMask);
            return false;
        }
    }
    else {
        RayHit localHitStorage;
        localHitStorage.setStoreRay(false);
        localHitStorage.resetForDistanceOnly();
        if ( !SurfaceRayIntersection::doIntersectionFirstHit(geometry, localRay, &localHitStorage) ) {
            return false;
        }
    }

    if ( outHit != 0 ) {
        double localHitT;
        if ( outHit->getRay() != 0 ) {
            localHitT = outHit->getRay()->getT();
        }
        else if ( outHit->hasHitDistance() ) {
            localHitT = outHit->getHitDistance();
        }
        else {
            return false;
        }

        double worldT = localHitT / localDirectionLength;

        outHit->setStoreRay(requestedStoreRay);
        outHit->setRequiredDetailMask(requestedDetailMask);
        if ( requestedStoreRay || requestedDetailMask != RayHit::DETAIL_NONE ) {
            Ray worldRay = inOutRay.withT(worldT);
            outHit->setRay(worldRay);
            if ( requestedDetailMask != RayHit::DETAIL_NONE ) {
                doExtraInformation(worldRay, worldT, outHit);
            }
        }
        else {
            outHit->setHitDistance(worldT);
        }
    }
    return true;
}

bool SimpleBody::doIntersectionWithTranslationOnly(
    const Ray& inOutRay,
    RayHit* outHit,
    int requiredDetailMask) const
{
    Ray localRay = inOutRay.withOrigin(inOutRay.getOrigin().subtract(position));

    RayHit localHitStorage;
    RayHit* hit = outHit != 0 ? outHit : &localHitStorage;

    if ( requiredDetailMask == RayHit::DETAIL_NONE ) {
        hit->resetForDistanceOnly();
    }
    else {
        hit->reset(requiredDetailMask);
    }

    if ( !SurfaceRayIntersection::doIntersectionFirstHit(geometry, localRay, hit) ) {
        return false;
    }

    double localHitT;
    if ( hit->getRay() != 0 ) {
        localHitT = hit->getRay()->getT();
    }
    else if ( hit->hasHitDistance() ) {
        localHitT = hit->getHitDistance();
    }
    else {
        return false;
    }

    if ( outHit != 0 ) {
        if ( outHit->shouldStoreRay() || outHit->needsAnySurfaceData() ) {
            outHit->setRay(inOutRay.withT(localHitT));
        }
        else {
            outHit->setHitDistance(localHitT);
        }
        if ( outHit->needsPoint() ) {
            outHit->point = hit->point.add(position);
        }
        if ( outHit->needsNormal() ) {
            outHit->normal = hit->normal;
        }
        if ( outHit->needsTextureCoordinates() ) {
            outHit->u = hit->u;
            outHit->v = hit->v;
        }
        if ( outHit->needsTangent() ) {
            outHit->tangent = hit->tangent;
        }
        outHit->material = hit->material;
        outHit->texture = hit->texture;
        outHit->normalMap = hit->normalMap;
    }

    return true;
}

int SimpleBody::computeQuantitativeInvisibility(
    const Vector3Dd& origin,
    const Vector3Dd& p) const
{
    if ( geometry == 0 || !hasInvertibleScale ) {
        return 0;
    }

    Vector3Dd translatedOrigin = origin.subtract(position);
    Vector3Dd rotatedOrigin = rotationInverseQuaternion.rotate(translatedOrigin);
    Vector3Dd myOrigin(
        rotatedOrigin.x() * inverseScale.x(),
        rotatedOrigin.y() * inverseScale.y(),
        rotatedOrigin.z() * inverseScale.z());

    Vector3Dd translatedPoint = p.subtract(position);
    Vector3Dd rotatedPoint = rotationInverseQuaternion.rotate(translatedPoint);
    Vector3Dd myPoint(
        rotatedPoint.x() * inverseScale.x(),
        rotatedPoint.y() * inverseScale.y(),
        rotatedPoint.z() * inverseScale.z());

    return geometry->computeQuantitativeInvisibility(myOrigin, myPoint);
}

void SimpleBody::doExtraInformation(const Ray&, double, RayHit* outData) const
{
    if ( outData == 0 || geometry == 0 || !hasInvertibleScale ) return;

    const Ray* worldRayPtr = outData->getRay();
    if ( worldRayPtr == 0 ) {
        outData->material = material;
        outData->texture = texture;
        outData->normalMap = normalMap;
        return;
    }

    const Ray worldRay = *worldRayPtr;
    const double worldT = worldRay.getT();

    Vector3Dd translatedOrigin = worldRay.getOrigin().subtract(position);
    Vector3Dd rotatedOrigin = rotationInverseQuaternion.rotate(translatedOrigin);
    Vector3Dd localOrigin(
        rotatedOrigin.x() * inverseScale.x(),
        rotatedOrigin.y() * inverseScale.y(),
        rotatedOrigin.z() * inverseScale.z());

    Vector3Dd rotatedDirection = rotationInverseQuaternion.rotate(worldRay.getDirection());
    Vector3Dd localDirection(
        rotatedDirection.x() * inverseScale.x(),
        rotatedDirection.y() * inverseScale.y(),
        rotatedDirection.z() * inverseScale.z());
    double localDirLength = localDirection.length();
    if ( localDirLength <= VSDK::EPSILON ) {
        outData->material = material;
        outData->texture = texture;
        outData->normalMap = normalMap;
        return;
    }
    localDirection = localDirection.multiply(1.0 / localDirLength);
    double localT = worldT * localDirLength;
    Ray localRay(localOrigin, localDirection, localT);

    geometry->doExtraInformation(localRay, localT, outData);

    if ( outData->needsPoint() ) {
        Vector3Dd worldPoint = rotation.multiply(Vector3Dd(
            outData->point.x() * scale.x(),
            outData->point.y() * scale.y(),
            outData->point.z() * scale.z())).add(position);
        outData->point = worldPoint;
    }

    if ( outData->needsNormal() ) {
        Vector3Dd worldNormal = rotation.multiply(Vector3Dd(
            outData->normal.x() * inverseScale.x(),
            outData->normal.y() * inverseScale.y(),
            outData->normal.z() * inverseScale.z())).normalized();
        outData->normal = worldNormal;
    }

    if ( outData->needsTangent() ) {
        Vector3Dd worldTangent = rotation.multiply(Vector3Dd(
            outData->tangent.x() * scale.x(),
            outData->tangent.y() * scale.y(),
            outData->tangent.z() * scale.z())).normalized();
        outData->tangent = worldTangent;
    }

    outData->material = material;
    outData->texture = texture;
    outData->normalMap = normalMap;
    outData->setRay(worldRay.withT(worldT));
}

Material* SimpleBody::getSurfaceMaterial() const { return material; }


SimpleBody::QueryPlacementStatus SimpleBody::queryPlacementStatus() const
{
    if (!java::Double::isFinite(position.x()) || !java::Double::isFinite(position.y()) ||
        !java::Double::isFinite(position.z()) || !java::Double::isFinite(scale.x()) ||
        !java::Double::isFinite(scale.y()) || !java::Double::isFinite(scale.z()) ||
        scale.x() == 0 || scale.y() == 0 || scale.z() == 0) return QueryPlacementStatus::Invalid;
    if (!hasInvertibleScale) return QueryPlacementStatus::Indeterminate;
    // This scene type promises a decomposed rotation, not a general affine matrix.
    // Reject non-orthogonal or inconsistent matrices rather than silently using
    // the quaternion-derived inverse as if it were the inverse of a shear.
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            double product = 0, orthogonal = 0;
            for (int k = 0; k < 3; k++) {
                const double a = rotation.get(row,k), inv = rotationInverse.get(k,column);
                const double x = rotation.get(k,row), y = rotation.get(k,column);
                if (!java::Double::isFinite(a) || !java::Double::isFinite(inv) ||
                    !java::Double::isFinite(x) || !java::Double::isFinite(y)) return QueryPlacementStatus::Invalid;
                product += a * inv;
                orthogonal += x * y;
            }
            const double expected = row == column ? 1.0 : 0.0;
            if (!java::Double::isFinite(product) || !java::Double::isFinite(orthogonal) ||
                java::Math::abs(product - expected) > 1e-8 ||
                java::Math::abs(orthogonal - expected) > 1e-8) return QueryPlacementStatus::Indeterminate;
        }
    }
    return QueryPlacementStatus::Valid;
}

RaySolidQueryStatus SimpleBody::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    result.clear();
    if (!supportsDecomposedPlacement() || geometry == nullptr) return RaySolidQueryStatus::Unsupported;
    const QueryPlacementStatus placement = queryPlacementStatus();
    if (placement == QueryPlacementStatus::Invalid) return RaySolidQueryStatus::InvalidInput;
    if (placement == QueryPlacementStatus::Indeterminate) return RaySolidQueryStatus::Indeterminate;
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();
    if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
        !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
        !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()))
        return RaySolidQueryStatus::InvalidInput;
    const Matrix4x4d childToParent = Matrix4x4d().translation(position)
        .multiply(rotation).multiply(Matrix4x4d().scale(scale));
    const Matrix4x4d parentToChild = Matrix4x4d().scale(inverseScale)
        .multiply(rotationInverse).multiply(Matrix4x4d().translation(position.multiply(-1)));
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            if (!java::Double::isFinite(childToParent.get(row,column)) ||
                !java::Double::isFinite(parentToChild.get(row,column))) return RaySolidQueryStatus::Indeterminate;
        }
    }
    const Vector3Dd localOrigin = parentToChild.multiply(origin);
    const Vector3Dd localDirection = parentToChild.withoutTranslation().multiply(direction);
    if (!java::Double::isFinite(localOrigin.x()) || !java::Double::isFinite(localOrigin.y()) ||
        !java::Double::isFinite(localOrigin.z()) || !java::Double::isFinite(localDirection.x()) ||
        !java::Double::isFinite(localDirection.y()) || !java::Double::isFinite(localDirection.z()))
        return RaySolidQueryStatus::Indeterminate;
    const Ray localRay = Ray::fromAffine(localOrigin, localDirection, ray.getT());
    const RaySolidQueryStatus status = geometry->classifyRay(localRay, query, result);
    if (status != RaySolidQueryStatus::Supported) { result.clear(); return status; }
    const RayInstanceStep step(this, childToParent, parentToChild);
    for (long i = 0; i < result.boundaries.size(); i++) {
        if (!result.boundaries[i].appendInstance(step)) {
            result.clear();
            return RaySolidQueryStatus::Indeterminate;
        }
    }
    return status;
}

GeometryContainment SimpleBody::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (!supportsDecomposedPlacement() || geometry == nullptr) return GeometryContainment::Unsupported;
    if (!java::Double::isFinite(tolerance) || tolerance < 0 ||
        !java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return GeometryContainment::InvalidInput;
    const QueryPlacementStatus placement = queryPlacementStatus();
    if (placement == QueryPlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement == QueryPlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
    const double sx = java::Math::abs(scale.x()), sy = java::Math::abs(scale.y()), sz = java::Math::abs(scale.z());
    if (tolerance > 0 && (sx != sy || sx != sz)) return GeometryContainment::Indeterminate;
    const double localTolerance = tolerance == 0 ? 0 : tolerance / sx;
    if (!java::Double::isFinite(localTolerance)) return GeometryContainment::Indeterminate;
    const Matrix4x4d inverse = Matrix4x4d().scale(inverseScale)
        .multiply(rotationInverse).multiply(Matrix4x4d().translation(position.multiply(-1)));
    const Vector3Dd localPoint = inverse.multiply(point);
    if (!java::Double::isFinite(localPoint.x()) || !java::Double::isFinite(localPoint.y()) ||
        !java::Double::isFinite(localPoint.z())) return GeometryContainment::Indeterminate;
    return geometry->classifyPoint(localPoint, localTolerance);
}
