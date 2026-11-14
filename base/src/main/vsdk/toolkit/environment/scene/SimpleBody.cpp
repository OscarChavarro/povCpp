#include <typeinfo>
#include <cfloat>
#include <cmath>

#include "java/lang/String.h"
#include "java/lang/IllegalStateException.h"
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
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/SurfaceRayIntersection.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "vsdk/toolkit/environment/scene/AffineToleranceMap.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.h"
#include "vsdk/toolkit/environment/scene/InstanceRayCrossingBuffer.h"
SimpleBody::SimpleBody()
    : SceneSolid(), affinePlacement(), explicitAffinePlacement(false), position(0, 0, 0), scale(1, 1, 1), rotation(), rotationInverse(),
      rotationQuaternion(rotation.exportToQuaternion()), rotationInverseQuaternion(rotationInverse.exportToQuaternion()),
      inverseScale(1, 1, 1), hasInvertibleScale(true),
      hasIdentityRotation(true), hasUnitScale(true), hasZeroTranslation(true),
      hasTranslationOnlyTransform(true), hasIdentityTransform(true),
      material(0), ownsMaterial(true), texture(0), ownsTexture(true), normalMap(0), ownsNormalMap(true), normalMapRgb(0)
{
}

SimpleBody::~SimpleBody()
{
    delete normalMapRgb;
    if ( material != 0 ) {
        if ( ownsMaterial ) {
            delete material;
        }
        material = 0;
    }
    if ( texture != 0 && ownsTexture ) {
        delete texture;
    }
    texture = 0;
    if ( normalMap != 0 && ownsNormalMap ) {
        delete normalMap;
    }
    normalMap = 0;
}

bool SimpleBody::denotesPlacedGeometry() const { return typeid(*this)==typeid(SimpleBody); }

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

void SimpleBody::updateDecomposedPlacement()
{
    const Matrix4x4d forward = Matrix4x4d().translation(position)
        .multiply(rotation.multiply(Matrix4x4d().scale(scale)));
    const Matrix4x4d inverse = Matrix4x4d().scale(inverseScale)
        .multiply(rotationInverse).multiply(Matrix4x4d().translation(position.multiply(-1)));
    affinePlacement.assign(forward, inverse);
}

Matrix4x4d SimpleBody::getRotation() const {
    if (explicitAffinePlacement) throw java::IllegalStateException("Rotation is not defined for an arbitrary affine placement");
    return rotation;
}
void SimpleBody::setRotation(const Matrix4x4d& r)
{
    if (explicitAffinePlacement) throw java::IllegalStateException("Cannot decompose an arbitrary affine placement");
    Matrix4x4d sanitized = r.withoutTranslation();
    rotationQuaternion = sanitized.exportToQuaternion().normalized();
    rotation = sanitized;
    rotationInverseQuaternion = rotationQuaternion.conjugated();
    rotationInverse = Matrix4x4d().importFromQuaternion(rotationInverseQuaternion);
    updateTransformFlags();
    updateDecomposedPlacement();
    markModified();
}

Matrix4x4d SimpleBody::getRotationInverse() const {
    if (explicitAffinePlacement) throw java::IllegalStateException("Rotation is not defined for an arbitrary affine placement");
    return rotationInverse;
}
void SimpleBody::setRotationInverse(const Matrix4x4d& ri)
{
    if (explicitAffinePlacement) throw java::IllegalStateException("Cannot decompose an arbitrary affine placement");
    Matrix4x4d sanitized = ri.withoutTranslation();
    rotationInverseQuaternion = sanitized.exportToQuaternion().normalized();
    rotationInverse = sanitized;
    rotationQuaternion = rotationInverseQuaternion.conjugated();
    rotation = Matrix4x4d().importFromQuaternion(rotationQuaternion);
    updateTransformFlags();
    updateDecomposedPlacement();
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
    if ( texture != in && texture != 0 && ownsTexture ) {
        delete texture;
    }
    texture = in;
    ownsTexture = true;
    markModified();
}
void SimpleBody::setTextureReference(Image* in)
{
    if ( texture != in && texture != 0 && ownsTexture ) {
        delete texture;
    }
    texture = in;
    ownsTexture = false;
    markModified();
}
NormalMap* SimpleBody::getNormalMap() const { return normalMap; }
RGBImageUncompressed* SimpleBody::getNormalMapRgb() const { return normalMapRgb; }

void SimpleBody::setNormalMap(NormalMap* in)
{
    if ( normalMap != in && normalMap != 0 && ownsNormalMap ) {
        delete normalMap;
    }
    normalMap = in;
    ownsNormalMap = true;
    delete normalMapRgb;
    normalMapRgb = in ? new NormalMapTexture(*in) : 0;
    markModified();
}
void SimpleBody::setNormalMapReference(NormalMap* in)
{
    if ( normalMap != in && normalMap != 0 && ownsNormalMap ) {
        delete normalMap;
    }
    normalMap = in;
    ownsNormalMap = false;
    delete normalMapRgb;
    normalMapRgb = in ? new NormalMapTexture(*in) : 0;
    markModified();
}

Vector3Dd SimpleBody::getPosition() const {
    return explicitAffinePlacement ? affinePlacement.getForward().extractTranslation() : position;
}
void SimpleBody::setPosition(const Vector3Dd& p) {
    if (explicitAffinePlacement) {
        const Matrix4x4d forward = affinePlacement.getForward().withTranslation(p);
        const Vector3Dd inverseTranslation = affinePlacement.getInverse().withoutTranslation().multiply(p).multiply(-1);
        const Matrix4x4d inverse = affinePlacement.getInverse().withTranslation(inverseTranslation);
        AffinePlacement candidate;
        if (candidate.assign(forward,inverse) != AffinePlacementStatus::Valid)
            throw java::IllegalStateException("Invalid affine translation");
        affinePlacement = candidate;
        markModified();
        return;
    }
    position = p; updateTransformFlags(); updateDecomposedPlacement(); markModified();
}
Vector3Dd SimpleBody::getScale() const {
    if (explicitAffinePlacement) throw java::IllegalStateException("Scale is not defined for an arbitrary affine placement");
    return scale;
}
Matrix4x4d SimpleBody::getTransformationMatrix() const { return affinePlacement.getForward(); }
Matrix4x4d SimpleBody::getAffinePlacement() const { return affinePlacement.getForward(); }
AffinePlacementStatus SimpleBody::setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse)
{
    AffinePlacement candidate;
    const AffinePlacementStatus status = candidate.assign(forward,inverse);
    if (status != AffinePlacementStatus::Valid) return status;
    affinePlacement = candidate;
    explicitAffinePlacement = true;
    markModified();
    return status;
}

void SimpleBody::resetAffinePlacement()
{
    explicitAffinePlacement = false;
    position = Vector3Dd(); scale = Vector3Dd(1,1,1); inverseScale = Vector3Dd(1,1,1);
    rotation = Matrix4x4d::identityMatrix(); rotationInverse = rotation;
    rotationQuaternion = rotation.exportToQuaternion(); rotationInverseQuaternion = rotationQuaternion;
    hasInvertibleScale = true;
    updateTransformFlags(); updateDecomposedPlacement(); markModified();
}

void SimpleBody::setScale(const Vector3Dd& s)
{
    if (explicitAffinePlacement) throw java::IllegalStateException("Cannot decompose an arbitrary affine placement");
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
    updateDecomposedPlacement();
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
    if ( geometry == 0 || (!explicitAffinePlacement && !hasInvertibleScale) ) {
        return false;
    }
    if (explicitAffinePlacement) return doIntersectionWithAffinePlacement(inOutRay, outHit);

    const int requestedDetailMask = outHit != 0 ? outHit->getRequiredDetailMask() : RayHit::DETAIL_NONE;

    if ( hasTranslationOnlyTransform && requestedDetailMask == RayHit::DETAIL_NONE && hasSphereGeometry() ) {
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

bool SimpleBody::doIntersectionWithAffinePlacement(const Ray& ray, RayHit* outHit) const
{
    const Matrix4x4d& inverse = affinePlacement.getInverse();
    const Vector3Dd origin = inverse.multiply(ray.getOrigin());
    const Vector3Dd direction = inverse.withoutTranslation().multiply(ray.getDirection());
    const double length = direction.length();
    if (!java::Double::isFinite(length) || length <= VSDK::EPSILON) return false;
    double localT = ray.getT();
    if (localT >= DBL_MAX / length) localT = DBL_MAX; else localT *= length;
    const Ray localRay(origin, direction, localT);
    RayHit localHit(RayHit::DETAIL_NONE, false);
    if (!SurfaceRayIntersection::doIntersectionFirstHit(geometry, localRay, &localHit)) return false;
    const double distance = localHit.getRay() ? localHit.getRay()->getT() : localHit.getHitDistance();
    if (!localHit.getRay() && !localHit.hasHitDistance()) return false;
    const double worldT = distance / length;
    if (!java::Double::isFinite(worldT)) return false;
    if (outHit) {
        if (outHit->shouldStoreRay() || outHit->needsAnySurfaceData()) {
            const Ray worldRay = ray.withT(worldT);
            outHit->setRay(worldRay);
            if (outHit->needsAnySurfaceData()) doExtraInformationWithAffinePlacement(worldRay, worldT, outHit);
        } else outHit->setHitDistance(worldT);
    }
    return true;
}

void SimpleBody::doExtraInformationWithAffinePlacement(const Ray& ray, double t, RayHit* hit) const
{
    const Matrix4x4d& forward = affinePlacement.getForward();
    const Matrix4x4d& inverse = affinePlacement.getInverse();
    const Vector3Dd origin = inverse.multiply(ray.getOrigin());
    const Vector3Dd rawDirection = inverse.withoutTranslation().multiply(ray.getDirection());
    const double length = rawDirection.length();
    if (!java::Double::isFinite(length) || length <= VSDK::EPSILON) return;
    const Ray localRay(origin, rawDirection, t*length);
    geometry->doExtraInformation(localRay, t*length, hit);
    if (hit->needsPoint()) hit->point = forward.multiply(hit->point);
    if (hit->needsNormal()) {
        const Vector3Dd n = hit->normal;
        hit->normal = Vector3Dd(
            inverse.get(0,0)*n.x()+inverse.get(1,0)*n.y()+inverse.get(2,0)*n.z(),
            inverse.get(0,1)*n.x()+inverse.get(1,1)*n.y()+inverse.get(2,1)*n.z(),
            inverse.get(0,2)*n.x()+inverse.get(1,2)*n.y()+inverse.get(2,2)*n.z()).normalized();
    }
    if (hit->needsTangent()) hit->tangent = forward.withoutTranslation().multiply(hit->tangent).normalized();
    hit->material = material; hit->texture = texture; hit->normalMap = normalMap;
    hit->setRay(ray.withT(t));
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
    if ( geometry == 0 || (!explicitAffinePlacement && !hasInvertibleScale) ) {
        return 0;
    }
    if (explicitAffinePlacement) return geometry->computeQuantitativeInvisibility(
        affinePlacement.getInverse().multiply(origin), affinePlacement.getInverse().multiply(p));

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
    if ( outData == 0 || geometry == 0 || (!explicitAffinePlacement && !hasInvertibleScale) ) return;
    if (explicitAffinePlacement) {
        if (outData->getRay()) doExtraInformationWithAffinePlacement(*outData->getRay(), outData->getRay()->getT(), outData);
        return;
    }

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

AffinePlacementStatus SimpleBody::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{
    if (explicitAffinePlacement) {
        forward = affinePlacement.getForward(); inverse = affinePlacement.getInverse();
        return affinePlacement.getStatus();
    }
    const QueryPlacementStatus status = queryPlacementStatus();
    if (status == QueryPlacementStatus::Invalid) return AffinePlacementStatus::Invalid;
    if (status == QueryPlacementStatus::Indeterminate) return AffinePlacementStatus::Indeterminate;
    forward = affinePlacement.getForward(); inverse = affinePlacement.getInverse();
    return affinePlacement.getStatus();
}

