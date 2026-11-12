#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/IntersectionCandidate.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingComposite.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "vsdk/toolkit/environment/raycasting/geometry/element/PriorityQueuePool.txx"

void RayCastingComposite::getWorldBounds(GeometryBounds& result) const
{
    result.setEmpty();
    for (long int i = 0; i < simpleBodies.size(); ++i) {
        if (simpleBodies[i] == nullptr) { result.setIndeterminate(); return; }
        GeometryBounds child;
        simpleBodies[i]->getWorldBounds(child);
        result = result.enclosing(child);
    }
    // Child bounds are in composite-local coordinates. Only the composite's
    // outer layer is applied by its traversal; geometry placement belongs to
    // individual children and must not be applied a second time.
    if (transformation != nullptr) result = result.transformed(transformation->transpose());
}

int
RayCastingComposite::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)materialOverride;
    bool intersectionFound;
    bool anyIntersectionFound;
    RayCastingBody *boundingShape;
    RayCastingBody *clippingShape;
    RayCastingBody *localObject;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    GeometryStatistics &stats = *ray->getGeometryStatistics();

    // Body parameterized on the composite-local ray, factored into a lambda so
    // the local clone is built only when this composite carries a transform (an
    // untransformed composite forwards the parent ray with no RayWithTracingState
    // construction); RAII still destroys it across the early bounding rejection.
    auto intersectInCompositeSpace = [&](RayWithTracingState *compositeRay) -> int {
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(compositeRay->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            IntersectionCandidate _boundingHit;
            if (!boundingShape->doIntersectionFirstHitViaCrossings(compositeRay, _boundingHit) &&
                boundingShape->doContainmentTest(rayOrigin, GeometryConfig::SMALL_TOLERANCE) ==
                    RayCastingGeometry::OUTSIDE) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->doIntersectionForAllRayCrossings(compositeRay, localDepthQueue);
    }

    for (IntersectionCandidate& localIntersection : *localDepthQueue) {
        localIntersection.getAttributes().pushDetailOwner(
            localIntersection.getAttributes().getHitBody());
        localIntersection.getAttributes().setHitBody(this);
        if (getTransformation() != nullptr) {
            localIntersection.getIntersection().point =
                getTransformation()->transformPoint(localIntersection.getIntersection().point);
        }
        localIntersection.getIntersection().t =
            localIntersection.getIntersection().point
                .subtract(ray->getOrigin()).length();

        intersectionFound = true;

        // The composite's own clipping shapes live in its outer frame, so
        // containment is tested against the point after this composite's
        // transform (already applied above), not converted back to the
        // composite-local frame.
        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];
            stats.incrementClippingRegionTests();
            if (clippingShape->doContainmentTest(
                    localIntersection.getIntersection().point,
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

    if (getTransformationInverse() != nullptr) {
        RayWithTracingState localRay = RayWithTracingState::localIntersectionClone(*ray);
        localRay.setOrigin(getTransformationInverse()->transformPoint(ray->getOrigin()));
        localRay.setDirection(getTransformationInverse()->transformDirection(ray->getDirection()));
        localRay.setQuadricConstantsCached(false);
        return intersectInCompositeSpace(&localRay);
    }
    return intersectInCompositeSpace(ray);
}

int
RayCastingComposite::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    RayCastingBody *boundingShape;
    RayCastingBody *clippingShape;
    RayCastingBody *localObject;
    const Vector3Dd localPoint = worldPointToLocal(point);

    // The composite's own bounding/clipping shapes live in its outer frame
    // (the frame `point` arrives in), not in the composite-local frame used
    // below for child objects.
    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(point, distanceTolerance) == RayCastingGeometry::OUTSIDE) {
            return RayCastingGeometry::OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(point, distanceTolerance) == RayCastingGeometry::OUTSIDE) {
            return RayCastingGeometry::OUTSIDE;
        }
    }

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        if (localObject->doContainmentTest(localPoint, distanceTolerance) != RayCastingGeometry::OUTSIDE) {
            return RayCastingGeometry::INSIDE;
        }
    }

    return RayCastingGeometry::OUTSIDE;
}

RayCastingComposite::RayCastingComposite(const RayCastingComposite &other) :
    RayCastingBody(other)
{
    for (long int i = other.getSimpleBodies().size() - 1; i >= 0; i--) {
        simpleBodies.add(
            (RayCastingBody *)other.getSimpleBodies()[i]->copy());
    }
}

RayCastingComposite::~RayCastingComposite()
{
    for (long int i = 0; i < simpleBodies.size(); i++) {
        delete simpleBodies[i];
    }
}

void
RayCastingComposite::detachOwnership()
{
    RayCastingBody::detachOwnership();
    simpleBodies.clear();
}

void *
RayCastingComposite::copy()
{
    return new RayCastingComposite(*this);
}

void
RayCastingComposite::translate(Vector3Dd *vector)
{
    applyTranslationToBodyTransform(vector);
    applyOwnedTranslation(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedTranslation(vector);
    }
}

void
RayCastingComposite::rotate(Vector3Dd *vector)
{
    applyRotationToBodyTransform(vector);
    applyOwnedRotation(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedRotation(vector);
    }
}

void
RayCastingComposite::scale(Vector3Dd *vector)
{
    applyScaleToBodyTransform(vector);
    applyOwnedScale(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedScale(vector);
    }
}

void
RayCastingComposite::propagateOwnedTranslation(Vector3Dd *vector)
{
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedTranslation(vector);
    }
    RayCastingBody::propagateOwnedTranslation(vector);
}

void
RayCastingComposite::propagateOwnedRotation(Vector3Dd *vector)
{
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedRotation(vector);
    }
    RayCastingBody::propagateOwnedRotation(vector);
}

void
RayCastingComposite::propagateOwnedScale(Vector3Dd *vector)
{
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedScale(vector);
    }
    RayCastingBody::propagateOwnedScale(vector);
}

void
RayCastingComposite::invert()
{
    RayCastingBody *localObject;
    RayCastingBody *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];
        localObject->invert();
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invert();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invert();
    }
}
