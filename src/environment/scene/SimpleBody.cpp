#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/scene/SimpleBody.h"
#include "environment/material/Material.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"

bool
SimpleBody::doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out)
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
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    SimpleBody *boundingShape;
    SimpleBody *clippingShape;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    GeometryStatistics &stats = *ray->getGeometryStatistics();

    // The body of the intersection routine, parameterized on the ray already
    // expressed in this body's object-local space. Factored into a lambda so the
    // object-local clone below is built only when a transform actually exists:
    // an untransformed body passes the parent ray straight through with no
    // RayWithTracingState construction at all, and RAII still destroys the clone
    // across the early bounding-shape rejection return.
    auto intersectInObjectSpace = [&](RayWithTracingState *objectRay) -> int {
    RayWithTracingState geometryLocalRay = RayWithTracingState::localIntersectionClone(*objectRay);
    bool hasGeometryLocalSpace = geometryTransformationInverse != nullptr;
    if (hasGeometryLocalSpace) {
        geometryLocalRay.setOrigin(
            geometryTransformationInverse->transformPoint(objectRay->getOrigin()));
        geometryLocalRay.setDirection(
            geometryTransformationInverse->transformDirection(objectRay->getDirection()));
        geometryLocalRay.setQuadricConstantsCached(false);
    }
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(objectRay->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            IntersectionCandidate _boundingHit;
            if (!boundingShape->doIntersectionFirstHitViaCrossings(objectRay, _boundingHit) &&
                boundingShape->doContainmentTest(rayOrigin, GeometryConfig::SMALL_TOLERANCE) ==
                    Geometry::OUTSIDE) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;
    Material *effectiveGeometryMaterial =
        this->getGeometryMaterial() != nullptr ?
            this->getGeometryMaterial() : materialOverride;
    if (hasGeometryLocalSpace) {
        this->getGeometry()->doIntersectionForAllRayCrossings(
            &geometryLocalRay, localDepthQueue, effectiveGeometryMaterial);
    } else {
        this->getGeometry()->doIntersectionForAllRayCrossings(
            objectRay, localDepthQueue, effectiveGeometryMaterial);
    }

    // Mutate each crossing in place in the scratch queue (drained and recycled
    // right after the loop) rather than copying the ~168-byte candidate into
    // localIntersection and then again into depthQueue.
    for (IntersectionCandidate& localIntersection : *localDepthQueue) {
        localIntersection.getAttributes().setObjectTexture(this->getObjectTexture());
        localIntersection.getAttributes().setObjectColor(this->getObjectColor());
        localIntersection.getAttributes().setNoShadowFlag(this->getNoShadowFlag());
        localIntersection.getAttributes().setHitBody(this);
        if (geometryTransformation != nullptr) {
            localIntersection.getIntersection().point =
                geometryTransformation->transformPoint(
                    localIntersection.getIntersection().point);
        }
        const Vector3Dd objectLocalPoint = localIntersection.getIntersection().point;
        if (transformation != nullptr) {
            localIntersection.getIntersection().point =
                transformation->transformPoint(localIntersection.getIntersection().point);
        }
        // Re-express the (now world-space) crossing as a signed ray parameter
        // along the original ray, not a bare distance: length() would force
        // every crossing positive, so any crossing behind the ray origin -
        // exactly the case for refracted/internal rays whose origin sits on or
        // inside the body - would sort in front and corrupt the CSG in/out
        // classification. The signed projection equals the geometry's native
        // t for any ray direction.
        {
            const Vector3Dd rayDir = ray->getDirection();
            localIntersection.getIntersection().t =
                localIntersection.getIntersection().point
                    .subtract(ray->getOrigin()).dotProduct(rayDir) /
                rayDir.dotProduct(rayDir);
        }
        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];

            stats.incrementClippingRegionTests();
            if (clippingShape->doContainmentTest(
                    objectLocalPoint,
                    GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
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
    };

    if (transformationInverse != nullptr) {
        RayWithTracingState localRay = RayWithTracingState::localIntersectionClone(*ray);
        localRay.setOrigin(transformationInverse->transformPoint(ray->getOrigin()));
        localRay.setDirection(transformationInverse->transformDirection(ray->getDirection()));
        localRay.setQuadricConstantsCached(false);
        return intersectInObjectSpace(&localRay);
    }
    return intersectInObjectSpace(ray);
}

int
SimpleBody::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    SimpleBody *boundingShape;
    SimpleBody *clippingShape;
    const Vector3Dd localPoint = worldPointToLocal(point);

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(
                localPoint, distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(
                localPoint, distanceTolerance) == Geometry::OUTSIDE) {
            return Geometry::OUTSIDE;
        }
    }

    if (this->getGeometry()->doContainmentTest(
            objectPointToGeometryLocal(localPoint),
            distanceTolerance) != Geometry::OUTSIDE) {
        return Geometry::INSIDE;
    }
    return Geometry::OUTSIDE;
}

void
SimpleBody::doExtraInformation(const RayWithTracingState &ray, double t, PovRayHit *hit)
{
    Geometry *detailGeometry = hit->hitGeometry != nullptr ? hit->hitGeometry : geometry;
    const bool bakedLeaf = bakedTransformFolded;
    // Consume the detail-owner chain outermost-first (popDetailOwnerBack): the
    // owners were pushed innermost-first while collecting the hit, so for a
    // nested CSG operand (e.g. an intersection nested inside a transformed
    // union operand) the outer operand's transform must be peeled off the ray
    // before the inner ones. Each CsgOperand::doExtraInformation then recurses
    // into the next inner owner, and only the innermost owner evaluates the
    // primitive normal; unwinding re-applies each operand transform to the
    // normal from innermost to outermost. Popping only the first (innermost)
    // owner here would drop every outer operand's transform.
    RayOperationOwner *detailOwner = hit->popDetailOwnerBack();
    if (detailGeometry != nullptr) {
        RayWithTracingState localRay = RayWithTracingState::localIntersectionClone(ray);
        const Vector3Dd worldPoint = hit->p;
        if (transformationInverse != nullptr && !bakedLeaf) {
            localRay.setOrigin(transformationInverse->transformPoint(ray.getOrigin()));
            localRay.setDirection(transformationInverse->transformDirection(ray.getDirection()));
            localRay.setQuadricConstantsCached(false);
            hit->p = transformationInverse->transformPoint(worldPoint);
        }
        if (detailOwner != nullptr) {
            detailOwner->doExtraInformation(localRay, t, hit);
        } else {
            RayWithTracingState geometryLocalRay = RayWithTracingState::localIntersectionClone(localRay);
            if (geometryTransformationInverse != nullptr && !bakedLeaf) {
                geometryLocalRay.setOrigin(
                    geometryTransformationInverse->transformPoint(localRay.getOrigin()));
                geometryLocalRay.setDirection(
                    geometryTransformationInverse->transformDirection(localRay.getDirection()));
                geometryLocalRay.setQuadricConstantsCached(false);
                hit->p = geometryTransformationInverse->transformPoint(hit->p);
            }
            detailGeometry->doExtraInformation(geometryLocalRay, t, hit);
            if (geometryTransformationInverse != nullptr && !bakedLeaf) {
                hit->n = geometryNormalToObjectLocal(hit->n);
            }
        }
        if (transformationInverse != nullptr && !bakedLeaf) {
            hit->n = localNormalToWorld(hit->n);
        } else {
            // The detail-owner chain defers normalization (see
            // CsgOperand::doExtraInformation); when this body carries no
            // object transform of its own, normalize once here so the chained
            // normal is unit length before shading.
            hit->n = hit->n.normalizedFast();
        }
        hit->p = worldPoint;
    }
}

SimpleBody::SimpleBody(const SimpleBody &other) :
    geometry(other.getGeometry() != nullptr ?
        (Geometry *)other.getGeometry()->copy() : nullptr),
    geometryMaterial(other.getGeometryMaterial() != nullptr ?
        new PovRayMaterial(*static_cast<PovRayMaterial *>(other.getGeometryMaterial())) : nullptr),
    transformation(other.getTransformation() != nullptr ?
        new Matrix4x4d(*other.getTransformation()) : nullptr),
    transformationInverse(other.getTransformationInverse() != nullptr ?
        new Matrix4x4d(*other.getTransformationInverse()) : nullptr),
    geometryTransformation(other.getGeometryTransformation() != nullptr ?
        new Matrix4x4d(*other.getGeometryTransformation()) : nullptr),
    geometryTransformationInverse(other.getGeometryTransformationInverse() != nullptr ?
        new Matrix4x4d(*other.getGeometryTransformationInverse()) : nullptr),
    noShadowFlag(other.getNoShadowFlag()),
    objectColor(other.getObjectColor() != nullptr ?
        new ColorRgba(*other.getObjectColor()) : nullptr),
    objectTexture(other.getObjectTexture() != nullptr ?
        new PovRayMaterial(*static_cast<PovRayMaterial *>(other.getObjectTexture())) : nullptr),
    bodySteps(other.getBodySteps()),
    geometrySteps(other.getGeometrySteps())
{
    for (long int i = other.getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShapes.add(
            (SimpleBody *)other.getBoundingShapes()[i]->copy());
    }
    for (long int i = other.getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShapes.add(
            (SimpleBody *)other.getClippingShapes()[i]->copy());
    }
}

SimpleBody::~SimpleBody()
{
    delete geometry;
    delete geometryMaterial;
    delete transformation;
    delete transformationInverse;
    delete geometryTransformation;
    delete geometryTransformationInverse;
    delete objectColor;
    // objectTexture may be a private clone (delete it) or an alias to a shared
    // constant such as the scene's default texture (do not delete, just close
    // out its alias bookkeeping). releaseFromOwner() encapsulates that decision.
    if (objectTexture != nullptr) {
        static_cast<PovRayMaterial *>(objectTexture)->releaseFromOwner();
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
SimpleBody::ensureGeometryMatrices()
{
    if (geometryTransformation == nullptr) {
        geometryTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        geometryTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
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
    bodySteps.add(TransformStep(TransformStep::Kind::Translate, *vector));
}

void
SimpleBody::applyTranslationToGeometryTransform(Vector3Dd *vector)
{
    ensureGeometryMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *geometryTransformation = geometryTransformation->multiply(delta);
    *geometryTransformationInverse =
        deltaInverse.multiply(*geometryTransformationInverse);
    geometrySteps.add(TransformStep(TransformStep::Kind::Translate, *vector));
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
    bodySteps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
}

void
SimpleBody::applyRotationToGeometryTransform(Vector3Dd *vector)
{
    ensureGeometryMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *geometryTransformation = geometryTransformation->multiply(delta);
    *geometryTransformationInverse =
        deltaInverse.multiply(*geometryTransformationInverse);
    geometrySteps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
}

void
SimpleBody::applyScaleToBodyTransform(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    bodySteps.add(TransformStep(TransformStep::Kind::Scale, *vector));
}

void
SimpleBody::applyScaleToGeometryTransform(Vector3Dd *vector)
{
    ensureGeometryMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z()).transpose();
    *geometryTransformation = geometryTransformation->multiply(delta);
    *geometryTransformationInverse =
        deltaInverse.multiply(*geometryTransformationInverse);
    geometrySteps.add(TransformStep(TransformStep::Kind::Scale, *vector));
}

void
SimpleBody::applyOwnedTranslation(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->translate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->translate(vector);
    }
}

void
SimpleBody::applyOwnedRotation(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->rotate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->rotate(vector);
    }
}

void
SimpleBody::applyOwnedScale(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->scale(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->scale(vector);
    }
}

void
SimpleBody::propagateOwnedTranslation(Vector3Dd *vector)
{
    for (long int i = boundingShapes.size() - 1; i >= 0; i--) {
        boundingShapes[i]->propagateOwnedTranslation(vector);
    }
    for (long int i = clippingShapes.size() - 1; i >= 0; i--) {
        clippingShapes[i]->propagateOwnedTranslation(vector);
    }
    applyOwnedTranslation(vector);
}

void
SimpleBody::propagateOwnedRotation(Vector3Dd *vector)
{
    for (long int i = boundingShapes.size() - 1; i >= 0; i--) {
        boundingShapes[i]->propagateOwnedRotation(vector);
    }
    for (long int i = clippingShapes.size() - 1; i >= 0; i--) {
        clippingShapes[i]->propagateOwnedRotation(vector);
    }
    applyOwnedRotation(vector);
}

void
SimpleBody::propagateOwnedScale(Vector3Dd *vector)
{
    for (long int i = boundingShapes.size() - 1; i >= 0; i--) {
        boundingShapes[i]->propagateOwnedScale(vector);
    }
    for (long int i = clippingShapes.size() - 1; i >= 0; i--) {
        clippingShapes[i]->propagateOwnedScale(vector);
    }
    applyOwnedScale(vector);
}

Vector3Dd
SimpleBody::objectPointToGeometryLocal(const Vector3Dd &point) const
{
    return geometryTransformationInverse != nullptr ?
        geometryTransformationInverse->transformPoint(point) : point;
}

Vector3Dd
SimpleBody::geometryPointToObjectLocal(const Vector3Dd &point) const
{
    return geometryTransformation != nullptr ?
        geometryTransformation->transformPoint(point) : point;
}

Vector3Dd
SimpleBody::objectDirectionToGeometryLocal(const Vector3Dd &direction) const
{
    return geometryTransformationInverse != nullptr ?
        geometryTransformationInverse->transformDirection(direction) : direction;
}

Vector3Dd
SimpleBody::geometryNormalToObjectLocal(const Vector3Dd &normal) const
{
    if (geometryTransformationInverse == nullptr) {
        return normal;
    }
    return geometryTransformationInverse->withoutTranslation()
        .multiply(normal).normalizedFast();
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
    geometryTransformation = nullptr;
    geometryTransformationInverse = nullptr;
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
    applyTranslationToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedTranslation(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->propagateOwnedTranslation(vector);
    }
    applyOwnedTranslation(vector);
}

void
SimpleBody::translateOwnerOnly(Vector3Dd *vector)
{
    applyTranslationToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedTranslation(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->propagateOwnedTranslation(vector);
    }
    applyOwnedTranslation(vector);
}

void
SimpleBody::translateGeometryLayer(Vector3Dd *vector)
{
    applyTranslationToGeometryTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->translate(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->translate(vector);
    }
    applyOwnedTranslation(vector);
}

void
SimpleBody::rotate(Vector3Dd *vector)
{
    applyRotationToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedRotation(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->propagateOwnedRotation(vector);
    }
    applyOwnedRotation(vector);
}

void
SimpleBody::rotateOwnerOnly(Vector3Dd *vector)
{
    applyRotationToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedRotation(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->propagateOwnedRotation(vector);
    }
    applyOwnedRotation(vector);
}

void
SimpleBody::rotateGeometryLayer(Vector3Dd *vector)
{
    applyRotationToGeometryTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->rotate(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->rotate(vector);
    }
    applyOwnedRotation(vector);
}

void
SimpleBody::scale(Vector3Dd *vector)
{
    applyScaleToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedScale(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->propagateOwnedScale(vector);
    }
    applyOwnedScale(vector);
}

void
SimpleBody::scaleOwnerOnly(Vector3Dd *vector)
{
    applyScaleToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedScale(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->propagateOwnedScale(vector);
    }
    applyOwnedScale(vector);
}

void
SimpleBody::scaleGeometryLayer(Vector3Dd *vector)
{
    applyScaleToGeometryTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->scale(vector);
    }
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->scale(vector);
    }
    applyOwnedScale(vector);
}

void
SimpleBody::invert()
{
    SimpleBody *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invert();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invert();
    }
    if (this->getGeometry() != nullptr) {
        this->getGeometry()->invertGeometry();
        geometrySteps.add(TransformStep(TransformStep::Kind::Invert, Vector3Dd(0.0, 0.0, 0.0)));
    }
}

AxisAlignedBoundingBox
SimpleBody::getAABB() const
{
    // Bounding shapes live in this body's object-local space, exactly like the
    // object-local ray passed to them in doIntersectionForAllRayCrossings().
    // Their combined box therefore still needs this body's own transform to be
    // mapped into world space before top-level culling can consume it.
    if (boundingShapes.size() > 0) {
        AxisAlignedBoundingBox result = AxisAlignedBoundingBox::unbounded();
        for (long int i = 0; i < boundingShapes.size(); i++) {
            result = result.intersection(boundingShapes[i]->getAABB());
        }
        if (transformation != nullptr && !result.isUnbounded()) {
            return AxisAlignedBoundingBox::fromTransformedCorners(
                result.min, result.max, transformation);
        }
        return result;
    }
    if (geometry != nullptr) {
        AxisAlignedBoundingBox box = geometry->getMinMax();
        if (geometryTransformation != nullptr && !box.isUnbounded()) {
            box = AxisAlignedBoundingBox::fromTransformedCorners(
                box.min, box.max, geometryTransformation);
        }
        if (transformation != nullptr && !box.isUnbounded()) {
            return AxisAlignedBoundingBox::fromTransformedCorners(
                box.min, box.max, transformation);
        }
        return box;
    }
    return AxisAlignedBoundingBox::unbounded();
}
