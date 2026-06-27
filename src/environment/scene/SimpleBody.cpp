#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/scene/SimpleBody.h"
#include "environment/material/Material.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"

bool
SimpleBody::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (doIntersectionForAllRayCrossings(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}

int
SimpleBody::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)materialOverride;
    bool intersectionFound;
    bool anyIntersectionFound;
    IntersectionCandidate localIntersection;
    TransformedGeometry *boundingShape;
    TransformedGeometry *clippingShape;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    Statistics &stats = *ray->getStatistics();
    RayWithSegments localRay = *ray;
    RayWithSegments *geometryRay = ray;

    if (transformationInverse != nullptr) {
        localRay.setOrigin(transformationInverse->transformPoint(ray->getOrigin()));
        localRay.setDirection(transformationInverse->transformDirection(ray->getDirection()));
        localRay.setQuadricConstantsCached(false);
        geometryRay = &localRay;
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(geometryRay->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            IntersectionCandidate _boundingHit;
            if (!boundingShape->doIntersectionFirstHit(geometryRay, _boundingHit) &&
                boundingShape->doContainmentTest(rayOrigin, Config::SMALL_TOLERANCE) ==
                    Geometry::OUTSIDE) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;
    this->getGeometry()->doIntersectionForAllRayCrossings(
        geometryRay, localDepthQueue, this->getGeometryMaterial());

    for (const IntersectionCandidate& candidate : *localDepthQueue) {
        localIntersection = candidate;

        localIntersection.getAttributes().setObjectTexture(this->getObjectTexture());
        localIntersection.getAttributes().setObjectColor(this->getObjectColor());
        localIntersection.getAttributes().setNoShadowFlag(this->getNoShadowFlag());
        localIntersection.getAttributes().setHitBody(this);
        if (transformation != nullptr) {
            localIntersection.getIntersection().point =
                transformation->transformPoint(localIntersection.getIntersection().point);
        }
        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];

            stats.incrementClippingRegionTests();
            if (clippingShape->doContainmentTest(
                    worldPointToLocal(localIntersection.getIntersection().point),
                    Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
                intersectionFound = false;
                break;
            }
            stats.incrementClippingRegionTestsSucceeded();
        }

        if (intersectionFound) {
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return (anyIntersectionFound);
}

int
SimpleBody::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    TransformedGeometry *boundingShape;
    TransformedGeometry *clippingShape;
    const Vector3Dd localPoint = worldPointToLocal(point);

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(localPoint, distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(localPoint, distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    if (this->getGeometry()->doContainmentTest(localPoint, distanceTolerance) != Geometry::OUTSIDE) {
        return Geometry::INSIDE;
    }
    return Geometry::OUTSIDE;
}

void
SimpleBody::doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit)
{
    Geometry *detailGeometry = hit->hitGeometry != nullptr ? hit->hitGeometry : geometry;
    if (detailGeometry != nullptr) {
        if (transformationInverse == nullptr) {
            detailGeometry->doExtraInformation(ray, t, hit);
            return;
        }

        RayWithSegments localRay = ray;
        localRay.setOrigin(transformationInverse->transformPoint(ray.getOrigin()));
        localRay.setDirection(transformationInverse->transformDirection(ray.getDirection()));
        localRay.setQuadricConstantsCached(false);

        const Vector3Dd worldPoint = hit->p;
        hit->p = transformationInverse->transformPoint(worldPoint);
        detailGeometry->doExtraInformation(localRay, t, hit);
        hit->n = localNormalToWorld(hit->n);
        hit->p = worldPoint;
    }
}

SimpleBody::SimpleBody(const SimpleBody &other) :
    geometry(other.getGeometry() != nullptr ?
        (TransformedGeometry *)other.getGeometry()->copy() : nullptr),
    geometryMaterial(other.getGeometryMaterial() != nullptr ?
        other.getGeometryMaterial()->copy() : nullptr),
    transformation(other.getTransformation() != nullptr ?
        new Matrix4x4d(*other.getTransformation()) : nullptr),
    transformationInverse(other.getTransformationInverse() != nullptr ?
        new Matrix4x4d(*other.getTransformationInverse()) : nullptr),
    noShadowFlag(other.getNoShadowFlag()),
    objectColor(other.getObjectColor() != nullptr ?
        new ColorRgba(*other.getObjectColor()) : nullptr),
    objectTexture(other.getObjectTexture() != nullptr ?
        other.getObjectTexture()->copy() : nullptr)
{
    for (long int i = other.getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShapes.add(
            (TransformedGeometry *)other.getBoundingShapes()[i]->copy());
    }
    for (long int i = other.getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShapes.add(
            (TransformedGeometry *)other.getClippingShapes()[i]->copy());
    }
}

SimpleBody::~SimpleBody()
{
    delete geometry;
    delete geometryMaterial;
    delete transformation;
    delete transformationInverse;
    delete objectColor;
    // objectTexture may be a private clone (delete it) or an alias to a shared
    // constant such as the scene's default texture (do not delete, just close
    // out its alias bookkeeping). releaseFromOwner() encapsulates that decision
    // so this destructor needs to know nothing about the concrete material type.
    if (objectTexture != nullptr) {
        objectTexture->releaseFromOwner();
    }
    for (long int i = 0; i < boundingShapes.size(); i++) {
        delete boundingShapes[i];
    }
    for (long int i = 0; i < clippingShapes.size(); i++) {
        delete clippingShapes[i];
    }
}

void
SimpleBody::ensureMatrices()
{
    if (transformation == nullptr) {
        transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

void
SimpleBody::applyTranslationToBodyTransform(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
}

void
SimpleBody::applyRotationToBodyTransform(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
}

void
SimpleBody::applyScaleToBodyTransform(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
}

Vector3Dd
SimpleBody::worldPointToLocal(const Vector3Dd &point) const
{
    return transformationInverse != nullptr ?
        transformationInverse->transformPoint(point) : point;
}

Vector3Dd
SimpleBody::localPointToWorld(const Vector3Dd &point) const
{
    return transformation != nullptr ? transformation->transformPoint(point) : point;
}

Vector3Dd
SimpleBody::worldDirectionToLocal(const Vector3Dd &direction) const
{
    return transformationInverse != nullptr ?
        transformationInverse->transformDirection(direction) : direction;
}

Vector3Dd
SimpleBody::localNormalToWorld(const Vector3Dd &normal) const
{
    if (transformationInverse == nullptr) {
        return normal;
    }
    return transformationInverse->withoutTranslation().multiply(normal).normalizedFast();
}

void
SimpleBody::detachOwnership()
{
    geometry = nullptr;
    geometryMaterial = nullptr;
    transformation = nullptr;
    transformationInverse = nullptr;
    objectColor = nullptr;
    objectTexture = nullptr;
    boundingShapes.clear();
    clippingShapes.clear();
}

void *
SimpleBody::copy()
{
    return new SimpleBody(*this);
}

void
SimpleBody::translate(Vector3Dd *vector)
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->translateGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->translateGeometry(vector);
    }

    if (this->getGeometry() != nullptr) {
        this->getGeometry()->translateGeometry(vector);
    }

    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->translate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->translate(vector);
    }
}

void
SimpleBody::rotate(Vector3Dd *vector)
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->rotateGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->rotateGeometry(vector);
    }

    if (this->getGeometry() != nullptr) {
        this->getGeometry()->rotateGeometry(vector);
    }

    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->rotate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->rotate(vector);
    }
}

void
SimpleBody::scale(Vector3Dd *vector)
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->scaleGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->scaleGeometry(vector);
    }

    if (this->getGeometry() != nullptr) {
        this->getGeometry()->scaleGeometry(vector);
    }

    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->scale(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->scale(vector);
    }
}

void
SimpleBody::invert()
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invertGeometry();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invertGeometry();
    }
    if (this->getGeometry() != nullptr) {
        this->getGeometry()->invertGeometry();
    }
}

AxisAlignedBox
SimpleBody::getAABB() const
{
    // If bounding shapes are present they define the visible extent; use the
    // intersection of their AABBs (a tighter world-space bound than the geometry alone).
    if (boundingShapes.size() > 0) {
        AxisAlignedBox result = AxisAlignedBox::unbounded();
        for (long int i = 0; i < boundingShapes.size(); i++) {
            result = result.intersection(boundingShapes[i]->getMinMax());
        }
        return result;
    }
    if (geometry != nullptr) {
        AxisAlignedBox box = geometry->getMinMax();
        if (transformation != nullptr && !box.isUnbounded()) {
            return AxisAlignedBox::fromTransformedCorners(
                box.min, box.max, transformation);
        }
        return box;
    }
    return AxisAlignedBox::unbounded();
}
