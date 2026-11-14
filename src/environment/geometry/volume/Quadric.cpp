#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/trace/QuadricTrace.h"
#include "environment/geometry/volume/Quadric.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

Quadric::Quadric() :
    Quadric(Vector3Dd(1.0, 1.0, 1.0), Vector3Dd(0.0, 0.0, 0.0),
        Vector3Dd(0.0, 0.0, 0.0), 1.0)
{
}

Quadric::Quadric(const Vector3Dd &object2Terms,
    const Vector3Dd &objectMixedTerms,
    const Vector3Dd &objectTerms, double objectConstant) :
    Quadric(object2Terms, objectMixedTerms, objectTerms, objectConstant,
        HUGE_VAL, false, false)
{
    updateSquareTermFlag();
}

Quadric::Quadric(const Vector3Dd &object2Terms,
    const Vector3Dd &objectMixedTerms,
    const Vector3Dd &objectTerms, double objectConstant,
    double objectVpConstant, bool constantCached, bool nonZeroSquareTerm) :
    surface(object2Terms, objectMixedTerms, objectTerms, objectConstant)
{
    traceState.setViewpointValue(objectVpConstant);
    traceState.setViewpointCached(constantCached);
    traceState.setNonZeroSquareTerm(nonZeroSquareTerm);
}

void
Quadric::updateSquareTermFlag()
{
    traceState.setNonZeroSquareTerm(
        !((surface.squareCoefficients().x() == 0.0) &&
            (surface.squareCoefficients().y() == 0.0) &&
            (surface.squareCoefficients().z() == 0.0) &&
            (surface.mixedCoefficients().x() == 0.0) &&
            (surface.mixedCoefficients().y() == 0.0) &&
            (surface.mixedCoefficients().z() == 0.0)));
}

int
Quadric::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return QuadricTrace::emitCrossings(surface, traceState, ray, depthQueue, materialOverride,
        nullptr);
}

int
Quadric::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return QuadricTrace::emitCrossings(surface, traceState, ray, depthQueue,
        context.getMaterialOverride(), &context);
}

bool
Quadric::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    return QuadricTrace::firstHit(surface, traceState, ray, out, materialOverride);
}

int
Quadric::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return QuadricTrace::containment(surface, testPoint, distanceTolerance);
}

void *
Quadric::copy()
{
    return new Quadric(*this);
}

BoundingVolumeHierarchy *
Quadric::createBoundingVolume() const
{
    GeometryBounds bounds;
    surface.getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new AabbBoundingVolume(bounds.getMinimum(), bounds.getMaximum());
    return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
}

void
Quadric::invertGeometry()
{
    surface.squareCoefficients() = surface.squareCoefficients().multiply(-1.0);
    surface.mixedCoefficients() = surface.mixedCoefficients().multiply(-1.0);
    surface.linearCoefficients() = surface.linearCoefficients().multiply(-1.0);
    surface.constantCoefficient() *= -1.0;
    traceState.setViewpointCached(false);
}
