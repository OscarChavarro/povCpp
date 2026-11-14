#include "java/lang/IllegalArgumentException.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/RayCastingHit.h"
#include "environment/geometry/surface/parametric/RayCastingParametricBiCubicPatch.h"
#include "environment/geometry/trace/BicubicPatchTrace.h"
#include "java/util/PriorityQueue.txx"


BezierPatchSubdivision
RayCastingParametricBiCubicPatch::subdivisionOf(int patchType)
{
    switch (patchType) {
    case 0: return BezierPatchSubdivision::UniformGrid;
    case 1: return BezierPatchSubdivision::PrecomputedGrid;
    case 2: return BezierPatchSubdivision::Adaptive;
    case 3: return BezierPatchSubdivision::AdaptiveTree;
    case 4: return BezierPatchSubdivision::SmoothGrid;
    default: break;
    }
    throw java::IllegalArgumentException("bicubic_patch type must be 0 to 4");
}

RayCastingParametricBiCubicPatch::RayCastingParametricBiCubicPatch(int patchType, int uSteps,
    int vSteps, double flatnessValue, const Vector3Dd (&controlPoints)[4][4]) :
    patch(subdivisionOf(patchType), uSteps, vSteps, flatnessValue, controlPoints)
{
}

// The per-ray scratch is not copied: it belongs to the rays that hit other.
RayCastingParametricBiCubicPatch::RayCastingParametricBiCubicPatch(
    const RayCastingParametricBiCubicPatch &other) :
    patch(other.patch)
{
}

RayCastingParametricBiCubicPatch::~RayCastingParametricBiCubicPatch()
{
}

int
RayCastingParametricBiCubicPatch::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return BicubicPatchTrace::emitCrossings(patch, ray, depthQueue, materialOverride, nullptr);
}

int
RayCastingParametricBiCubicPatch::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return BicubicPatchTrace::emitCrossings(patch, ray, depthQueue,
        context.getMaterialOverride(), &context);
}

// A patch is not a solid, so an inside test doesn't make sense
int
RayCastingParametricBiCubicPatch::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)point;
    (void)distanceTolerance;
    return OUTSIDE;
}


void *
RayCastingParametricBiCubicPatch::copy()
{
    return new RayCastingParametricBiCubicPatch(*this);
}

void
RayCastingParametricBiCubicPatch::invertGeometry()
{
}

BoundingVolumeHierarchy *
RayCastingParametricBiCubicPatch::createBoundingVolume() const
{
    AxisAlignedBoundingBox result = AxisAlignedBoundingBox::empty();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result = result.expandedBy(patch.getControlPoint(i, j));
        }
    }
    return new AabbBoundingVolume(result);
}
