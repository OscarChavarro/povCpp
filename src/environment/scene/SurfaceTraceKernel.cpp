#include "java/lang/IllegalStateException.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "environment/scene/SurfaceTraceKernel.h"
#include "environment/scene/NativeSurfaceTrace.h"
#include "environment/scene/ProjectedTriangleTraversal.h"
#include "environment/geometry/trace/QuadricTrace.h"
#include "environment/geometry/trace/BoxTrace.h"
#include "environment/geometry/trace/PlaneTrace.h"
#include "environment/geometry/trace/PolynomialTrace.h"
#include "environment/geometry/trace/BlobTrace.h"
#include "environment/geometry/trace/HeightFieldTrace.h"
#include "environment/geometry/trace/BicubicPatchTrace.h"

int
SurfaceTraceKernel::emitCrossings(Geometry *surface, SurfaceDetailFamily family,
    SurfaceTraceState &state, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    switch (family) {
    case SurfaceDetailFamily::Quadric:
        return QuadricTrace::emitCrossings(*static_cast<QuadricSurface *>(surface), state, ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Box:
        return BoxTrace::emitCrossings(*static_cast<Box *>(surface), ray, depthQueue,
            materialOverride, context);
    case SurfaceDetailFamily::Plane:
        return PlaneTrace::emitCrossings(*static_cast<PlanarHalfSpace *>(surface), state, ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Polynomial:
        return PolynomialTrace::emitCrossings(*static_cast<PolynomialSurface *>(surface), ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Blob:
        return BlobTrace::emitCrossings(*static_cast<BlobSolid *>(surface), ray, depthQueue,
            materialOverride, context);
    case SurfaceDetailFamily::HeightField:
        return HeightFieldTrace::emitCrossings(*static_cast<HeightFieldSolid *>(surface), ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::BicubicPatch:
        return BicubicPatchTrace::emitCrossings(*static_cast<SubdividedBezierPatch *>(surface),
            ray, depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Sphere:
    case SurfaceDetailFamily::Mesh:
    case SurfaceDetailFamily::Generic: {
        const GeometryIntersectionEmissionContext plain(materialOverride, nullptr, false);
        const GeometryIntersectionEmissionContext &effective = context != nullptr ? *context : plain;
        if (family == SurfaceDetailFamily::Mesh) {
            return state.getTriangleTraversal()->emit(ray, depthQueue, effective);
        }
        if (family == SurfaceDetailFamily::Sphere) {
            return NativeSurfaceTrace::emitSphereCrossings(*static_cast<Sphere *>(surface), ray,
                depthQueue, effective);
        }
        return NativeSurfaceTrace::emitGenericCrossings(*surface, ray, depthQueue, effective);
    }
    case SurfaceDetailFamily::Empty:
        return false;
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no trace kernel");
}

bool
SurfaceTraceKernel::firstHit(Geometry *surface, SurfaceDetailFamily family,
    SurfaceTraceState &state, RayWithTracingState *ray, IntersectionCandidate &out,
    Material *materialOverride)
{
    switch (family) {
    case SurfaceDetailFamily::Quadric:
        return QuadricTrace::firstHit(*static_cast<QuadricSurface *>(surface), state, ray, out,
            materialOverride);
    case SurfaceDetailFamily::Box:
        return BoxTrace::firstHit(*static_cast<Box *>(surface), ray, out, materialOverride);
    case SurfaceDetailFamily::Plane:
        return PlaneTrace::firstHit(*static_cast<PlanarHalfSpace *>(surface), state, ray, out,
            materialOverride);
    case SurfaceDetailFamily::Sphere:
        return NativeSurfaceTrace::sphereFirstHit(*static_cast<Sphere *>(surface), ray, out,
            materialOverride);
    case SurfaceDetailFamily::Generic:
        return NativeSurfaceTrace::genericFirstHit(*surface, ray, out, materialOverride);
    case SurfaceDetailFamily::Mesh:
    case SurfaceDetailFamily::Polynomial:
    case SurfaceDetailFamily::Blob:
    case SurfaceDetailFamily::HeightField:
    case SurfaceDetailFamily::BicubicPatch:
    case SurfaceDetailFamily::Empty:
        return false;
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no trace kernel");
}

int
SurfaceTraceKernel::containment(Geometry *surface, SurfaceDetailFamily family,
    const SurfaceTraceState &state, const Vector3Dd &point, double distanceTolerance)
{
    switch (family) {
    case SurfaceDetailFamily::Quadric:
        return QuadricTrace::containment(*static_cast<const QuadricSurface *>(surface), point,
            distanceTolerance);
    case SurfaceDetailFamily::Box:
        return BoxTrace::containment(*static_cast<const Box *>(surface), state, point,
            distanceTolerance);
    case SurfaceDetailFamily::Plane:
        return PlaneTrace::containment(*static_cast<const PlanarHalfSpace *>(surface), point,
            distanceTolerance);
    case SurfaceDetailFamily::Polynomial:
        return PolynomialTrace::containment(*static_cast<const PolynomialSurface *>(surface),
            state, point, distanceTolerance);
    case SurfaceDetailFamily::Blob:
        return BlobTrace::containment(*static_cast<const BlobSolid *>(surface), state, point,
            distanceTolerance);
    case SurfaceDetailFamily::HeightField:
        return HeightFieldTrace::containment(*static_cast<const HeightFieldSolid *>(surface),
            point, distanceTolerance);
    case SurfaceDetailFamily::Sphere:
        return NativeSurfaceTrace::sphereContainment(*static_cast<const Sphere *>(surface), state,
            point, distanceTolerance);
    case SurfaceDetailFamily::Generic:
        return NativeSurfaceTrace::genericContainment(*surface, state, point, distanceTolerance);
    case SurfaceDetailFamily::Mesh:
    case SurfaceDetailFamily::BicubicPatch:
    case SurfaceDetailFamily::Empty:
        return static_cast<int>(Containment::OUTSIDE);
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no trace kernel");
}
