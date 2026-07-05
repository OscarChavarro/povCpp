#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/scene/Composite.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"

int
Composite::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)materialOverride;
    bool intersectionFound;
    bool anyIntersectionFound;
    SimpleBody *boundingShape;
    SimpleBody *clippingShape;
    SimpleBody *localObject;
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
                    Geometry::OUTSIDE) {
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

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];
            stats.incrementClippingRegionTests();
            if (clippingShape->doContainmentTest(
                    worldPointToLocal(localIntersection.getIntersection().point),
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
Composite::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    SimpleBody *boundingShape;
    SimpleBody *clippingShape;
    SimpleBody *localObject;
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

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        if (localObject->doContainmentTest(localPoint, distanceTolerance) != Geometry::OUTSIDE) {
            return Geometry::INSIDE;
        }
    }

    return Geometry::OUTSIDE;
}

Composite::Composite(const Composite &other) :
    SimpleBody(other)
{
    for (long int i = other.getSimpleBodies().size() - 1; i >= 0; i--) {
        simpleBodies.add(
            (SimpleBody *)other.getSimpleBodies()[i]->copy());
    }
}

Composite::~Composite()
{
    for (long int i = 0; i < simpleBodies.size(); i++) {
        delete simpleBodies[i];
    }
}

void
Composite::detachOwnership()
{
    SimpleBody::detachOwnership();
    simpleBodies.clear();
}

void *
Composite::copy()
{
    return new Composite(*this);
}

void
Composite::translate(Vector3Dd *vector)
{
    applyTranslationToBodyTransform(vector);
    applyOwnedTranslation(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedTranslation(vector);
    }
}

void
Composite::rotate(Vector3Dd *vector)
{
    applyRotationToBodyTransform(vector);
    applyOwnedRotation(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedRotation(vector);
    }
}

void
Composite::scale(Vector3Dd *vector)
{
    applyScaleToBodyTransform(vector);
    applyOwnedScale(vector);
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedScale(vector);
    }
}

void
Composite::propagateOwnedTranslation(Vector3Dd *vector)
{
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedTranslation(vector);
    }
    SimpleBody::propagateOwnedTranslation(vector);
}

void
Composite::propagateOwnedRotation(Vector3Dd *vector)
{
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedRotation(vector);
    }
    SimpleBody::propagateOwnedRotation(vector);
}

void
Composite::propagateOwnedScale(Vector3Dd *vector)
{
    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        this->getSimpleBodies()[i]->propagateOwnedScale(vector);
    }
    SimpleBody::propagateOwnedScale(vector);
}

void
Composite::invert()
{
    SimpleBody *localObject;
    SimpleBody *localShape;

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
