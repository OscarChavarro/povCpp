#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/scene/Composite.h"
#include "environment/material/Material.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"

int
Composite::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)materialOverride;
    bool intersectionFound;
    bool anyIntersectionFound;
    SimpleBody *boundingShape;
    SimpleBody *clippingShape;
    IntersectionCandidate localIntersection;
    SimpleBody *localObject;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    Statistics &stats = *ray->getStatistics();
    RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, *ray);
    RayWithSegments *compositeRay = ray;

    if (getTransformationInverse() != nullptr) {
        localRay.setOrigin(getTransformationInverse()->transformPoint(ray->getOrigin()));
        localRay.setDirection(getTransformationInverse()->transformDirection(ray->getDirection()));
        localRay.setQuadricConstantsCached(false);
        compositeRay = &localRay;
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(compositeRay->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            IntersectionCandidate _boundingHit;
            if (!boundingShape->doIntersectionFirstHit(compositeRay, _boundingHit) &&
                boundingShape->doContainmentTest(rayOrigin, Config::SMALL_TOLERANCE) ==
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

    for (const IntersectionCandidate& candidate : *localDepthQueue) {
        localIntersection = candidate;
        // Append (not prepend) the child as a detail owner so the detail-owner
        // stack is built innermost-first, exactly like CsgOperand. The normal
        // chain is consumed outermost-first via PovRayHit::popDetailOwnerBack
        // (see SimpleBody::doExtraInformation); a single consume direction can
        // only be correct if every producer pushes in the same order. The
        // original prependDetailOwner reversed the order for composites, so a
        // nested composite (e.g. fish13's stems: composite{composite{...sphere
        // texture}} placed with scale<3 3 3>) had its enclosing transforms
        // applied to the surface normal in scrambled order, flattening the
        // marble + phong shading. Appending matches the CSG ordering and keeps
        // the §15 frame/CSG fixes intact.
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
