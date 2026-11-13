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
    : geometry(0), ownsGeometry(true), geometryIsSphere(false), affinePlacement(), explicitAffinePlacement(false), position(0, 0, 0), scale(1, 1, 1), rotation(), rotationInverse(),
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

void SimpleBody::updateDecomposedPlacement()
{
    const Matrix4x4d forward = Matrix4x4d().translation(position)
        .multiply(rotation.multiply(Matrix4x4d().scale(scale)));
    const Matrix4x4d inverse = Matrix4x4d().scale(inverseScale)
        .multiply(rotationInverse).multiply(Matrix4x4d().translation(position.multiply(-1)));
    affinePlacement.assign(forward, inverse);
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

void SimpleBody::getWorldBounds(GeometryBounds& result) const
{
    Geometry* shape = getGeometry();
    if (shape == nullptr) { result.setEmpty(); return; }
    GeometryBounds local;
    shape->getBounds(local);
    result = local.transformed(getAffinePlacement());
}

void SimpleBody::getSolidWorldBounds(GeometryBounds& result) const
{
    if (typeid(*this)!=typeid(SimpleBody) || dynamic_cast<const Solid*>(getGeometry())==nullptr) {
        result.setUnsupported();
        return;
    }
    getWorldBounds(result);
}

GeometryContainment SimpleBody::classifyWorldBoxCertified(const GeometryBounds& box) const
{
    if (typeid(*this)!=typeid(SimpleBody) || geometry==nullptr) return GeometryContainment::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (placement==AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
    return classifyPlacedBoxCertified(*geometry,false,inverse,box);
}

GeometryContainment SimpleBody::classifyPlacedBoxCertified(const Geometry& geometry, bool inverted,
    const Matrix4x4d& inverse, const GeometryBounds& box)
{
    if (box.getKind()==GeometryBounds::Kind::Unsupported) return GeometryContainment::Unsupported;
    if (box.getKind()!=GeometryBounds::Kind::Finite) return GeometryContainment::Indeterminate;
    // The conservative local bounds of the box's preimage, as centre and
    // outward-rounded half widths.
    const GeometryBounds local=box.transformed(inverse);
    if (local.getKind()!=GeometryBounds::Kind::Finite) return GeometryContainment::Indeterminate;
    const double lo[3]={local.getMinimum().x(),local.getMinimum().y(),local.getMinimum().z()};
    const double hi[3]={local.getMaximum().x(),local.getMaximum().y(),local.getMaximum().z()};
    double centre[3], half[3];
    for (int i=0;i<3;++i) {
        centre[i]=lo[i]*0.5+hi[i]*0.5;
        const FloatingInterval below=FloatingInterval::exact(centre[i]).subtract(FloatingInterval::exact(lo[i]));
        const FloatingInterval above=FloatingInterval::exact(hi[i]).subtract(FloatingInterval::exact(centre[i]));
        if (!java::Double::isFinite(centre[i]) || !below.isValid() || !above.isValid())
            return GeometryContainment::Indeterminate;
        half[i]=java::Math::max(java::Math::max(below.getUpper(),above.getUpper()),0.0);
    }
    const GeometryContainment result=geometry.classifyBoxCertified(Vector3Dd(centre[0],centre[1],centre[2]),
        Vector3Dd(half[0],half[1],half[2]));
    if (inverted && result==GeometryContainment::Inside) return GeometryContainment::Outside;
    if (inverted && result==GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
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

RayCrossingSupport SimpleBody::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    if (geometry == nullptr) return RayCrossingSupport::Unsupported;
    Matrix4x4d forward, inverse;
    const AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
    if (status == AffinePlacementStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status == AffinePlacementStatus::Invalid) return RayCrossingSupport::InvalidInput;
    if (status == AffinePlacementStatus::Indeterminate) return RayCrossingSupport::Indeterminate;
    const Vector3Dd origin = inverse.multiply(ray.getOrigin());
    const Vector3Dd direction = inverse.withoutTranslation().multiply(ray.getDirection());
    if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
        !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
        !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()))
        return RayCrossingSupport::Indeterminate;
    const Ray local = Ray::fromAffine(origin, direction, ray.getT());
    InstanceRayCrossingBuffer buffer(RayInstanceStep(this,forward,inverse), this, getSurfaceMaterial());
    const RayCrossingSupport result = geometry->emitRayCrossings(local,query,buffer);
    if (result != RayCrossingSupport::Supported) return result;
    if (!buffer.isValid()) return RayCrossingSupport::Indeterminate;
    buffer.deliver(sink);
    return RayCrossingSupport::Supported;
}

RaySolidQueryStatus SimpleBody::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    result.clear();
    if (geometry == nullptr) return RaySolidQueryStatus::Unsupported;
    Matrix4x4d childToParent, parentToChild;
    const AffinePlacementStatus placement = queryAffinePlacement(childToParent, parentToChild);
    if (placement == AffinePlacementStatus::Unsupported) return RaySolidQueryStatus::Unsupported;
    if (placement == AffinePlacementStatus::Invalid) return RaySolidQueryStatus::InvalidInput;
    if (placement == AffinePlacementStatus::Indeterminate) return RaySolidQueryStatus::Indeterminate;
    const Vector3Dd& origin = ray.getOrigin();
    const Vector3Dd& direction = ray.getDirection();
    if (!java::Double::isFinite(origin.x()) || !java::Double::isFinite(origin.y()) ||
        !java::Double::isFinite(origin.z()) || !java::Double::isFinite(direction.x()) ||
        !java::Double::isFinite(direction.y()) || !java::Double::isFinite(direction.z()))
        return RaySolidQueryStatus::InvalidInput;
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

GeometryDistanceStatus SimpleBody::encloseMinimumBoundaryDistance(const Vector3Dd& point,
    double maxWidth, GeometryContainment& side, double& lower, double& upper) const
{
    if (!geometryIsSphere) return GeometryDistanceStatus::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus status=queryAffinePlacement(forward,inverse);
    if (status==AffinePlacementStatus::Invalid) return GeometryDistanceStatus::InvalidInput;
    if (status==AffinePlacementStatus::Unsupported) return GeometryDistanceStatus::Unsupported;
    if (status==AffinePlacementStatus::Indeterminate) return GeometryDistanceStatus::Indeterminate;
    return AffineEllipsoidMinimumDistance::query(point,static_cast<const Sphere*>(geometry)->getRadius(),
        forward,inverse,maxWidth,side,lower,upper);
}

GeometryDistanceStatus SimpleBody::encloseBoundaryDistance(const Vector3Dd& point,
    const Vector3Dd& localDirection, GeometryContainment& side, double& lower, double& upper) const
{
    if (!geometryIsSphere) return GeometryDistanceStatus::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus status=queryAffinePlacement(forward,inverse);
    if (status==AffinePlacementStatus::Invalid) return GeometryDistanceStatus::InvalidInput;
    if (status==AffinePlacementStatus::Unsupported) return GeometryDistanceStatus::Unsupported;
    if (status==AffinePlacementStatus::Indeterminate) return GeometryDistanceStatus::Indeterminate;
    return AffineEllipsoidDistanceBounds::query(point,static_cast<const Sphere*>(geometry)->getRadius(),
        forward,localDirection,side,lower,upper);
}

const Geometry* SimpleBody::getSolidEquivalenceGeometry() const
{
    return typeid(*this)==typeid(SimpleBody) ? getGeometry() : nullptr;
}

GeometryContainment SimpleBody::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    if (geometry==nullptr) return GeometryContainment::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (placement==AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
    return classifyPlacedPointCertified(*geometry,false,forward,inverse,point,tolerance);
}

GeometryContainment SimpleBody::classifyPlacedPointCertified(const Geometry& geometry, bool inverted,
    const Matrix4x4d& forward, const Matrix4x4d& inverse, const Vector3Dd& point, double tolerance)
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    const Sphere* sphere=dynamic_cast<const Sphere*>(&geometry);
    const GeometryContainment result=sphere ?
        classifyPlacedSphereCertified(sphere->getRadius(),forward,inverse,point,tolerance,128) :
        classifyPlacedSolidCertified(geometry,forward,point,tolerance);
    if (inverted && result==GeometryContainment::Inside) return GeometryContainment::Outside;
    if (inverted && result==GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}

GeometryContainment SimpleBody::classifyPlacedSolidCertified(const Geometry& geometry,
    const Matrix4x4d& forward, const Vector3Dd& point, double tolerance)
{
    bool identity=true;
    for (int row=0;row<4;++row) for (int column=0;column<4;++column)
        if (forward.get(row,column)!=(row==column ? 1.0 : 0.0)) identity=false;
    if (identity) return geometry.classifyPointCertified(point,tolerance);
    Vector3Dd local, halfWidths;
    double outer=0,inner=-1;
    const AffinePlacementStatus mapped=AffineToleranceMap::map(forward,point,tolerance,local,outer,&inner,&halfWidths);
    if (mapped==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (mapped==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (mapped!=AffinePlacementStatus::Valid) return GeometryContainment::Indeterminate;
    // The axis-aligned local box also contains the preimage and is tighter
    // under anisotropic placement; it can only prove Inside or Outside.
    const GeometryContainment region=geometry.classifyBoxCertified(local,halfWidths);
    if (region==GeometryContainment::Inside || region==GeometryContainment::Outside ||
        region==GeometryContainment::InvalidInput) return region;
    // The outer local ball contains the preimage of the world ball, so a
    // definite side for it is a definite world side. A zero outer radius means
    // the preimage is exactly the local point.
    const GeometryContainment result=geometry.classifyPointCertified(local,outer);
    if (result==GeometryContainment::Inside || result==GeometryContainment::Outside ||
        result==GeometryContainment::InvalidInput || result==GeometryContainment::Unsupported) return result;
    if (result==GeometryContainment::Boundary && outer==0) return result;
    // A boundary inside the inner local ball maps inside the world ball.
    if (inner>0) {
        const GeometryContainment near=geometry.classifyPointCertified(local,inner);
        if (near==GeometryContainment::Boundary || near==GeometryContainment::InvalidInput ||
            near==GeometryContainment::Unsupported) return near;
    }
    if (!(tolerance>0)) return GeometryContainment::Indeterminate;
    // Under anisotropic placement the inner ball is small. Two world points of
    // the (convex) tolerance ball with certified opposite membership prove a
    // boundary point on the segment joining them.
    const GeometryContainment centre=classifyPlacedSolidCertified(geometry,forward,point,0);
    int centreSign=centre==GeometryContainment::Inside ? -1 : centre==GeometryContainment::Outside ? 1 : 0;
    bool foundInside=centreSign<0, foundOutside=centreSign>0;
    const double step=tolerance*(1.0-1.0/1048576.0);
    const double diagonal=step/java::Math::sqrt(3.0)*(1.0-1.0/1048576.0);
    for (int direction=0;direction<14;++direction) {
        Vector3Dd offset;
        if (direction<6) {
            const double sign=direction%2==0 ? 1.0 : -1.0;
            offset=Vector3Dd(direction/2==0 ? sign*step : 0, direction/2==1 ? sign*step : 0,
                direction/2==2 ? sign*step : 0);
        } else {
            const int corner=direction-6;
            offset=Vector3Dd(corner&1 ? -diagonal : diagonal, corner&2 ? -diagonal : diagonal,
                corner&4 ? -diagonal : diagonal);
        }
        const Vector3Dd sample=point.add(offset);
        FloatingInterval squared=FloatingInterval::exact(0);
        squared=squared.add(FloatingInterval::exact(sample.x()).subtract(FloatingInterval::exact(point.x())).square());
        squared=squared.add(FloatingInterval::exact(sample.y()).subtract(FloatingInterval::exact(point.y())).square());
        squared=squared.add(FloatingInterval::exact(sample.z()).subtract(FloatingInterval::exact(point.z())).square());
        const FloatingInterval distance=squared.squareRoot();
        if (!distance.isValid() || distance.getUpper()>tolerance) continue;
        const GeometryContainment side=classifyPlacedSolidCertified(geometry,forward,sample,0);
        if (side==GeometryContainment::Inside) foundInside=true;
        else if (side==GeometryContainment::Outside) foundOutside=true;
        else if (side==GeometryContainment::Boundary) return GeometryContainment::Boundary;
        if (foundInside && foundOutside) return GeometryContainment::Boundary;
    }
    return GeometryContainment::Indeterminate;
}

GeometryContainment SimpleBody::classifyPlacedSphereCertified(double radius,
    const Matrix4x4d& forward, const Matrix4x4d& inverse, const Vector3Dd& point,
    double tolerance, int iterationLimit)
{
    Vector3Dd seed=inverse.multiply(point);
    if (!java::Double::isFinite(seed.x()) || !java::Double::isFinite(seed.y()) ||
        !java::Double::isFinite(seed.z())) return GeometryContainment::Indeterminate;
    const bool center=(point.x()==forward.get(0,3) && point.y()==forward.get(1,3) && point.z()==forward.get(2,3)) ||
        (seed.x()==0 && seed.y()==0 && seed.z()==0);
    const Vector3Dd axes[]={Vector3Dd(1,0,0),Vector3Dd(0,1,0),Vector3Dd(0,0,1)};
    double forwardNormSquared=0;
    for (int i=0;i<3;++i) for (int j=0;j<3;++j)
        forwardNormSquared+=forward.get(i,j)*forward.get(i,j);
    const double lipschitz=radius*radius*forwardNormSquared;
    const int seedCount=center ? 3 : 1;
    for (int candidate=0;candidate<seedCount;++candidate) {
      if (center) seed=axes[candidate];
      for (int iteration=0;iteration<iterationLimit;++iteration) {
        if (iteration==0 || iteration%4==0 || iteration+1==iterationLimit) {
            GeometryContainment side=GeometryContainment::Indeterminate;
            double lower=0,upper=0;
            const GeometryDistanceStatus status=AffineEllipsoidDistanceBounds::query(point,
                radius,forward,seed,side,lower,upper,iteration==0 && candidate==0);
            if (status==GeometryDistanceStatus::InvalidInput) return GeometryContainment::InvalidInput;
            if (status==GeometryDistanceStatus::Unsupported) return GeometryContainment::Unsupported;
            if (status!=GeometryDistanceStatus::Valid) return GeometryContainment::Indeterminate;
            if (upper<=tolerance) return GeometryContainment::Boundary;
            if (lower>tolerance) return side;
        }
        if (!(lipschitz>0) || !java::Double::isFinite(lipschitz)) break;
        const double seedLength=seed.length();
        if (!(seedLength>0) || !java::Double::isFinite(seedLength)) break;
        const Vector3Dd unit=seed.multiply(1.0/seedLength);
        const Vector3Dd delta=forward.multiply(unit.multiply(radius)).subtract(point);
        const double components[3]={delta.x(),delta.y(),delta.z()};
        double gradient[3]={0,0,0};
        for (int i=0;i<3;++i) for (int j=0;j<3;++j)
            gradient[i]+=radius*forward.get(j,i)*components[j];
        const Vector3Dd g(gradient[0],gradient[1],gradient[2]);
        const Vector3Dd tangent=g.subtract(unit.multiply(g.dotProduct(unit)));
        if (tangent.dotProduct(tangent)==0) break;
        seed=unit.subtract(tangent.multiply(1.0/lipschitz));
        if (!java::Double::isFinite(seed.x()) || !java::Double::isFinite(seed.y()) ||
            !java::Double::isFinite(seed.z())) break;
      }
    }
    // If surface-directed search cannot decide, use a separately certified
    // minimum-distance enclosure. The numerical estimate only proposes its
    // witness; interval bounds, never the estimate, determine this result.
    const double requestedWidth=tolerance*1e-3;
    if (requestedWidth>0 && java::Double::isFinite(requestedWidth)) {
        GeometryContainment side=GeometryContainment::Indeterminate;
        double lower=0,upper=0;
        const GeometryDistanceStatus status=AffineEllipsoidMinimumDistance::query(
            point,radius,forward,inverse,requestedWidth,side,lower,upper);
        if (status==GeometryDistanceStatus::InvalidInput) return GeometryContainment::InvalidInput;
        if (status==GeometryDistanceStatus::Unsupported) return GeometryContainment::Unsupported;
        if (status==GeometryDistanceStatus::Valid) {
            if (upper<=tolerance) return GeometryContainment::Boundary;
            if (lower>tolerance) return side;
        }
    }
    return GeometryContainment::Indeterminate;
}

GeometryDistanceStatus SimpleBody::estimateBoundaryDistance(const Vector3Dd& point,
    double& distance, double& estimatedError) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return GeometryDistanceStatus::InvalidInput;
    if (!geometryIsSphere) return GeometryDistanceStatus::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus status=queryAffinePlacement(forward,inverse);
    if (status==AffinePlacementStatus::Invalid) return GeometryDistanceStatus::InvalidInput;
    if (status==AffinePlacementStatus::Unsupported) return GeometryDistanceStatus::Unsupported;
    if (status==AffinePlacementStatus::Indeterminate) return GeometryDistanceStatus::Indeterminate;
    return AffineEllipsoidDistance::query(point,static_cast<const Sphere*>(geometry)->getRadius(),
        forward,distance,estimatedError);
}

GeometryContainment SimpleBody::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (geometry == nullptr) return GeometryContainment::Unsupported;
    if (!java::Double::isFinite(tolerance) || tolerance < 0 ||
        !java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return GeometryContainment::InvalidInput;
    if (tolerance == 0) {
        Matrix4x4d forward, inverse;
        const AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
        if (status == AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
        if (status == AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
        if (status == AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
        const Vector3Dd local = inverse.multiply(point);
        if (!java::Double::isFinite(local.x()) || !java::Double::isFinite(local.y()) ||
            !java::Double::isFinite(local.z())) return GeometryContainment::Indeterminate;
        return geometry->classifyPoint(local, 0);
    }
    return classifyPointCertified(point, tolerance);
}
