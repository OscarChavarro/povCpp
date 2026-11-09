#include <stdexcept>
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "vsdk/toolkit/environment/raycasting/geometry/element/PriorityQueuePool.txx"

bool
RayCastingBody::doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out)
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
RayCastingBody::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    RayCastingBody *boundingShape;
    RayCastingBody *clippingShape;
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
                    RayCastingGeometry::OUTSIDE) {
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
        if (transformation != nullptr) {
            localIntersection.getIntersection().point =
                transformation->transformPoint(localIntersection.getIntersection().point);
        }
        // Clipping shapes live in this body's outer frame (they only ever
        // receive object-level transform deltas via propagateOwned*, never
        // ones internal to a CSG block), so containment is tested against
        // the point after `transformation`, not before.
        const Vector3Dd clipTestPoint = localIntersection.getIntersection().point;
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
                    clipTestPoint,
                    GeometryConfig::SMALL_TOLERANCE) == RayCastingGeometry::OUTSIDE) {
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
RayCastingBody::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    RayCastingBody *boundingShape;
    RayCastingBody *clippingShape;
    const Vector3Dd localPoint = worldPointToLocal(point);

    // Bounding/clipping shapes live in this body's outer frame (the frame
    // `point` arrives in), not in its local/geometry frame.
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(
                point, distanceTolerance) == RayCastingGeometry::OUTSIDE) {
            return RayCastingGeometry::OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(
                point, distanceTolerance) == RayCastingGeometry::OUTSIDE) {
            return RayCastingGeometry::OUTSIDE;
        }
    }

    if (this->getGeometry()->doContainmentTest(
            objectPointToGeometryLocal(localPoint),
            distanceTolerance) != RayCastingGeometry::OUTSIDE) {
        return RayCastingGeometry::INSIDE;
    }
    return RayCastingGeometry::OUTSIDE;
}

void
RayCastingBody::doExtraInformation(const RayWithTracingState &ray, double t, RayCastingHit *hit)
{
    PostRayHitElement *detailGeometry = hit->hitGeometry != nullptr ? hit->hitGeometry : getGeometry();
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
    PostRayHitElement *detailOwner = hit->popDetailOwnerBack();
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

RayCastingBody::RayCastingBody(const RayCastingBody &other) :
    SimpleBody(),
    geometryMaterial(other.getGeometryMaterial() != nullptr ?
        other.getGeometryMaterial()->copy() : nullptr),
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
        other.getObjectTexture()->copy() : nullptr),
    bodySteps(other.getBodySteps()),
    geometrySteps(other.getGeometrySteps())
{
    setGeometry(other.getGeometry() != nullptr ?
        static_cast<RayCastingGeometry*>(other.getGeometry()->copy()) : nullptr);
    for (long int i = other.getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShapes.add(
            (RayCastingBody *)other.getBoundingShapes()[i]->copy());
    }
    for (long int i = other.getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShapes.add(
            (RayCastingBody *)other.getClippingShapes()[i]->copy());
    }
}

RayCastingBody::~RayCastingBody()
{
    delete geometryMaterial;
    delete transformation;
    delete transformationInverse;
    delete geometryTransformation;
    delete geometryTransformationInverse;
    delete objectColor;
    // objectTexture may be a private clone (plain delete is correct) or an
    // alias to a shared constant such as the scene's default texture (which
    // needs its own release bookkeeping instead of being deleted outright).
    // That decision depends on the concrete Material subtype, which the scene
    // layer has no business knowing about; whoever constructed this body
    // supplies objectTextureReleaser to make the right call.
    if (objectTexture != nullptr) {
        if (objectTextureReleaser != nullptr) {
            objectTextureReleaser(objectTexture);
        } else {
            delete objectTexture;
        }
    }
    for (long int i = 0; i < boundingShapes.size(); i++) {
        delete boundingShapes[i];
    }
    for (long int i = 0; i < clippingShapes.size(); i++) {
        delete clippingShapes[i];
    }
}

void
RayCastingBody::ensureMatrices()
{
    if (transformation == nullptr) {
        transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

void
RayCastingBody::ensureGeometryMatrices()
{
    if (geometryTransformation == nullptr) {
        geometryTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        geometryTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

void
RayCastingBody::applyTranslationToBodyTransform(Vector3Dd *vector)
{
    markModified();
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
RayCastingBody::applyTranslationToGeometryTransform(Vector3Dd *vector)
{
    markModified();
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
RayCastingBody::applyRotationToBodyTransform(Vector3Dd *vector)
{
    markModified();
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    bodySteps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
}

void
RayCastingBody::applyRotationToGeometryTransform(Vector3Dd *vector)
{
    markModified();
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
RayCastingBody::applyScaleToBodyTransform(Vector3Dd *vector)
{
    markModified();
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
RayCastingBody::applyScaleToGeometryTransform(Vector3Dd *vector)
{
    markModified();
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
RayCastingBody::applyOwnedTranslation(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->translate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->translate(vector);
    }
}

void
RayCastingBody::applyOwnedRotation(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->rotate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->rotate(vector);
    }
}

void
RayCastingBody::applyOwnedScale(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->scale(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->scale(vector);
    }
}

void
RayCastingBody::propagateOwnedTranslation(Vector3Dd *vector)
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
RayCastingBody::propagateOwnedRotation(Vector3Dd *vector)
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
RayCastingBody::propagateOwnedScale(Vector3Dd *vector)
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
RayCastingBody::objectPointToGeometryLocal(const Vector3Dd &point) const
{
    return geometryTransformationInverse != nullptr ?
        geometryTransformationInverse->transformPoint(point) : point;
}

Vector3Dd
RayCastingBody::geometryPointToObjectLocal(const Vector3Dd &point) const
{
    return geometryTransformation != nullptr ?
        geometryTransformation->transformPoint(point) : point;
}

Vector3Dd
RayCastingBody::objectDirectionToGeometryLocal(const Vector3Dd &direction) const
{
    return geometryTransformationInverse != nullptr ?
        geometryTransformationInverse->transformDirection(direction) : direction;
}

Vector3Dd
RayCastingBody::geometryNormalToObjectLocal(const Vector3Dd &normal) const
{
    if (geometryTransformationInverse == nullptr) {
        return normal;
    }
    return geometryTransformationInverse->withoutTranslation()
        .multiply(normal).normalizedFast();
}

Vector3Dd
RayCastingBody::worldPointToLocal(const Vector3Dd &point) const
{
    return transformationInverse != nullptr ?
        transformationInverse->transformPoint(point) : point;
}

Vector3Dd
RayCastingBody::localPointToWorld(const Vector3Dd &point) const
{
    return transformation != nullptr ? transformation->transformPoint(point) : point;
}

Vector3Dd
RayCastingBody::worldDirectionToLocal(const Vector3Dd &direction) const
{
    return transformationInverse != nullptr ?
        transformationInverse->transformDirection(direction) : direction;
}

Vector3Dd
RayCastingBody::localNormalToWorld(const Vector3Dd &normal) const
{
    if (transformationInverse == nullptr) {
        return normal;
    }
    return transformationInverse->withoutTranslation().multiply(normal).normalizedFast();
}

void
RayCastingBody::detachOwnership()
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
RayCastingBody::copy()
{
    return new RayCastingBody(*this);
}

void
RayCastingBody::translate(Vector3Dd *vector)
{
    applyTranslationToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedTranslation(vector);
    }
    // Clipping shapes are specified in the frame current at their definition
    // point, and object-level transforms parsed after clipped_by must carry
    // them along (POV 1.0 semantics); containment is later tested against the
    // point after this body's transform, so the clip geometry must follow it.
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->translate(vector);
    }
    applyOwnedTranslation(vector);
}

void
RayCastingBody::translateOwnerOnly(Vector3Dd *vector)
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
RayCastingBody::translateGeometryLayer(Vector3Dd *vector)
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
RayCastingBody::rotate(Vector3Dd *vector)
{
    applyRotationToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedRotation(vector);
    }
    // Clip geometry must follow object-level transforms; see translate().
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->rotate(vector);
    }
    applyOwnedRotation(vector);
}

void
RayCastingBody::rotateOwnerOnly(Vector3Dd *vector)
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
RayCastingBody::rotateGeometryLayer(Vector3Dd *vector)
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
RayCastingBody::scale(Vector3Dd *vector)
{
    applyScaleToBodyTransform(vector);
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        this->getBoundingShapes()[i]->propagateOwnedScale(vector);
    }
    // Clip geometry must follow object-level transforms; see translate().
    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        this->getClippingShapes()[i]->scale(vector);
    }
    applyOwnedScale(vector);
}

void
RayCastingBody::scaleOwnerOnly(Vector3Dd *vector)
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
RayCastingBody::scaleGeometryLayer(Vector3Dd *vector)
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
RayCastingBody::invert()
{
    markModified();
    RayCastingBody *localShape;

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
RayCastingBody::getAABB() const
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
                result.getMin(), result.getMax(), transformation);
        }
        return result;
    }
    if (geometry != nullptr) {
        BoundingVolumeHierarchy *bounds = getGeometry()->createBoundingVolume();
        AxisAlignedBoundingBox box = bounds->axisAlignedExtent();
        delete bounds;
        if (geometryTransformation != nullptr && !box.isUnbounded()) {
            box = AxisAlignedBoundingBox::fromTransformedCorners(
                box.getMin(), box.getMax(), geometryTransformation);
        }
        if (transformation != nullptr && !box.isUnbounded()) {
            return AxisAlignedBoundingBox::fromTransformedCorners(
                box.getMin(), box.getMax(), transformation);
        }
        return box;
    }
    return AxisAlignedBoundingBox::unbounded();
}

#include "vsdk/toolkit/environment/raycasting/geometry/element/RayCastingQueryAdapter.h"

bool RayCastingBody::doIntersectionFirstHit(const Ray& source, RayHit* out) const
{
    RayCastingQueryAdapter context(source);
    IntersectionCandidate candidate;
    auto* body = const_cast<RayCastingBody*>(this);
    if (!body->doIntersectionFirstHitViaCrossings(&context.ray, candidate)) return false;
    RayCastingHit detail = RayCastingHit::fromCandidate(candidate,
        out ? out->getRequiredDetailMask() : RayCastingHit::DETAIL_NONE);
    body->doExtraInformation(context.ray, detail.hitDistance, &detail);
    RayCastingQueryAdapter::exportHit(source, detail, out);
    return true;
}

void RayCastingBody::doExtraInformation(const Ray& source, double t, RayHit* out) const
{
    if (!out) return;
    RayCastingQueryAdapter context(source);
    IntersectionCandidate candidate;
    if (!context.findCrossing(const_cast<RayCastingBody*>(this), t, candidate)) return;
    RayCastingHit detail = RayCastingHit::fromCandidate(candidate, out->getRequiredDetailMask());
    const_cast<RayCastingBody*>(this)->doExtraInformation(context.ray, t, &detail);
    RayCastingQueryAdapter::exportHit(source, detail, out);
}

RayCrossingSupport RayCastingBody::emitRayCrossings(const Ray& source,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    RayCastingQueryAdapter context(source);
    auto* queue = context.pool.pop(128);
    const_cast<RayCastingBody*>(this)->doIntersectionForAllRayCrossings(&context.ray, queue);
    while (queue->size() > 0) {
        const IntersectionCandidate candidate = queue->poll();
        const double t = candidate.getIntersection().t;
        if (t < query.minT || t > query.maxT) continue;
        RayCrossing event(t, dynamic_cast<const Geometry*>(candidate.getAttributes().getHitGeometry()),
            RayCrossing::Kind::Unclassified);
        event.body = dynamic_cast<const SimpleBody*>(candidate.getAttributes().getHitBody());
        event.material = candidate.getAttributes().getMaterial();
        sink.crossing(event);
    }
    context.pool.push(queue);
    return RayCrossingSupport::Supported;
}

SimpleMaterial* RayCastingBody::getMaterial() const
{
    Material* surface = getSurfaceMaterial();
    auto* simple = dynamic_cast<SimpleMaterial*>(surface);
    if (surface && !simple) {
        throw std::invalid_argument("This material requires its application's shader pipeline");
    }
    return simple;
}

void RayCastingBody::setGeometry(Geometry* value)
{
    if (value && !dynamic_cast<RayCastingGeometry*>(value))
        throw std::invalid_argument("RayCastingBody requires a RayCastingGeometry");
    SimpleBody::setGeometry(value);
}

void RayCastingBody::setGeometryReference(Geometry* value)
{
    if (value && !dynamic_cast<RayCastingGeometry*>(value))
        throw std::invalid_argument("RayCastingBody requires a RayCastingGeometry");
    SimpleBody::setGeometryReference(value);
}

void RayCastingBody::setPosition(const Vector3Dd& position)
{
    Vector3Dd delta = position.subtract(getPosition());
    translateOwnerOnly(&delta);
}

Vector3Dd RayCastingBody::getScale() const
{ throw std::logic_error("Affine body: use getTransformationMatrix()"); }
Matrix4x4d RayCastingBody::getRotation() const
{ throw std::logic_error("Affine body: use getTransformationMatrix()"); }
Matrix4x4d RayCastingBody::getRotationInverse() const
{ throw std::logic_error("Affine body: use getTransformationMatrix().invert()"); }
void RayCastingBody::setScale(const Vector3Dd&)
{ throw std::logic_error("Affine body: use scale() to compose a placement"); }
void RayCastingBody::setRotation(const Matrix4x4d&)
{ throw std::logic_error("Affine body: use rotate() to compose a placement"); }
void RayCastingBody::setRotationInverse(const Matrix4x4d&)
{ throw std::logic_error("Affine body: use rotate(); inverse is maintained automatically"); }
