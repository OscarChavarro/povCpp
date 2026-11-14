#include "environment/intersection/SurfaceDetailResolver.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRothRayClassifier.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "environment/scene/RayCastingCoreOperand.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include <typeinfo>
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/Double.h"
#include <stdexcept>
#include "vsdk/toolkit/environment/scene/SceneSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.h"
#include "environment/intersection/ray/IntersectionTolerances.h"
#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "environment/scene/PovrayBody.h"
#include "environment/intersection/ProjectedTriangleTraversal.h"
#include "vsdk/toolkit/environment/scene/InstanceRayCrossingBuffer.h"
#include "vsdk/toolkit/environment/geometry/element/RayInstanceStep.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "environment/intersection/SurfaceIntersector.h"
#include "environment/intersection/SurfaceBoundingVolume.h"
#include "environment/intersection/BoxInvertedCrossingSink.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/intersection/ray/PriorityQueuePool.txx"

bool
PovrayBody::doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out)
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
PovrayBody::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    PovrayBody *boundingShape;
    PovrayBody *clippingShape;
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
                boundingShape->doContainmentTest(rayOrigin, IntersectionTolerances::SMALL_TOLERANCE) ==
                    RayCastingContainment::OUTSIDE) {
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
        traceSurfaceCrossings(&geometryLocalRay, localDepthQueue, effectiveGeometryMaterial);
    } else {
        traceSurfaceCrossings(objectRay, localDepthQueue, effectiveGeometryMaterial);
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
                    IntersectionTolerances::SMALL_TOLERANCE) == RayCastingContainment::OUTSIDE) {
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
PovrayBody::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    PovrayBody *boundingShape;
    PovrayBody *clippingShape;
    const Vector3Dd localPoint = worldPointToLocal(point);

    // Bounding/clipping shapes live in this body's outer frame (the frame
    // `point` arrives in), not in its local/geometry frame.
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(
                point, distanceTolerance) == RayCastingContainment::OUTSIDE) {
            return RayCastingContainment::OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(
                point, distanceTolerance) == RayCastingContainment::OUTSIDE) {
            return RayCastingContainment::OUTSIDE;
        }
    }

    if (surfaceContainment(
            objectPointToGeometryLocal(localPoint),
            distanceTolerance) != RayCastingContainment::OUTSIDE) {
        return RayCastingContainment::INSIDE;
    }
    return RayCastingContainment::OUTSIDE;
}

void
PovrayBody::doExtraInformation(const RayWithTracingState &ray, double t, RayCastingHit *hit)
{
    const bool hasDetailSource = hit->hitSurface != nullptr || hasShape();
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
    if (hasDetailSource) {
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
            SurfaceDetailResolver::resolve(geometryLocalRay, t, hit);
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

PovrayBody::PovrayBody(const PovrayBody &other) :
    SceneSolid(),
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
    objectTextureReleaser(other.objectTextureReleaser),
    bodySteps(other.getBodySteps()),
    geometrySteps(other.getGeometrySteps()),
    arbitraryAffineTransform(other.arbitraryAffineTransform)
{
    if (other.csg != nullptr) {
        setCsg(other.csg->copy());
    } else if (other.light != nullptr) {
        setLight(other.light->copy());
    } else {
        setGeometry(other.geometry != nullptr ? other.geometry->copyGeometry() : nullptr);
    }
    if (other.triangleTraversal != nullptr) {
        setTriangleTraversal(new ProjectedTriangleTraversal(*other.triangleTraversal,
            static_cast<TriangleMesh*>(geometry)));
    }
    surfaceState.setInverted(other.surfaceState.isInverted());
    surfaceState.setViewpointValue(other.surfaceState.getViewpointValue());
    surfaceState.setViewpointCached(other.surfaceState.isViewpointCached());
    for (long int i = other.getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShapes.add(
            (PovrayBody *)other.getBoundingShapes()[i]->copy());
    }
    for (long int i = other.getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShapes.add(
            (PovrayBody *)other.getClippingShapes()[i]->copy());
    }
}

PovrayBody::~PovrayBody()
{
    delete csg;
    delete light;
    delete triangleTraversal;
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
PovrayBody::ensureMatrices()
{
    if (transformation == nullptr) {
        transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

void
PovrayBody::ensureGeometryMatrices()
{
    if (geometryTransformation == nullptr) {
        geometryTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        geometryTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

void
PovrayBody::applyTranslationToBodyTransform(Vector3Dd *vector)
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
PovrayBody::applyTranslationToGeometryTransform(Vector3Dd *vector)
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
PovrayBody::applyRotationToBodyTransform(Vector3Dd *vector)
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
PovrayBody::applyRotationToGeometryTransform(Vector3Dd *vector)
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
PovrayBody::applyScaleToBodyTransform(Vector3Dd *vector)
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
PovrayBody::applyScaleToGeometryTransform(Vector3Dd *vector)
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
PovrayBody::applyOwnedTranslation(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->translate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->translate(vector);
    }
}

void
PovrayBody::applyOwnedRotation(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->rotate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->rotate(vector);
    }
}

void
PovrayBody::applyOwnedScale(Vector3Dd *vector)
{
    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->scale(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->scale(vector);
    }
}

void
PovrayBody::propagateOwnedTranslation(Vector3Dd *vector)
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
PovrayBody::propagateOwnedRotation(Vector3Dd *vector)
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
PovrayBody::propagateOwnedScale(Vector3Dd *vector)
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
PovrayBody::objectPointToGeometryLocal(const Vector3Dd &point) const
{
    return geometryTransformationInverse != nullptr ?
        geometryTransformationInverse->transformPoint(point) : point;
}

Vector3Dd
PovrayBody::geometryPointToObjectLocal(const Vector3Dd &point) const
{
    return geometryTransformation != nullptr ?
        geometryTransformation->transformPoint(point) : point;
}

Vector3Dd
PovrayBody::objectDirectionToGeometryLocal(const Vector3Dd &direction) const
{
    return geometryTransformationInverse != nullptr ?
        geometryTransformationInverse->transformDirection(direction) : direction;
}

Vector3Dd
PovrayBody::geometryNormalToObjectLocal(const Vector3Dd &normal) const
{
    if (geometryTransformationInverse == nullptr) {
        return normal;
    }
    return geometryTransformationInverse->withoutTranslation()
        .multiply(normal).normalizedFast();
}

Vector3Dd
PovrayBody::worldPointToLocal(const Vector3Dd &point) const
{
    return transformationInverse != nullptr ?
        transformationInverse->transformPoint(point) : point;
}

Vector3Dd
PovrayBody::localPointToWorld(const Vector3Dd &point) const
{
    return transformation != nullptr ? transformation->transformPoint(point) : point;
}

Vector3Dd
PovrayBody::worldDirectionToLocal(const Vector3Dd &direction) const
{
    return transformationInverse != nullptr ?
        transformationInverse->transformDirection(direction) : direction;
}

Vector3Dd
PovrayBody::localNormalToWorld(const Vector3Dd &normal) const
{
    if (transformationInverse == nullptr) {
        return normal;
    }
    return transformationInverse->withoutTranslation().multiply(normal).normalizedFast();
}

void
PovrayBody::setObjectColor(ColorRgba *color)
{
    if (color != objectColor) {
        delete objectColor;
        objectColor = color;
    }
}

void *
PovrayBody::copy()
{
    return new PovrayBody(*this);
}

void
PovrayBody::translate(Vector3Dd *vector)
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
PovrayBody::translateOwnerOnly(Vector3Dd *vector)
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
PovrayBody::translateGeometryLayer(Vector3Dd *vector)
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
PovrayBody::rotate(Vector3Dd *vector)
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
PovrayBody::rotateOwnerOnly(Vector3Dd *vector)
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
PovrayBody::rotateGeometryLayer(Vector3Dd *vector)
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
PovrayBody::scale(Vector3Dd *vector)
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
PovrayBody::scaleOwnerOnly(Vector3Dd *vector)
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
PovrayBody::scaleGeometryLayer(Vector3Dd *vector)
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
PovrayBody::invert()
{
    markModified();
    PovrayBody *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invert();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invert();
    }
    if (hasShape()) {
        invertSurface();
        geometrySteps.add(TransformStep(TransformStep::Kind::Invert, Vector3Dd(0.0, 0.0, 0.0)));
    }
}

Matrix4x4d PovrayBody::getAffinePlacement() const
{
    // POV stores row-vector transforms; the scene contract uses column vectors.
    const Matrix4x4d outer = transformation ? transformation->transpose() : Matrix4x4d::identityMatrix();
    const Matrix4x4d inner = geometryTransformation ? geometryTransformation->transpose() : Matrix4x4d::identityMatrix();
    return outer.multiply(inner);
}

AffinePlacementStatus PovrayBody::setAffinePlacement(const Matrix4x4d& forward,
    const Matrix4x4d& inverse)
{
    const AffinePlacementStatus input = AffinePlacement::validate(forward, inverse);
    if (input != AffinePlacementStatus::Valid) return input;
    if ((geometryTransformation == nullptr) != (geometryTransformationInverse == nullptr) ||
        (transformation == nullptr) != (transformationInverse == nullptr))
        return AffinePlacementStatus::Indeterminate;
    const Matrix4x4d inner = geometryTransformation ? geometryTransformation->transpose() : Matrix4x4d::identityMatrix();
    const Matrix4x4d innerInverse = geometryTransformationInverse ? geometryTransformationInverse->transpose() : Matrix4x4d::identityMatrix();
    const Matrix4x4d outer = forward.multiply(innerInverse);
    const Matrix4x4d outerInverse = inner.multiply(inverse);
    const AffinePlacementStatus composed = AffinePlacement::validate(outer,outerInverse);
    if (composed != AffinePlacementStatus::Valid) return composed;
    if (boundingShapes.size() != 0 || clippingShapes.size() != 0) {
        // The object-level change in the outer frame, which POV also applies
        // to owned regions for TRS edits. Reject before changing anything.
        const Matrix4x4d current = transformation ? transformation->transpose() : Matrix4x4d::identityMatrix();
        const Matrix4x4d currentInverse = transformationInverse ? transformationInverse->transpose() : Matrix4x4d::identityMatrix();
        const Matrix4x4d delta = outer.multiply(currentInverse);
        const Matrix4x4d deltaInverse = current.multiply(outerInverse);
        const AffinePlacementStatus change = AffinePlacement::validate(delta,deltaInverse);
        if (change != AffinePlacementStatus::Valid) return change;
        if (!canMoveRegions(delta,deltaInverse)) return AffinePlacementStatus::Unsupported;
        moveRegions(delta,deltaInverse);
    }
    ensureMatrices();
    *transformation = outer.transpose();
    *transformationInverse = outerInverse.transpose();
    arbitraryAffineTransform = true;
    bakedTransformFolded = false;
    markModified();
    return AffinePlacementStatus::Valid;
}

void PovrayBody::resetAffinePlacement()
{
    const AffinePlacementStatus status = setAffinePlacement(Matrix4x4d::identityMatrix(), Matrix4x4d::identityMatrix());
    if (status != AffinePlacementStatus::Valid)
        throw java::IllegalStateException("Cannot reset this layered POV placement");
}

AffinePlacementStatus PovrayBody::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{
    if ((transformation == nullptr) != (transformationInverse == nullptr) ||
        (geometryTransformation == nullptr) != (geometryTransformationInverse == nullptr))
        return AffinePlacementStatus::Indeterminate;
    forward = getAffinePlacement();
    const Matrix4x4d outer = transformationInverse ? transformationInverse->transpose() : Matrix4x4d::identityMatrix();
    const Matrix4x4d inner = geometryTransformationInverse ? geometryTransformationInverse->transpose() : Matrix4x4d::identityMatrix();
    inverse = inner.multiply(outer);
    return AffinePlacement::validate(forward, inverse);
}

bool PovrayBody::queryCoreSolid(const Geometry*& intrinsic, bool& inverted) const
{
    intrinsic=nullptr; inverted=false;
    if (!preservesCoreSolid()) return false;
    if (!geometry) return false;
    // A POV CSG (no geometry) and light geometry have no VITRAL solid.
    // A bicubic patch is not a solid.
    if (surfaceFamily==SurfaceDetailFamily::BicubicPatch) return false;
    intrinsic=geometry;
    inverted=surfaceState.isInverted();
    return true;
}

bool PovrayBody::preservesCoreSolid() const { return typeid(*this)==typeid(PovrayBody); }

bool PovrayBody::queryDenotedSolid(const Geometry*& intrinsic, bool& inverted) const
{
    if (!boundingShapes.isEmpty() || !clippingShapes.isEmpty()) { intrinsic=nullptr; inverted=false; return false; }
    return queryCoreSolid(intrinsic,inverted);
}

void PovrayBody::getCoreSolidWorldBounds(GeometryBounds& result) const
{
    const Geometry* intrinsic=nullptr; bool inverted=false;
    Matrix4x4d forward,inverse;
    if (!queryCoreSolid(intrinsic,inverted) || dynamic_cast<const Solid*>(intrinsic)==nullptr) {
        result.setUnsupported(); return;
    }
    if (queryAffinePlacement(forward,inverse)!=AffinePlacementStatus::Valid) { result.setIndeterminate(); return; }
    if (inverted) { result.setUnbounded(); return; }
    GeometryBounds local;
    intrinsic->getBounds(local);
    result=local.transformed(forward);
}

bool PovrayBody::canMoveRegions(const Matrix4x4d& delta, const Matrix4x4d& deltaInverse) const
{
    for (int list=0;list<2;++list) {
        const java::ArrayList<PovrayBody*>& regions=list==0 ? boundingShapes : clippingShapes;
        for (long int i=0;i<regions.size();++i) {
            const PovrayBody* region=regions[i];
            // Only plain bodies are known to apply a placement atomically.
            if (!region || typeid(*region)!=typeid(PovrayBody)) return false;
            Matrix4x4d forward,inverse;
            if (region->queryAffinePlacement(forward,inverse)!=AffinePlacementStatus::Valid ||
                AffinePlacement::validate(delta.multiply(forward),inverse.multiply(deltaInverse))!=AffinePlacementStatus::Valid ||
                !region->canMoveRegions(delta,deltaInverse)) return false;
        }
    }
    return true;
}

void PovrayBody::moveRegions(const Matrix4x4d& delta, const Matrix4x4d& deltaInverse)
{
    for (int list=0;list<2;++list) {
        java::ArrayList<PovrayBody*>& regions=list==0 ? boundingShapes : clippingShapes;
        for (long int i=0;i<regions.size();++i) {
            Matrix4x4d forward,inverse;
            regions[i]->queryAffinePlacement(forward,inverse);
            regions[i]->setAffinePlacement(delta.multiply(forward),inverse.multiply(deltaInverse));
        }
    }
}

bool PovrayBody::boundingRegionsVerified() const
{
    if (boundingShapes.isEmpty()) return true;
    GeometryBounds core;
    getCoreSolidWorldBounds(core);
    if (core.getKind()==GeometryBounds::Kind::Empty) return true;
    if (core.getKind()!=GeometryBounds::Kind::Finite) return false;
    for (long int i=0;i<boundingShapes.size();++i) {
        if (!boundingShapes[i] || boundingShapes[i]->classifyWorldBoxCertified(core)!=GeometryContainment::Inside)
            return false;
    }
    return true;
}

GeometryContainment PovrayBody::classifyCoreCertified(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    const Geometry* intrinsic=nullptr; bool inverted=false;
    if (!queryCoreSolid(intrinsic,inverted)) return GeometryContainment::Unsupported;
    Matrix4x4d forward,inverse;
    const AffinePlacementStatus placement=queryAffinePlacement(forward,inverse);
    if (placement==AffinePlacementStatus::Invalid) return GeometryContainment::InvalidInput;
    if (placement==AffinePlacementStatus::Unsupported) return GeometryContainment::Unsupported;
    if (placement==AffinePlacementStatus::Indeterminate) return GeometryContainment::Indeterminate;
    return classifyPlacedPointCertified(*intrinsic,inverted,forward,inverse,point,tolerance);
}

void PovrayBody::getSolidWorldBounds(GeometryBounds& result) const
{
    // Clipping only removes material, so the core bounds intersected with each
    // clip region's solid bounds still enclose the body. An unverified
    // bounding hint has no defined solid.
    if (!boundingRegionsVerified()) { result.setUnsupported(); return; }
    getCoreSolidWorldBounds(result);
    for (long int i=0;i<clippingShapes.size();++i) {
        if (!clippingShapes[i]) { result.setUnsupported(); return; }
        GeometryBounds clip;
        clippingShapes[i]->getSolidWorldBounds(clip);
        result=result.intersecting(clip);
    }
}

GeometryContainment PovrayBody::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance<0)
        return GeometryContainment::InvalidInput;
    const Geometry* intrinsic=nullptr; bool inverted=false;
    if (!queryCoreSolid(intrinsic,inverted)) return GeometryContainment::Unsupported;
    // bounded_by is a hint: it never changes membership, and it is only
    // accepted when certified to contain the core solid.
    if (!boundingRegionsVerified()) return GeometryContainment::Unsupported;
    if (clippingShapes.isEmpty()) return classifyCoreCertified(point,tolerance);
    // clipped_by keeps the core material inside every clip region: the shared
    // conservative intersection decides, including exposed clip boundaries.
    RayCastingCoreOperand core(*this);
    CsgTree clipped(CsgOperation::INTERSECTION);
    java::ArrayList<CsgBodyOperand*> views(clippingShapes.size());
    const GeometryContainment result=buildClippedExpression(core,clipped,views) ?
        CsgPointClassification::classify(clipped,point,tolerance) : GeometryContainment::Unsupported;
    for (long int i=0;i<views.size();++i) delete views[i];
    return result;
}

bool PovrayBody::buildClippedExpression(const CsgOperandView& core, CsgTree& expression,
    java::ArrayList<CsgBodyOperand*>& views) const
{
    if (!expression.addOperand(core)) return false;
    for (long int i=0;i<clippingShapes.size();++i) {
        if (!clippingShapes[i]) return false;
        CsgBodyOperand* view=new CsgBodyOperand(*clippingShapes[i]);
        if (!views.add(view)) { delete view; return false; }
        if (!expression.addOperand(*view)) return false;
    }
    return true;
}

AxisAlignedBoundingBox
PovrayBody::getAABB() const
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
            const Matrix4x4d placement = transformation->transpose();
            return AxisAlignedBoundingBox::fromTransformedCorners(
                result.getMin(), result.getMax(), &placement);
        }
        return result;
    }
    if (hasShape()) {
        BoundingVolumeHierarchy *bounds = createSurfaceBoundingVolume();
        AxisAlignedBoundingBox box = bounds->axisAlignedExtent();
        delete bounds;
        if (geometryTransformation != nullptr && !box.isUnbounded()) {
            const Matrix4x4d placement = geometryTransformation->transpose();
            box = AxisAlignedBoundingBox::fromTransformedCorners(
                box.getMin(), box.getMax(), &placement);
        }
        if (transformation != nullptr && !box.isUnbounded()) {
            const Matrix4x4d placement = transformation->transpose();
            return AxisAlignedBoundingBox::fromTransformedCorners(
                box.getMin(), box.getMax(), &placement);
        }
        return box;
    }
    return AxisAlignedBoundingBox::unbounded();
}

#include "environment/intersection/ray/RayCastingQueryAdapter.h"

bool PovrayBody::doIntersectionFirstHit(const Ray& source, RayHit* out) const
{
    RayCastingQueryAdapter context(source);
    IntersectionCandidate candidate;
    auto* body = const_cast<PovrayBody*>(this);
    if (!body->doIntersectionFirstHitViaCrossings(&context.ray, candidate)) return false;
    RayCastingHit detail = RayCastingHit::fromCandidate(candidate,
        out ? out->getRequiredDetailMask() : RayCastingHit::DETAIL_NONE);
    body->doExtraInformation(context.ray, detail.hitDistance, &detail);
    RayCastingQueryAdapter::exportHit(source, detail, out);
    return true;
}

void PovrayBody::doExtraInformation(const Ray& source, double t, RayHit* out) const
{
    if (!out) return;
    RayCastingQueryAdapter context(source);
    IntersectionCandidate candidate;
    if (!context.findCrossing(const_cast<PovrayBody*>(this), t, candidate)) return;
    RayCastingHit detail = RayCastingHit::fromCandidate(candidate, out->getRequiredDetailMask());
    const_cast<PovrayBody*>(this)->doExtraInformation(context.ray, t, &detail);
    RayCastingQueryAdapter::exportHit(source, detail, out);
}

RaySolidQueryStatus PovrayBody::classifyCoreRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    if (triangleTraversal != nullptr) { result.clear(); return RaySolidQueryStatus::Unsupported; }
    const RaySolidQueryStatus status=SceneSolid::classifyRay(ray,query,result);
    if (status==RaySolidQueryStatus::Supported && surfaceState.isInverted()) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}

RaySolidQueryStatus PovrayBody::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    result.clear();
    // bounded_by never changes membership once certified to contain the core;
    // clipped_by is the Roth combination of the core and its clip regions.
    if (!boundingRegionsVerified()) return RaySolidQueryStatus::Unsupported;
    if (clippingShapes.isEmpty()) return classifyCoreRay(ray,query,result);
    RayCastingCoreOperand core(*this);
    CsgTree clipped(CsgOperation::INTERSECTION);
    java::ArrayList<CsgBodyOperand*> views(clippingShapes.size());
    RaySolidQueryStatus status=RaySolidQueryStatus::Unsupported;
    if (buildClippedExpression(core,clipped,views)) {
        CsgRayClassification combined;
        status=CsgRothRayClassifier::classify(clipped,ray,query,combined);
        if (status==RaySolidQueryStatus::Supported) result=combined.getSolid();
    }
    for (long int i=0;i<views.size();++i) delete views[i];
    return status;
}

RayCrossingSupport PovrayBody::emitRayCrossings(const Ray& source,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    // A projected mesh is traced by POV's traversal, not by the VITRAL mesh.
    const bool typedLeaf = geometry != nullptr && surfaceFamily != SurfaceDetailFamily::Mesh &&
        surfaceFamily != SurfaceDetailFamily::BicubicPatch;
    if (surfaceFamily == SurfaceDetailFamily::Mesh &&
        boundingShapes.size() == 0 && clippingShapes.size() == 0) {
        return emitMeshCrossings(source, query, sink);
    }
    if (typedLeaf && boundingShapes.size() == 0 && clippingShapes.size() == 0) {
        if (!surfaceState.isInverted()) return SceneSolid::emitRayCrossings(source, query, sink);
        BoxInvertedCrossingSink reversed(sink);
        return SceneSolid::emitRayCrossings(source, query, reversed);
    }
    RayCastingQueryAdapter context(source);
    auto* queue = context.pool.pop(128);
    const_cast<PovrayBody*>(this)->doIntersectionForAllRayCrossings(&context.ray, queue);
    while (queue->size() > 0) {
        const IntersectionCandidate candidate = queue->poll();
        const double t = candidate.getIntersection().t;
        if (t < query.minT || t > query.maxT) continue;
        RayCrossing event(t, candidate.getAttributes().getHitSurface(),
            RayCrossing::Kind::Unclassified);
        event.surfaceIndex = candidate.getAttributes().getSurfaceIndex();
        event.body = dynamic_cast<const SceneSolid*>(candidate.getAttributes().getHitBody());
        event.material = candidate.getAttributes().getMaterial();
        sink.crossing(event);
    }
    context.pool.push(queue);
    return RayCrossingSupport::Supported;
}

RayCrossingSupport PovrayBody::emitMeshCrossings(const Ray& source,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    Matrix4x4d forward, inverse;
    const AffinePlacementStatus status = queryAffinePlacement(forward, inverse);
    if (status == AffinePlacementStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status == AffinePlacementStatus::Invalid) return RayCrossingSupport::InvalidInput;
    if (status == AffinePlacementStatus::Indeterminate) return RayCrossingSupport::Indeterminate;
    const Ray local = Ray::fromAffine(inverse.multiply(source.getOrigin()),
        inverse.withoutTranslation().multiply(source.getDirection()), source.getT());
    InstanceRayCrossingBuffer buffer(RayInstanceStep(this, forward, inverse), this,
        getSurfaceMaterial());
    RayCastingQueryAdapter context(local);
    auto* queue = context.pool.pop(128);
    SurfaceIntersector::emitCrossings(surface, surfaceFamily,
        const_cast<SurfaceIntersectionState&>(surfaceState), &context.ray, queue, nullptr, nullptr);
    while (queue->size() > 0) {
        const IntersectionCandidate candidate = queue->poll();
        const double t = candidate.getIntersection().t;
        if (t < query.minT || t > query.maxT) continue;
        RayCrossing event(t, candidate.getAttributes().getHitSurface(),
            RayCrossing::Kind::Unclassified);
        event.surfaceIndex = candidate.getAttributes().getSurfaceIndex();
        event.material = candidate.getAttributes().getMaterial();
        buffer.crossing(event);
    }
    context.pool.push(queue);
    if (!buffer.isValid()) return RayCrossingSupport::Indeterminate;
    buffer.deliver(sink);
    return RayCrossingSupport::Supported;
}

void PovrayBody::setGeometry(Geometry* value)
{
    delete triangleTraversal;
    triangleTraversal = nullptr;
    delete csg;
    csg = nullptr;
    delete light;
    light = nullptr;
    SceneSolid::setGeometry(value);
    bindSurface();
}

void PovrayBody::setCsg(ConstructiveSolidGeometry* value)
{
    setGeometry(nullptr);
    csg = value;
    bindSurface();
}

void PovrayBody::setLight(Light* value)
{
    setGeometry(nullptr);
    light = value;
    bindSurface();
}

void PovrayBody::setGeometryReference(Geometry* value)
{
    delete triangleTraversal;
    triangleTraversal = nullptr;
    delete csg;
    csg = nullptr;
    delete light;
    light = nullptr;
    SceneSolid::setGeometryReference(value);
    bindSurface();
}

void PovrayBody::setTriangleTraversal(ProjectedTriangleTraversal* value)
{
    if (value != nullptr && dynamic_cast<TriangleMesh*>(geometry) == nullptr)
        throw java::IllegalStateException("A projected traversal needs a triangle mesh geometry");
    delete triangleTraversal;
    triangleTraversal = value;
    bindSurface();
}

void PovrayBody::bindSurface()
{
    surfaceState = SurfaceIntersectionState();
    if (csg != nullptr) {
        surface = nullptr;
        surfaceFamily = SurfaceDetailFamily::None;
    } else if (light != nullptr) {
        surface = nullptr;
        surfaceFamily = SurfaceDetailFamily::Empty;
    } else if (geometry != nullptr && triangleTraversal != nullptr) {
        surface = geometry;
        surfaceFamily = SurfaceDetailFamily::Mesh;
        surfaceState.setTriangleTraversal(triangleTraversal);
    } else if (geometry != nullptr) {
        surface = geometry;
        surfaceFamily = SurfaceIntersector::familyOf(geometry);
        SurfaceIntersector::initializeState(geometry, surfaceFamily, surfaceState);
    } else {
        surface = nullptr;
        surfaceFamily = SurfaceDetailFamily::None;
    }
}

BoundingVolumeHierarchy* PovrayBody::createSurfaceBoundingVolume() const
{
    if (csg != nullptr) return csg->createBoundingVolume();
    return SurfaceBoundingVolume::create(surface, surfaceFamily, surfaceState);
}

int PovrayBody::traceSurfaceCrossings(RayWithTracingState* ray,
    java::PriorityQueue<IntersectionCandidate>* depthQueue, Material* materialOverride)
{
    if (csg != nullptr)
        return csg->doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride);
    return SurfaceIntersector::emitCrossings(surface, surfaceFamily, surfaceState, ray, depthQueue,
        materialOverride, nullptr);
}

int PovrayBody::surfaceContainment(const Vector3Dd& point, double distanceTolerance)
{
    if (csg != nullptr)
        return csg->doContainmentTest(point, distanceTolerance);
    return SurfaceIntersector::containment(surface, surfaceFamily, surfaceState, point,
        distanceTolerance);
}

void PovrayBody::invertSurface()
{
    if (SurfaceIntersector::invertsByState(surfaceFamily)) {
        surfaceState.setInverted(!surfaceState.isInverted());
        return;
    }
    // Light geometry has nothing to invert.
    if (csg != nullptr) csg->invert();
    else if (triangleTraversal != nullptr) triangleTraversal->invert();
    else if (light == nullptr) SurfaceIntersector::invertGeometry(surface, surfaceFamily);
    surfaceState.setViewpointCached(false);
}

void PovrayBody::setSurfaceInverted(bool value)
{
    if (value && !SurfaceIntersector::invertsByState(surfaceFamily))
        throw java::IllegalStateException("This surface family is not inverted by its body");
    surfaceState.setInverted(value);
}

void PovrayBody::getSurfaceBounds(GeometryBounds& result) const
{
    // An inverted projected triangle can extend to infinity.
    if (surfaceState.isInverted() ||
        (triangleTraversal != nullptr && triangleTraversal->hasUnboundedFaces())) {
        result.setUnbounded(); return;
    }
    if (csg != nullptr) csg->getBounds(result);
    else if (light != nullptr) result.setUnsupported();
    else getGeometry()->getBounds(result);
}

void PovrayBody::getWorldBounds(GeometryBounds& result) const
{
    if (geometry != nullptr && (surfaceState.isInverted() ||
        (triangleTraversal != nullptr && triangleTraversal->hasUnboundedFaces()))) {
        result.setUnbounded(); return;
    }
    if (light != nullptr) { result.setUnsupported(); return; }
    if (csg != nullptr) {
        GeometryBounds local;
        csg->getBounds(local);
        result = local.transformed(getAffinePlacement());
        return;
    }
    SceneSolid::getWorldBounds(result);
}

GeometryContainment PovrayBody::classifyPoint(const Vector3Dd& point, double distanceTolerance) const
{
    if (triangleTraversal != nullptr) return GeometryContainment::Unsupported;
    const GeometryContainment side = SceneSolid::classifyPoint(point, distanceTolerance);
    // A positive tolerance is answered by classifyPointCertified, which
    // already orients the side; the zero-tolerance path asks the geometry.
    if (distanceTolerance != 0 || !surfaceState.isInverted()) return side;
    if (side == GeometryContainment::Inside) return GeometryContainment::Outside;
    if (side == GeometryContainment::Outside) return GeometryContainment::Inside;
    return side;
}

