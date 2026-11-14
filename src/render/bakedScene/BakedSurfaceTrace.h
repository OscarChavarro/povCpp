#ifndef __BAKED_SURFACE_TRACE__
#define __BAKED_SURFACE_TRACE__

#include "environment/geometry/RayCastingGeometry.h"
#include "environment/scene/SurfaceTraceKernel.h"

// Trace of a bake record's surface: the family kernel chosen when the record
// was built, with the record's trace state. Geometry without a family kernel
// (a POV CSG traced generically) is traced by the geometry itself.
class BakedSurfaceTrace {
  public:
    static int emitCrossings(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState *state, RayCastingGeometry *geometry, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue, Material *materialOverride)
    {
        if (family == SurfaceDetailFamily::None) {
            return geometry->doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride);
        }
        return SurfaceTraceKernel::emitCrossings(surface, family, *state, ray, depthQueue,
            materialOverride, nullptr);
    }

    static int emitCrossingsAnnotated(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState *state, RayCastingGeometry *geometry, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context)
    {
        if (family == SurfaceDetailFamily::None) {
            return geometry->doIntersectionForAllRayCrossingsAnnotated(ray, depthQueue, context);
        }
        return SurfaceTraceKernel::emitCrossings(surface, family, *state, ray, depthQueue,
            context.getMaterialOverride(), &context);
    }

    static bool firstHit(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState *state, RayCastingGeometry *geometry, RayWithTracingState *ray,
        IntersectionCandidate &out, Material *materialOverride)
    {
        if (family == SurfaceDetailFamily::None) {
            return geometry->doIntersectionFirstHit(ray, out, materialOverride);
        }
        return SurfaceTraceKernel::firstHit(surface, family, *state, ray, out, materialOverride);
    }

    static int containment(Geometry *surface, SurfaceDetailFamily family,
        const SurfaceTraceState *state, RayCastingGeometry *geometry, const Vector3Dd &point,
        double distanceTolerance)
    {
        if (family == SurfaceDetailFamily::None) {
            return geometry->doContainmentTest(point, distanceTolerance);
        }
        return SurfaceTraceKernel::containment(surface, family, *state, point, distanceTolerance);
    }
};

#endif
