#ifndef __BAKED_SURFACE_TRACE__
#define __BAKED_SURFACE_TRACE__

#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/scene/SurfaceTraceKernel.h"

// Trace of a bake record's surface: the family kernel chosen when the record
// was built, with the record's trace state. A POV CSG has no family kernel and
// is traced by its own evaluator.
class BakedSurfaceTrace {
  public:
    static int emitCrossings(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState *state, ConstructiveSolidGeometry *csg, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue, Material *materialOverride)
    {
        if (family == SurfaceDetailFamily::None) {
            return csg->doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride);
        }
        return SurfaceTraceKernel::emitCrossings(surface, family, *state, ray, depthQueue,
            materialOverride, nullptr);
    }

    static int emitCrossingsAnnotated(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState *state, ConstructiveSolidGeometry *csg, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context)
    {
        if (family == SurfaceDetailFamily::None) {
            return csg->doIntersectionForAllRayCrossingsAnnotated(ray, depthQueue, context);
        }
        return SurfaceTraceKernel::emitCrossings(surface, family, *state, ray, depthQueue,
            context.getMaterialOverride(), &context);
    }

    static bool firstHit(Geometry *surface, SurfaceDetailFamily family,
        SurfaceTraceState *state, ConstructiveSolidGeometry *csg, RayWithTracingState *ray,
        IntersectionCandidate &out, Material *materialOverride)
    {
        if (family == SurfaceDetailFamily::None) {
            (void)csg;
            return false;
        }
        return SurfaceTraceKernel::firstHit(surface, family, *state, ray, out, materialOverride);
    }

    static int containment(Geometry *surface, SurfaceDetailFamily family,
        const SurfaceTraceState *state, ConstructiveSolidGeometry *csg, const Vector3Dd &point,
        double distanceTolerance)
    {
        if (family == SurfaceDetailFamily::None) {
            return csg->doContainmentTest(point, distanceTolerance);
        }
        return SurfaceTraceKernel::containment(surface, family, *state, point, distanceTolerance);
    }
};

#endif
