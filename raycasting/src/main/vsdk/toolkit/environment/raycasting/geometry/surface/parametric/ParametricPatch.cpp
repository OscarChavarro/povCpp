#include "java/lang/IllegalArgumentException.h"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/RayCastingHit.h"
#include "vsdk/toolkit/environment/raycasting/geometry/surface/parametric/RayCastingParametricBiCubicPatch.h"
#include "java/util/PriorityQueue.txx"

PatchScratch &
RayCastingParametricBiCubicPatch::patchScratchFor(const RayCastingParametricBiCubicPatch *patch)
{
    thread_local java::HashMap<const RayCastingParametricBiCubicPatch *, PatchScratch> scratchByPatch;
    PatchScratch *scratch = scratchByPatch.get(patch);
    if (scratch == nullptr) {
        scratchByPatch.put(patch, PatchScratch());
        scratch = scratchByPatch.get(patch);
    }
    return *scratch;
}

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
RayCastingParametricBiCubicPatch::traceCrossings(RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    const GeometryIntersectionEmissionContext *context)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayBicubicTests();
    PatchScratch &scratch = patchScratchFor(this);
    if (ray->isPrimaryRayEnabled()) {
        scratch.setIntersectionCount(0);
    }
    const int used = scratch.getIntersectionCount();
    double depths[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd points[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd normals[MAX_BICUBIC_INTERSECTIONS];
    const int count = patch.intersect(ray->getOrigin(), ray->getDirection(),
        GeometryConfig::SMALL_TOLERANCE, used, MAX_BICUBIC_INTERSECTIONS, depths, points, normals);
    if (count <= 0) {
        return 0;
    }
    stats.incrementRayBicubicTestsSucceeded();
    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(this);
    localElement.getAttributes().setMaterial(materialOverride);
    if (context != nullptr) {
        localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
            context->getMaterialUsesObjectLocalPoint());
    }
    for (int i = 0; i < count; i++) {
        scratch.getIntersectionPointAt(used + i) = points[i];
        scratch.getNormalVectorAt(used + i) = normals[i];
        if (!ray->isShadowRayEnabled()) {
            scratch.incrementIntersectionCount();
        }
        localElement.getIntersection().t = depths[i];
        localElement.getIntersection().point = points[i];
        localElement.getIntersection().normal = normals[i];
        depthQueue->offer(localElement);
    }
    return 1;
}

int
RayCastingParametricBiCubicPatch::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return traceCrossings(ray, depthQueue, materialOverride, nullptr);
}

int
RayCastingParametricBiCubicPatch::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return traceCrossings(ray, depthQueue, context.getMaterialOverride(), &context);
}

// A patch is not a solid, so an inside test doesn't make sense
int
RayCastingParametricBiCubicPatch::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)point;
    (void)distanceTolerance;
    return OUTSIDE;
}

void
RayCastingParametricBiCubicPatch::doExtraInformation(
    const RayWithTracingState &ray, double t, RayCastingHit *hit)
{
    (void)t;
    if (!hit->needsNormal()) {
        return;
    }
    if (hit->n.length() > GeometryConfig::PARAMETRIC_CURVE_EPSILON) {
        hit->n = hit->n.normalizedFast();
        return;
    }
    computeSurfaceNormal(&hit->n, &hit->p, ray.getConfig());
}

void
RayCastingParametricBiCubicPatch::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *localIntersectionPoint)
{
    computeSurfaceNormal(result, localIntersectionPoint, nullptr);
}

// The normal of the hit kept for this point by the ray that found it.
void
RayCastingParametricBiCubicPatch::computeSurfaceNormal(
    Vector3Dd *result,
    Vector3Dd *localIntersectionPoint,
    const RendererConfiguration * /*config*/)
{
    PatchScratch &scratch = patchScratchFor(this);
    const int intersectionCount = scratch.getIntersectionCount();
    for (int i = 0; i < intersectionCount; i++) {
        const Vector3Dd &cachedPoint = scratch.getIntersectionPointAt(i);
        if (localIntersectionPoint->subtract(cachedPoint).length() <= 1.0e-6) {
            *result = scratch.getNormalVectorAt(i);
            return;
        }
    }
    *result = Vector3Dd(1.0, 0.0, 0.0);
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
