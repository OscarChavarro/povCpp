#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/geometry/trace/PolynomialTrace.h"
#include "java/util/PriorityQueue.txx"

PolynomialShape::PolynomialShape(int initialOrder) :
    PolynomialShape(initialOrder, 0)
{
}

PolynomialShape::PolynomialShape(int initialOrder, int sturmFlagValue) :
    surface(initialOrder, sturmFlagValue != 0)
{
}

PolynomialShape::PolynomialShape(const PolynomialShape &other) :
    surface(other.surface),
    traceState(other.traceState)
{
}

PolynomialShape::~PolynomialShape()
{
}

int
PolynomialShape::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return PolynomialTrace::emitCrossings(surface, ray, depthQueue, materialOverride, nullptr);
}

int
PolynomialShape::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return PolynomialTrace::emitCrossings(surface, ray, depthQueue,
        context.getMaterialOverride(), &context);
}

int
PolynomialShape::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return PolynomialTrace::containment(surface, traceState, testPoint, distanceTolerance);
}

void *
PolynomialShape::copy()
{
    return new PolynomialShape(*this);
}

PolynomialShape *
PolynomialShape::copyWithSturmFlag(int flag) const
{
    PolynomialShape * const newShape = new PolynomialShape(surface.getOrder(), flag);
    newShape->traceState = traceState;
    for (int i = 0; i < PolynomialSurface::getTermCount(surface.getOrder()); i++) {
        newShape->surface.getCoefficients()[i] = surface.getCoefficients()[i];
    }
    return newShape;
}

void
PolynomialShape::invertGeometry()
{
    traceState.setInverted(!traceState.isInverted());
}

RaySolidQueryStatus
PolynomialShape::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    const RaySolidQueryStatus status = surface.classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && traceState.isInverted()) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}
